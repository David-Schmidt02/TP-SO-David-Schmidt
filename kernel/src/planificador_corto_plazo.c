#include "planificador_corto_plazo.h"

extern int conexion_kernel_cpu;
extern t_tcb* hilo_actual; // variable global que indica que hilo se está ejecutando actualmente, 
//va a servir para no perderlo una vez se mande a cpu y para volver a encolarlo si fue desalojado por fin de q
extern t_cola_hilo* hilos_cola_ready;
extern t_config *config;

/*
Planificador Corto Plazo
    -> Leer el archivo .config para obtener el algoritmo de planificación
        -> FIFO        V
        -> Prioridades V
        -> Multicolas
            -> Una cola por cada nivel de prioridad
            -> Entre colas prioridades sin desalojo
    -> Seleccion de hilos a ejecutar
        -> Transicionarlo al estado EXEC
        -> Mandarlo a CPU por el dispatch
        -> Esperar recibirlo con un motivo (si es necesario replanificarlo)
        -> En caso de que se necesite replanificar (por fin de q x ej) se manda a CPU por interrupt
*/
void obtener_planificador_corto_plazo()
{   //debo ver como incluir el ALGORITMO_PLANIFICACION -> desde el .h  que lo leería desde el .config
    char * algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    
    if (strcmp(algoritmo, "FIFO") == 0)
        corto_plazo_fifo(hilos_cola_ready);
    else if (strcmp(algoritmo, "PRIORIDADES") == 0)
        corto_plazo_prioridades(hilos_cola_ready);
    //else if (strcmp(algoritmo, "CMN") == 0)
    //    corto_plazo_colas_multinivel(hilos_cola_ready);
    else
    {
        // mensaje de error
    }
}

void corto_plazo_fifo(t_cola_hilo *cola_ready)
{
    while (1)
    {
        t_tcb *hilo_actual;
        // Debo esperar a tener un elemento en la lista
        sem_wait(cola_ready->sem_estado);
        // Utilizo el mutex
        sem_wait(cola_ready->mutex_estado);
        // Desencolo
        t_tcb *hilo = list_remove(cola_ready->lista_hilos, 0);
        // Debería encolarlo en una cola de EXEC o modificar su estado solamente
        hilo->estado = EXEC;  // Modificar estado a EXEC
        hilo_actual = hilo;
        sem_wait(cola_ready->mutex_estado);
        sem_post(cola_ready->sem_estado);
        // Transiciona a EXEC
        //enviar_a_cpu_dispatch(hilo);
        // Espera el motivo de la devolución
        recibir_motivo_devolucion();
    }

}

void corto_plazo_prioridades(t_cola_hilo *cola_ready)
{
        // Debo esperar a tener un elemento en la lista
        sem_wait(cola_ready->sem_estado);
        // Utilizo el mutex
        sem_wait(cola_ready->mutex_estado);
        // Ordenar la lista según prioridad (0 es la máxima prioridad)
        list_sort(cola_ready, (void *)comparar_prioridades); 
        // Desencolar el hilo con la prioridad más baja (en la cabeza de la lista)
        t_tcb *hilo; 
        //hilo = desencolar_hilo(cola_ready);
        // Transicionar el hilo a EXEC y enviarlo a CPU
        hilo->estado = EXEC;  // Modificar estado a EXEC
        hilo_actual = hilo; // Si quieres moverlo a una estructura de hilo en ejecución
        //enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        // Recibir el motivo de la devolución y actuar en consecuencia
        recibir_motivo_devolucion();
}

int comparar_prioridades(t_tcb *a, t_tcb *b) {
    return a->prioridad - b->prioridad;
}

// Inicializar las colas multinivel
void inicializar_colas_multinivel(t_colas_multinivel *multinivel) {
    multinivel->niveles_prioridad = list_create(); // Crear lista dinámica
}

// Buscar o crear una cola para la prioridad especificada
t_cola_hilo *obtener_o_crear_cola(t_colas_multinivel *multinivel, int prioridad) {
    // Buscar si ya existe una cola para esta prioridad
    for (int i = 0; i < list_size(multinivel->niveles_prioridad); i++) {
        t_nivel_prioridad *nivel = list_get(multinivel->niveles_prioridad, i);
        if (nivel->nivel_prioridad == prioridad) {
            return nivel->cola_hilos;
        }
    }

    // Si no existe, crear una nueva cola
    t_cola_hilo *nueva_cola = malloc(sizeof(t_cola_hilo));
    nueva_cola->nombre_estado = READY;
    nueva_cola->lista_hilos = list_create();
    pthread_mutex_init(&nueva_cola->mutex_estado, NULL);
    sem_init(&nueva_cola->sem_estado, 0, 0);

    // Crear un nuevo nivel de prioridad y agregarlo a la lista de prioridades
    t_nivel_prioridad *nuevo_nivel = malloc(sizeof(t_nivel_prioridad));
    nuevo_nivel->nivel_prioridad = prioridad;
    nuevo_nivel->cola_hilos = nueva_cola;

    // Insertar en la lista
    list_add(multinivel->niveles_prioridad, nuevo_nivel);

    return nueva_cola;
}

