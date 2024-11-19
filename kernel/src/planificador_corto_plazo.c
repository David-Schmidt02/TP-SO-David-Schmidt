#include "planificador_corto_plazo.h"
#include <syscalls.h>

//extern int conexion_kernel_cpu;
//todas estas estructuras luego deben incluirse desde el main con un extern
//luego se debe levantar el planificador de corto y largo plazo en el main

t_tcb* hilo_actual;  
t_pcb* proceso_actual;

t_cola_hilo* hilos_cola_ready;
pthread_mutex_t * mutex_hilos_cola_ready;
sem_t * sem_estado_hilos_cola_ready;

t_cola_hilo* hilos_cola_bloqueados;
pthread_mutex_t * mutex_hilos_cola_bloqueados;
sem_t * sem_estado_hilos_cola_bloqueados;

t_colas_multinivel *colas_multinivel;
pthread_mutex_t * mutex_colas_multinivel;
sem_t * sem_estado_multinivel;

//extern t_config *config;
char* algoritmo;
int quantum;

//extern pthread_mutex_t mutex_socket_cpu_dispatch;
//extern pthread_mutex_t mutex_socket_cpu_interrupt;

/*void obtener_planificador_corto_plazo()
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
*/
void corto_plazo_fifo()
{
    while (1)
    {
        sem_wait(sem_estado_hilos_cola_ready);
        pthread_mutex_lock(mutex_hilos_cola_ready);
        t_tcb *hilo = desencolar_hilos_fifo();
        hilo->estado = EXEC; 
        hilo_actual = hilo;
        pthread_mutex_unlock(mutex_hilos_cola_ready);
        //enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        //recibir_motivo_devolucion();
    }
}

void encolar_corto_plazo_fifo(t_tcb * hilo){
    pthread_mutex_lock(mutex_hilos_cola_ready);
    list_add(hilos_cola_ready->lista_procesos, hilo);
    pthread_mutex_unlock(mutex_hilos_cola_ready);
    sem_post(sem_estado_hilos_cola_ready);
}

t_tcb* desencolar_hilos_fifo(){
    t_tcb *hilo = list_remove(hilos_cola_ready->lista_hilos, 0);
    return hilo;
}

void corto_plazo_prioridades()
{
    while(1){
        sem_wait(sem_estado_hilos_cola_ready);
        pthread_mutex_lock(mutex_hilos_cola_ready);
        list_sort(hilos_cola_ready->lista_hilos, (void *)comparar_prioridades); 
        t_tcb *hilo = desencolar_hilos_prioridades();
        hilo->estado = EXEC;  
        hilo_actual = hilo;
        pthread_mutex_unlock(mutex_hilos_cola_ready);
        //enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        //recibir_motivo_devolucion();
    }
}

void encolar_corto_plazo_prioridades(t_tcb * hilo){
    pthread_mutex_lock(mutex_hilos_cola_ready);
    int i = 0;
    while (i < list_size(hilos_cola_ready->lista_hilos)) {
        t_tcb *hilo_existente = list_get(hilos_cola_ready->lista_hilos, i);
        if (hilos_cola_ready->prioridad > hilos_cola_ready->prioridad) {
            break;
        }
        i++;
    }
    list_add_in_index(hilos_cola_ready->lista_hilos, i, hilo);
    pthread_mutex_unlock(mutex_hilos_cola_ready);
}

t_tcb* desencolar_hilos_prioridades()
{
    list_sort(hilos_cola_ready->lista_hilos, (void*)comparar_prioridades);
    t_tcb *hilo = list_remove(hilos_cola_ready->lista_hilos, 0);
    return hilo;
}

int comparar_prioridades(t_tcb *a, t_tcb *b) {
    return a->prioridad - b->prioridad;
}

