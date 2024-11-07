#include "planificador_corto_plazo.h"
#include <utils/utils.h>
#include <syscalls.h>

extern int conexion_kernel_cpu;
t_tcb* hilo_actual;  
t_pcb* proceso_actual;

t_cola_hilo* hilos_cola_ready;
t_cola_hilo* hilos_cola_bloqueados;
t_colas_multinivel *colas_multinivel;

extern t_config *config;
char* algoritmo;
int quantum;

extern pthread_mutex_t mutex_socket_cpu_dispatch;
extern pthread_mutex_t mutex_socket_cpu_interrupt;

/*
Planificador Corto Plazo
    -> Leer el archivo .config para obtener el algoritmo de planificación
        -> FIFO        V
        -> Prioridades V
        -> Multicolas
            -> Una cola por cada nivel de prioridad V
            -> Entre colas prioridades sin desalojo V
    -> Seleccion de hilos a ejecutar
        -> Transicionarlo al estado EXEC V
        -> Mandarlo a CPU por el dispatch V
        -> Esperar recibirlo con un motivo (si es necesario replanificarlo)
        -> En caso de que se necesite replanificar (por fin de q x ej) se manda a CPU por interrupt
*/
void obtener_planificador_corto_plazo()
{ 
    algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    if (strcmp(algoritmo, "FIFO") == 0)
        corto_plazo_fifo();
    else if (strcmp(algoritmo, "PRIORIDADES") == 0)
        corto_plazo_prioridades();
    else if (strcmp(algoritmo, "CMN") == 0){
        corto_plazo_colas_multinivel(colas_multinivel);
        }
    else
    {
        // mensaje de error
    }
}

void corto_plazo_fifo()
{
    while (1)
    {
        sem_wait(hilos_cola_ready->sem_estado);
        pthread_mutex_lock(hilos_cola_ready->mutex_estado);
        t_tcb *hilo = desencolar_hilos_fifo();
        // Debería encolarlo en una cola de EXEC o modificar su estado solamente
        hilo->estado = EXEC; 
        hilo_actual = hilo;
        pthread_mutex_unlock(hilos_cola_ready->mutex_estado);
        sem_post(hilos_cola_ready->sem_estado);
        enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        //recibir_motivo_devolucion();
    }
}

t_tcb* desencolar_hilos_fifo(){
    t_tcb *hilo = list_remove(hilos_cola_ready->lista_hilos, 0);
    return hilo;
}

void corto_plazo_prioridades()
{
    while(1){
        sem_wait(hilos_cola_ready->sem_estado);
        pthread_mutex_lock(hilos_cola_ready->mutex_estado);
        // Ordenar la lista según prioridad (0 es la máxima prioridad)
        list_sort(hilos_cola_ready->lista_hilos, (void *)comparar_prioridades); 
        // Desencolar el hilo con la prioridad más baja (en la cabeza de la lista)
        t_tcb *hilo = desencolar_hilos_prioridades();
        hilo->estado = EXEC;  
        hilo_actual = hilo;
        enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        // Recibir el motivo de la devolución y actuar en consecuencia
        //recibir_motivo_devolucion();
    }
}

t_tcb* desencolar_hilos_prioridades()
{
    sem_wait(hilos_cola_ready->sem_estado);
    pthread_mutex_lock(hilos_cola_ready->mutex_estado);
    list_sort(hilos_cola_ready->lista_hilos, (void*)comparar_prioridades);
    t_tcb *hilo = list_remove(hilos_cola_ready->lista_hilos, 0);
    hilo->estado = EXIT;
    pthread_mutex_unlock(hilos_cola_ready->mutex_estado);
    return hilo;
}

int comparar_prioridades(t_tcb *a, t_tcb *b) {
    return a->prioridad - b->prioridad;
}

void corto_plazo_colas_multinivel(t_colas_multinivel *multinivel) {
    while (1) {
        for (int i = 0; i < list_size(multinivel->niveles_prioridad); i++) {
            t_nivel_prioridad *nivel = list_get(multinivel->niveles_prioridad, i);
            t_cola_hilo *cola = nivel->cola_hilos;

            // Comprobar si la cola no está vacía
            if (!list_is_empty(cola->lista_hilos)) {
                ejecutar_round_robin(cola, nivel->nivel_prioridad);
                break;
            }
        }
    }
}

void ejecutar_round_robin(t_cola_hilo *cola, int prioridad) {
    t_tcb *hilo_a_ejecutar = list_remove(cola->lista_hilos, 0); // Extrae el primer hilo en la cola

    // Enviamos el hilo a la CPU mediante el canal de dispatch
    pthread_mutex_lock(&mutex_socket_cpu_dispatch);
    enviar_a_cpu_dispatch(hilo_a_ejecutar->tid, hilo_a_ejecutar->pid); // Envía el TID y PID al CPU
    pthread_mutex_unlock(&mutex_socket_cpu_dispatch);

    // Lanzamos un nuevo hilo que cuente el quantum y envíe una interrupción si se agota
    pthread_t thread_contador_quantum;
    pthread_create(&thread_contador_quantum, NULL, (void *)contar_quantum, (void *)hilo_a_ejecutar);
    //recibir_motivo_devolucion(); 
    pthread_join(thread_contador_quantum, NULL); // Esperamos que el hilo del quantum termine
}