// Encolar un hilo en la cola correspondiente a su prioridad
void encolar_hilo_multinivel(t_colas_multinivel *multinivel, t_tcb *hilo) {
    int prioridad = hilo->prioridad; // Obtener la prioridad del hilo
    t_cola_hilo *cola = obtener_o_crear_cola(multinivel, prioridad); // Obtener o crear la cola
    //encolar_hilo(cola, hilo); // Encolarlo en la cola de su prioridad
}

// Función principal del planificador de colas multinivel
void corto_plazo_colas_multinivel(t_colas_multinivel *multinivel) {
    while (1) {
        // Ordenar las colas por nivel de prioridad
        list_sort(multinivel->niveles_prioridad, (void *)comparar_prioridades_colas);

        for (int i = 0; i < list_size(multinivel->niveles_prioridad); i++) {
            t_nivel_prioridad *nivel = list_get(multinivel->niveles_prioridad, i);
            t_cola_hilo *cola = nivel->cola_hilos;

            // Si la cola no está vacía, procesar el hilo
            if (!list_is_empty(cola->lista_hilos)) {
                ejecutar_round_robin(cola, nivel->nivel_prioridad);
                break; // No mirar las colas de menor prioridad
            }
        }
    }
}

// Comparar prioridades de colas (para ordenar)
void *comparar_prioridades_colas(t_nivel_prioridad *a, t_nivel_prioridad *b) {
    return a->nivel_prioridad - b->nivel_prioridad; // Ordenar por prioridad ascendente
}

// Round Robin para cada cola de prioridad
void ejecutar_round_robin(t_cola_hilo *cola, int nivel_prioridad) {
    t_tcb *hilo;
    //hilo = desencolar_hilo(cola); // Tomar el primer hilo de la cola 

    // Transicionar a EXEC y enviar a la CPU
    hilo->estado = EXEC;
    //enviar_a_cpu_dispatch(hilo->tid, hilo->pid);

    // Esperar el motivo de la devolución
    recibir_motivo_devolucion();

    // Si el hilo no finalizó, volver a encolarlo al final de la misma cola
    if (hilo->estado != EXIT) {
        //encolar_hilo(cola, hilo);
    }
}

void enviar_a_cpu_dispatch(int tid, int pid)
{
    // Crear un paquete con el tid y el pib del hilo seleccionado para ejecutar
    t_paquete * send_handshake = crear_paquete(INFO_HILO);
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); // Agregar TID
    agregar_a_paquete(send_handshake, &pid, sizeof(tid)); // Agregar PID
    // Enviamos el paquete al cpu a través de la conexion cpu dispatch
    enviar_paquete(send_handshake, conexion_kernel_cpu);
    eliminar_paquete(send_handshake); // Eliminamos el paquete 
    //Se espera la respuesta, primero el tid y luego el motivo
    recibir_motivo_devolucion();
}
void recibir_motivo_devolucion()
{
    // Recibir un paquete de respuesta desde la CPU
    t_paquete *paquete_respuesta = recibir_paquete(conexion_kernel_cpu);

    // Extraer el TID y el motivo de la devolución
    int tid, motivo;
    //obtener_de_paquete(paquete_respuesta, &tid, sizeof(int));
    //obtener_de_paquete(paquete_respuesta, &motivo, sizeof(int));

    // Tratamos el motivo:
    switch (motivo)
    {
    case FINALIZACION:
        printf("El hilo %d ha finalizado correctamente\n", tid);
        break;
    case FIN_QUANTUM:
        printf("El hilo %d fue desalojado por fin de quantum\n", tid);
        // se debe replanificar y llamar nuevamente al planificador (supongo que funciona de manera recursiva)
        //replanificar(tid); -> debo obtener nuevamente el tipo de algoritmo y volver a la funcion
        break;
    case SEGMENTATION_FAULT:
        printf("El hilo %d tuvo un segmentation fault\n", tid);
        break;
    default:
        // ? 
        break;
    }
    eliminar_paquete(paquete_respuesta);
}