void corto_plazo_colas_multinivel() {
    while (1) {
        int replanificar = 0; // Variable para saber si hay que replanificar

        // Bloqueamos el mutex de la cola multinivel para evitar modificaciones mientras ejecutamos
        // Para ejecutar primero debo esperar a que haya un proceso
        printf("Entró al while del planificador\n");
        sem_wait(sem_estado_multinivel);
        printf("PASO EL SEMÁFORO Y PLANIFICA EL HILO\n");
        pthread_mutex_lock(mutex_colas_multinivel);
        t_nivel_prioridad *nivel_a_ejecutar = NULL;
        t_cola_hilo *cola_a_ejecutar = buscar_cola_menor_prioridad(colas_multinivel, &nivel_a_ejecutar);

        // Si encontramos una cola válida para ejecutar
        if (cola_a_ejecutar != NULL && nivel_a_ejecutar != NULL) {
            // Extraemos el primer hilo en la cola de menor prioridad
            t_tcb *hilo_a_ejecutar = list_remove(cola_a_ejecutar->lista_hilos, 0);

            // Mostrar información de prueba
            printf("Cola de prioridad %d: Ejecutando hilo TID=%d, PID=%d\n", nivel_a_ejecutar->nivel_prioridad, hilo_a_ejecutar->tid, hilo_a_ejecutar->pid);

            // Ejecutamos el hilo utilizando round robin
            ejecutar_round_robin(cola_a_ejecutar, hilo_a_ejecutar);

            // Indicamos que hay que replanificar, ya que se ha ejecutado un hilo
            replanificar = 1;
        }

        // Desbloqueamos el mutex para permitir que otros hilos puedan ser encolados
        pthread_mutex_unlock(mutex_colas_multinivel);

        // Si no se ejecutó ningún hilo, esperamos un poco antes de intentar nuevamente
        if (!replanificar) {
            usleep(100); 
        }
        // Si se ejecutó un hilo, volvemos a planificar
        if (replanificar) {
            continue;
        }
    }
}

t_cola_hilo* buscar_cola_menor_prioridad(t_colas_multinivel *multinivel, t_nivel_prioridad **nivel_a_ejecutar) {
    t_cola_hilo *cola_a_ejecutar = NULL;
    *nivel_a_ejecutar = NULL;

    // Recorremos los niveles de prioridad para encontrar el nivel con la menor prioridad que tenga hilos
    for (int i = 0; i < list_size(multinivel->niveles_prioridad); i++) {
        t_nivel_prioridad *nivel = list_get(multinivel->niveles_prioridad, i);
        t_cola_hilo *cola = nivel->cola_hilos;

        // Verificamos si la cola de hilos no está vacía
        if (!list_is_empty(cola->lista_hilos)) {
            // Si no hemos encontrado una cola válida o el nivel actual tiene menor prioridad
            if (*nivel_a_ejecutar == NULL || nivel->nivel_prioridad < (*nivel_a_ejecutar)->nivel_prioridad) {
                *nivel_a_ejecutar = nivel;
                cola_a_ejecutar = cola;
            }
        }
    }

    return cola_a_ejecutar;
}

void ejecutar_round_robin(t_cola_hilo *cola, t_tcb * hilo_a_ejecutar) {

    // Enviamos el hilo a la CPU mediante el canal de dispatch
    //pthread_mutex_lock(&mutex_socket_cpu_dispatch);

    ///////// agregado para pruebas
    printf("Hilo TID=%d, PID=%d ha terminado de ejecutarse\n", hilo_a_ejecutar->tid, hilo_a_ejecutar->pid);
    /////////

    //enviar_a_cpu_dispatch(hilo_a_ejecutar->tid, hilo_a_ejecutar->pid); // Envía el TID y PID al CPU
    //pthread_mutex_unlock(&mutex_socket_cpu_dispatch);

    // Lanzamos un nuevo hilo que cuente el quantum y envíe una interrupción si se agota
    pthread_t thread_contador_quantum;
    pthread_create(&thread_contador_quantum, NULL, (void *)contar_quantum, (void *)hilo_a_ejecutar);
    //recibir_motivo_devolucion(); 
    //pthread_join(thread_contador_quantum, NULL); // Esperamos que el hilo del quantum termine
    
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
    //pthread_mutex_lock(&mutex_socket_cpu_interrupt);
    //enviar_a_cpu_interrupt(hilo->tid); // Usa la función para enviar la interrupción
    //pthread_mutex_unlock(&mutex_socket_cpu_interrupt);
    //log_info(kernel_logger, "Interrupción por fin de quantum enviada para el TID %d", hilo->tid);

}

