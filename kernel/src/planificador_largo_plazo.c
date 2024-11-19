#include <planificador_largo_plazo.h>

t_cola_proceso* procesos_cola_ready;
pthread_mutex_t * mutex_procesos_cola_ready;
sem_t * sem_estado_procesos_cola_ready;

t_cola_procesos_a_crear* procesos_a_crear;
pthread_mutex_t * mutex_procesos_a_crear;
sem_t * sem_estado_procesos_a_crear;

void inicializar_semaforos_largo_plazo(){
    mutex_procesos_cola_ready = malloc(sizeof(pthread_mutex_t));
    if (mutex_procesos_cola_ready == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_procesos_cola_ready, NULL);

    mutex_procesos_a_crear = malloc(sizeof(pthread_mutex_t));
    if (mutex_procesos_a_crear == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_procesos_a_crear, NULL);
    
    sem_estado_procesos_cola_ready = malloc(sizeof(sem_t));
    if (sem_estado_procesos_cola_ready == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_estado_procesos_cola_ready, 0, 0);

    sem_estado_procesos_a_crear = malloc(sizeof(sem_t));
    if (sem_estado_procesos_a_crear == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_estado_procesos_a_crear, 0, 0);
}

void largo_plazo_fifo()
{
    while (1)
    {
        sem_wait(procesos_a_crear);
        pthread_mutex_lock(mutex_procesos_a_crear);
        t_pcb *proceso = desencolar_proceso_a_crear();
        //crear peticion a memoria
        //esperar respuesta de peticion desde memoria
        //if respuesta positiva
            proceso->estado = READY; 
            encolar_proceso_en_ready(proceso);
            encolar_corto_plazo(proceso);
            pthread_mutex_unlock(mutex_procesos_a_crear);
        //else respuesta negativa

    }
}

t_pcb* desencolar_proceso_a_crear(){
    t_pcb *proceso= list_remove(hilos_cola_ready->lista_hilos, 0);
    return proceso;
}
void encolar_proceso_en_ready(t_pcb * proceso){
    pthread_mutex_lock(mutex_procesos_cola_ready);
    list_add(procesos_cola_ready->lista_procesos, proceso);
    pthread_mutex_unlock(mutex_procesos_cola_ready);
    sem_post(sem_estado_procesos_cola_ready);
}

void encolar_corto_plazo(t_pcb * proceso){
    char *algoritmo = "FIFO";
    //obtener el hilo y mandarlo a encolar
    if (strcmp(algoritmo, "FIFO") == 0) {
        encolar_corto_plazo_fifo();
    } else if (strcmp(algoritmo, "PRIORIDADES") == 0) {
        encolar_corto_plazo_prioridades();
    } else if (strcmp(algoritmo, "CMN") == 0) {
        encolar_corto_plazo_colas_multinivel();
    } else {
        printf("Error: Algoritmo no reconocido.\n");
    }
}

/*
Planificador Largo Plazo
    -> Creación de procesos
        -> Cola new administrada por FIFO 
        -> Si la cola está llena porque memoria no tiene más espacio se espera a que un proceso finalice
    -> Finalizacion de procesos
        -> Se le avisa a memoria (que confirma), se libera el PCB asociado (donde se almacenan?) y 
        se intenta inicializar otro proceso
    -> Creación de hilos
        -> Kernel avisa a memoria y lo ingresa a la cola de Ready según su prioridad 
        (se una lista global de TCBs ordenada por prioridades)
    -> Finalizar hilos
        -> Se le informa a memoria se libera el TCB asociado (se tiene una lista global de TCBs) 
            y se desbloquean todos los hilos bloqueados por este hilo
            ahora finalizado
*/