// Función para contar el quantum y enviar una interrupción si se agota
void contar_quantum(void *hilo_void) {
    t_tcb* hilo = (t_tcb*) hilo_void;
    usleep(quantum * 1000);

    // Si el quantum se agotó, enviamos una interrupción al CPU por el canal de interrupt
    /*
    Si el hilo finalizó, igual se manda la interrupcion por Fin de Quantum, pero la CPU chequea que ese hilo ya 
    terminó y la desestima
    */
    pthread_mutex_lock(&mutex_socket_cpu_interrupt);
    enviar_a_cpu_interrupt(hilo->tid); // Usa la función para enviar la interrupción
    pthread_mutex_unlock(&mutex_socket_cpu_interrupt);
    //log_info(kernel_logger, "Interrupción por fin de quantum enviada para el TID %d", hilo->tid);

}

void encolar_hilo_multinivel(t_tcb* hilo) {
    // Asignar el hilo actual antes de llamar a nivel_existe
    hilo_actual = hilo;  // Guarda el hilo que se está encolando

    int prioridad = hilo->prioridad;  // Obtener la prioridad del hilo

    // Buscamos el nivel de prioridad en la lista de niveles
    t_nivel_prioridad* nivel = list_find(colas_multinivel->niveles_prioridad, nivel_existe);

    if (nivel == NULL) {
        // No se encontró la cola con el nivel de prioridad del hilo
        printf("Nivel de prioridad %d no existe. Creando nueva cola...\n", prioridad);
        
        // Crear una nueva cola de hilos
        t_cola_hilo* nueva_cola = malloc(sizeof(t_cola_hilo));
        nueva_cola->nombre_estado = READY;
        nueva_cola->lista_hilos = list_create();
        pthread_mutex_init(nueva_cola->mutex_estado, NULL);
        sem_init(nueva_cola->sem_estado, 0, 0);

        // Crear un nuevo nivel de prioridad y asociarlo con la nueva cola de hilos
        t_nivel_prioridad* nuevo_nivel = malloc(sizeof(t_nivel_prioridad));
        nuevo_nivel->nivel_prioridad = prioridad;
        nuevo_nivel->cola_hilos = nueva_cola;

        // Agregar el nuevo nivel de prioridad a la lista de niveles
        list_add(colas_multinivel->niveles_prioridad, nuevo_nivel);
        
        // Encolar el hilo en la nueva cola
        pthread_mutex_lock(nueva_cola->mutex_estado); 
        list_add(nueva_cola->lista_hilos, hilo);       
        pthread_mutex_unlock(nueva_cola->mutex_estado); 

        // Señalizar que hay un hilo en la nueva cola
        sem_post(nueva_cola->sem_estado);
    } else {
        // Si ya existe el nivel, encolamos el hilo en la cola correspondiente
        pthread_mutex_lock(nivel->cola_hilos->mutex_estado); 
        list_add(nivel->cola_hilos->lista_hilos, hilo);       
        pthread_mutex_unlock(nivel->cola_hilos->mutex_estado); 

        // Señalizar que hay un hilo en la cola de este nivel
        sem_post(nivel->cola_hilos->sem_estado);
    }
}

bool nivel_existe(void* elemento) {
    t_nivel_prioridad* nivel = (t_nivel_prioridad*)elemento;
    // Compara el nivel de prioridad del elemento con la prioridad del hilo actual
    return nivel->nivel_prioridad == hilo_actual->prioridad;
}

void enviar_a_cpu_dispatch(int tid, int pid)
{
    // Crear un paquete con el tid y el pib del hilo seleccionado para ejecutar y lo manda a cpu a través del dispatch
    t_paquete * send_handshake = crear_paquete(INFO_HILO);
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); 
    agregar_a_paquete(send_handshake, &pid, sizeof(pid)); 
    enviar_paquete(send_handshake, conexion_kernel_cpu);
    eliminar_paquete(send_handshake);
    //Se espera la respuesta, primero el tid y luego el motivo
    //recibir_motivo_devolucion();
}

void enviar_a_cpu_interrupt(int tid) {
    // Crear un paquete con el tid del hilo que necesita ser interrumpido
    t_paquete *send_interrupt = crear_paquete(FIN_QUANTUM);
    agregar_a_paquete(send_interrupt, &tid, sizeof(tid)); 
    enviar_paquete(send_interrupt, conexion_kernel_cpu);
    eliminar_paquete(send_interrupt);
}
/*
void recibir_motivo_devolucion() {
    t_list *paquete_respuesta = recibir_paquete(conexion_kernel_cpu);

    // Extraer el TID y el motivo de la devolución
    int tid, motivo;
    obtener_de_paquete(paquete_respuesta, &tid, sizeof(int));
    obtener_de_paquete(paquete_respuesta, &motivo, sizeof(int));

    // Tratamos el motivo:
    switch (motivo) {
    case FINALIZACION:
        printf("El hilo %d ha finalizado correctamente\n", tid);
        break;
    case FIN_QUANTUM:
        printf("El hilo %d fue desalojado por fin de quantum\n", tid);
        // Replanificamos el hilo y lo enviamos de vuelta al planificador
        // Encolar el hilo en la cola correspondiente
        encolar_hilo_multinivel(obtener_tcb_por_tid(tid));
        break;
    default:
        printf("Motivo desconocido para el hilo %d\n", tid);
        break;
    }
    eliminar_paquete(paquete_respuesta);
}
*/