void encolar_corto_plazo_multinivel(t_tcb* hilo) {
    // Asignar el hilo actual antes de llamar a nivel_existe
    hilo_actual = hilo;  // Guarda el hilo que se está encolando

    int prioridad = hilo->prioridad;  // Obtener la prioridad del hilo

    // Verificar si colas_multinivel está correctamente inicializado
    if (colas_multinivel == NULL) {
        perror("Error: colas_multinivel no está inicializado");
        exit(EXIT_FAILURE);
    }

    // Asegúrate de que niveles_prioridad está inicializado correctamente
    if (colas_multinivel->niveles_prioridad == NULL) {
        perror("Error: niveles_prioridad no está inicializado");
        exit(EXIT_FAILURE);
    }

    // Buscamos el nivel de prioridad en la lista de niveles
    t_nivel_prioridad* nivel = list_find(colas_multinivel->niveles_prioridad, nivel_existe);

    if (nivel == NULL) {
        // No se encontró la cola con el nivel de prioridad del hilo
        printf("Nivel de prioridad %d no existe. Creando nueva cola...\n", prioridad);
        
        // Crear una nueva cola de hilos
        t_cola_hilo* nueva_cola = malloc(sizeof(t_cola_hilo));
        if (nueva_cola == NULL) {
            perror("Error al asignar memoria para nueva cola de hilos");
            exit(EXIT_FAILURE);
        }
        nueva_cola->nombre_estado = READY;
        nueva_cola->lista_hilos = list_create();
        if (nueva_cola->lista_hilos == NULL) {
            perror("Error al crear lista de hilos en la nueva cola");
            exit(EXIT_FAILURE);
        }

        // Crear un nuevo nivel de prioridad y asociarlo con la nueva cola de hilos
        t_nivel_prioridad* nuevo_nivel = malloc(sizeof(t_nivel_prioridad));
        if (nuevo_nivel == NULL) {
            perror("Error al asignar memoria para nuevo nivel de prioridad");
            exit(EXIT_FAILURE);
        }

        nuevo_nivel->nivel_prioridad = prioridad;
        nuevo_nivel->cola_hilos = nueva_cola;
        // Agregar el nuevo nivel de prioridad a la lista de niveles
        pthread_mutex_lock(mutex_colas_multinivel); 
        list_add(colas_multinivel->niveles_prioridad, nuevo_nivel);
        printf("Se agrega la cola de nueva prioridad");
        // Encolar el hilo en la nueva cola
        list_add(nueva_cola->lista_hilos, hilo);   
        printf("Se agrega el hilo a su cola");    
        pthread_mutex_unlock(mutex_colas_multinivel); 

        // Señalizar que hay un hilo en la nueva cola
        sem_post(sem_estado_multinivel);    
        printf("Hilo de prioridad %d encolado en la nueva cola\n", prioridad);
    } else {
        // Si ya existe el nivel, encolamos el hilo en la cola correspondiente
        pthread_mutex_lock(mutex_colas_multinivel); 
        list_add(nivel->cola_hilos->lista_hilos, hilo);       
        pthread_mutex_unlock(mutex_colas_multinivel); 

        // Señalizar que hay un hilo en la cola de este nivel
        if (sem_estado_multinivel != NULL) {
            sem_post(sem_estado_multinivel);
            } else {
                fprintf(stderr, "Error: semáforo no inicializado 2.\n");
            }
        sem_post(sem_estado_multinivel);
        printf("Hilo de prioridad %d encolado en la cola existente\n", prioridad);
    }
}

bool nivel_existe(void* elemento) {
    t_nivel_prioridad* nivel = (t_nivel_prioridad*)elemento;
    // Compara el nivel de prioridad del elemento con la prioridad del hilo actual
    return nivel->nivel_prioridad == hilo_actual->prioridad;
}

/*
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

/*Pruebas Version 2, CMN con un hilo*/

void obtener_planificador_corto_plazo() {
    algoritmo = "CMN";  // Cambiar este valor entre "FIFO", "PRIORIDADES", y "CMN" para probar

    if (strcmp(algoritmo, "FIFO") == 0) {
        corto_plazo_fifo();
    } else if (strcmp(algoritmo, "PRIORIDADES") == 0) {
        corto_plazo_prioridades();
    } else if (strcmp(algoritmo, "CMN") == 0) {
        corto_plazo_colas_multinivel();
    } else {
        printf("Error: Algoritmo no reconocido.\n");
    }
}

t_cola_hilo* inicializar_cola_hilo(t_estado estado) {
    t_cola_hilo *cola = malloc(sizeof(t_cola_hilo));
    if (cola == NULL) {
        perror("Error al asignar memoria para cola de hilos");
        exit(EXIT_FAILURE);
    }

    cola->nombre_estado = estado;
    cola->lista_hilos = list_create();  // Crea una lista vacía
    if (cola->lista_hilos == NULL) {
        perror("Error al crear lista de hilos");
        exit(EXIT_FAILURE);
    }
}

void inicializar_semaforos_corto_plazo(){
    mutex_hilos_cola_ready = malloc(sizeof(pthread_mutex_t));
    if (mutex_hilos_cola_ready == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_hilos_cola_ready, NULL);

    mutex_hilos_cola_bloqueados = malloc(sizeof(pthread_mutex_t));
    if (mutex_hilos_cola_bloqueados == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_hilos_cola_bloqueados, NULL);
    
    mutex_colas_multinivel = malloc(sizeof(pthread_mutex_t));
    
    if (mutex_colas_multinivel == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_colas_multinivel, NULL);

    sem_estado_hilos_cola_ready = malloc(sizeof(sem_t));
    if (sem_estado_hilos_cola_ready == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_estado_hilos_cola_ready, 0, 0);

    sem_estado_hilos_cola_bloqueados = malloc(sizeof(sem_t));
    if (sem_estado_hilos_cola_bloqueados == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_estado_hilos_cola_bloqueados, 0, 0);

    sem_estado_multinivel = malloc(sizeof(sem_t));
    if (sem_estado_multinivel == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_estado_multinivel, 0, 0);
}

t_colas_multinivel* inicializar_colas_multinivel() {
    t_colas_multinivel *colas_multinivel = malloc(sizeof(t_colas_multinivel));
    if (colas_multinivel == NULL) {
        perror("Error al asignar memoria para colas multinivel");
        exit(EXIT_FAILURE);
    }
    colas_multinivel->niveles_prioridad = list_create();
    if (colas_multinivel->niveles_prioridad == NULL) {
        perror("Error al crear lista de niveles de prioridad");
        exit(EXIT_FAILURE);
    }
    return colas_multinivel;
}

void* planificador_corto_plazo_hilo(void* arg) {
    printf("Iniciando Planificador de Colas Multinivel en segundo plano...\n");
    // Ejecutar el planificador (queda esperando a que haya hilos encolados)
    corto_plazo_colas_multinivel();
    
    return NULL;
}

int main() {
    // Inicializar las colas de planificación
    printf("Iniciando estructuras, semáforos\n");
    inicializar_semaforos_corto_plazo();

    printf("PRIMEROS VALORES DE LOS SEMÁFOROS\n");
    printf("Dirección del mutex_multinivel: %p\n", (void*)&mutex_colas_multinivel);
    printf("Dirección del mutex_bloqueados: %p\n", (void*)&mutex_hilos_cola_bloqueados);
    printf("Dirección del mutex_ready: %p\n", (void*)&mutex_hilos_cola_ready);

    printf("Dirección del semáforo_mutinivel: %p\n", (void*)&sem_estado_multinivel);
    printf("Dirección del semáforo_bloqueados: %p\n", (void*)&sem_estado_hilos_cola_bloqueados);
    printf("Dirección del semáforo_ready: %p\n", (void*)&sem_estado_hilos_cola_ready);

    printf("Iniciando Planificador Colas Multinivel:\n");
    sleep(1);
    printf("Iniciando Cola de Ready:\n");
    hilos_cola_ready = inicializar_cola_hilo(READY);
    sleep(1);
    printf("Iniciando Cola de Block:\n");
    hilos_cola_bloqueados = inicializar_cola_hilo(BLOCK);
    sleep(1);
    printf("Iniciando Colas Multinivel:\n");
    colas_multinivel = inicializar_colas_multinivel();  
    
    printf("Creando el hilo para el Planificador Colas Multinivel:\n");
    pthread_t hilo_planificador;
    pthread_create(&hilo_planificador, NULL, planificador_corto_plazo_hilo, NULL);
    sleep(1);

    // Crear y encolar varios hilos de distintas prioridades
    printf("Encolando hilos con diferentes prioridades...\n");

    // Hilo 1: Prioridad baja
    t_tcb* hilo_1 = malloc(sizeof(t_tcb));
    hilo_1->tid = 1;
    hilo_1->prioridad = 3;  // Prioridad baja
    encolar_corto_plazo_multinivel(hilo_1);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_1->tid, hilo_1->prioridad);
    sleep(5);

    // Hilo 2: Prioridad alta
    t_tcb* hilo_2 = malloc(sizeof(t_tcb));
    hilo_2->tid = 2;
    hilo_2->prioridad = 0;  // Prioridad alta
    encolar_corto_plazo_multinivel(hilo_2);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_2->tid, hilo_2->prioridad);
    sleep(10);

    // Hilo 3: Prioridad media
    t_tcb* hilo_3 = malloc(sizeof(t_tcb));
    hilo_3->tid = 3;
    hilo_3->prioridad = 1;  // Prioridad media
    encolar_corto_plazo_multinivel(hilo_3);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_3->tid, hilo_3->prioridad);
    sleep(2);
    // Hilo 4: Prioridad baja
    t_tcb* hilo_4 = malloc(sizeof(t_tcb));
    hilo_4->tid = 4;
    hilo_4->prioridad = 2;  // Prioridad baja
    encolar_corto_plazo_multinivel(hilo_4);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_4->tid, hilo_4->prioridad);
    sleep(3);
    // Hilo 5: Prioridad alta
    t_tcb* hilo_5 = malloc(sizeof(t_tcb));
    hilo_5->tid = 5;
    hilo_5->prioridad = 0;  // Prioridad alta
    encolar_corto_plazo_multinivel(hilo_5);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_5->tid, hilo_5->prioridad);
    sleep(2);
    // Hilo 6: Prioridad media
    t_tcb* hilo_6 = malloc(sizeof(t_tcb));
    hilo_6->tid = 6;
    hilo_6->prioridad = 1;  // Prioridad media
    encolar_corto_plazo_multinivel(hilo_6);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_6->tid, hilo_6->prioridad);
    sleep(1);
    // Hilo 7: Prioridad baja
    t_tcb* hilo_7 = malloc(sizeof(t_tcb));
    hilo_7->tid = 7;
    hilo_7->prioridad = 3;  // Prioridad baja
    encolar_corto_plazo_multinivel(hilo_7);
    printf("Hilo TID: %d, Prioridad: %d encolado.\n", hilo_7->tid, hilo_7->prioridad);

    // Esperar un poco para que el planificador procese los hilos
    sleep(5);

    // Asegurarse de que el hilo del planificador termine su ejecución
    pthread_join(hilo_planificador, NULL);

    return 0;
}
