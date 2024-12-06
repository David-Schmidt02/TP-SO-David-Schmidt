#include <planificador_largo_plazo.h>
#include <planificador_corto_plazo.h>
#include <utils/utils.h>
#include <main.h>
#include <syscalls.h>

extern t_cola_proceso* procesos_cola_ready;
extern pthread_mutex_t * mutex_procesos_cola_ready;
extern sem_t * sem_estado_procesos_cola_ready;

extern t_cola_procesos_a_crear* procesos_a_crear;
extern pthread_mutex_t * mutex_procesos_a_crear;
extern sem_t * sem_estado_procesos_a_crear;

extern t_list * lista_t_peticiones;
extern pthread_mutex_t * mutex_lista_t_peticiones;
extern sem_t * sem_lista_t_peticiones; 
extern sem_t * sem_estado_respuesta_desde_memoria;

//extern pthread_mutex_t * mutex_respuesta_desde_memoria;
//extern pthread_cond_t * cond_respuesta_desde_memoria;

extern int conexion_kernel_cpu_dispatch;
extern int conexion_kernel_cpu_interrupt;

extern sem_t * sem_proceso_finalizado;

extern char* algoritmo;

t_cola_proceso* inicializar_cola_procesos_ready(){

    procesos_cola_ready = malloc(sizeof(t_cola_procesos_a_crear));
    if (procesos_cola_ready == NULL) {
        perror("Error al asignar memoria para cola de NEW");
        exit(EXIT_FAILURE);
    }

    procesos_cola_ready->nombre_estado = READY;
    procesos_cola_ready->lista_procesos = list_create();  // Crea una lista vacía
    if (procesos_cola_ready->lista_procesos == NULL) {
        perror("Error al crear lista de procesos en NEW");
        exit(EXIT_FAILURE);
    }
    return procesos_cola_ready;
}

t_cola_procesos_a_crear* inicializar_cola_procesos_a_crear(){
    procesos_a_crear = malloc(sizeof(t_cola_procesos_a_crear));
    if (procesos_a_crear == NULL) {
        perror("Error al asignar memoria para cola de NEW");
        exit(EXIT_FAILURE);
    }

    procesos_a_crear->nombre_estado = NEW;
    procesos_a_crear->lista_procesos = list_create();  // Crea una lista vacía
    if (procesos_a_crear->lista_procesos == NULL) {
        perror("Error al crear lista de procesos en NEW");
        exit(EXIT_FAILURE);
    }
    return procesos_a_crear;
}

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

    sem_proceso_finalizado = malloc(sizeof(sem_t));
    if (sem_proceso_finalizado == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_proceso_finalizado, 0, 0);

    log_info(logger,"COLAS DE PROCESOS EN READY/A CREAR CREADAS CORRECTAMENTE\n");
    log_info(logger,"MUTEXS DE LAS COLAS DE PROCESOS EN READY/A CREAR CREADOS CORRECTAMENTE\n");
    log_info(logger,"SEMÁFOROS DE ESTADO DE LAS COLAS DE PROCESOS EN READY/A CREAR/FINALIZADOS CORRECTAMENTE\n");
}

void largo_plazo_fifo()
{
    while (1)
    {
        log_info(logger,"Entraste al while del planificador de largo plazo\n");
        sem_wait(sem_estado_procesos_a_crear);
        log_info(logger,"Llego un procesos para crear\n");
        pthread_mutex_lock(mutex_procesos_a_crear);
        t_pcb *proceso = desencolar_proceso_a_crear();
    // Crear petición
        log_info(logger,"Se crea la peticion para memoria del proceso %d", proceso->pid);
        t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = PROCESS_CREATE_OP;
        peticion->proceso = proceso;
        peticion->hilo = NULL; // No aplica en este caso
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
        if (peticion->respuesta_exitosa) {
            log_info(logger,"Memoria respondió con éxito la peticion, se encola en ready el proceso %d", proceso->pid);
            proceso->estado = READY;
            encolar_proceso_en_ready(proceso);
            log_info(logger,"Se encola el hilo principal del proceso %d", proceso->pid);
            encolar_hilo_principal_corto_plazo(proceso);
            pthread_mutex_unlock(mutex_procesos_a_crear);
        } else {
            log_info(logger, "Memoria no tiene espacio suficiente, reintentando cuando un proceso se termine...");
            //probar como funciona
            while(!peticion->respuesta_exitosa){
                sem_wait(sem_proceso_finalizado);
                pthread_mutex_lock(mutex_procesos_a_crear);
                list_add_in_index(procesos_a_crear->lista_procesos, 0, proceso);
                pthread_mutex_unlock(mutex_procesos_a_crear);
                sem_post(sem_estado_procesos_a_crear);
            }
        }
        free(peticion);
        /*
        int aux;
        int val = sem_getvalue(sem_estado_procesos_a_crear,aux);
        if(val == 0){
            t_paquete* send_terminate = crear_paquete(TERMINATE);
            enviar_paquete(send_terminate, conexion_kernel_cpu_interrupt);
            //lanzar funciones para liberar estructuras
        }
        */
    }
    
}

t_pcb* desencolar_proceso_a_crear(){
    t_pcb *proceso = list_remove(procesos_a_crear->lista_procesos, 0);
    log_info(logger,"Se desencoló el proceso %d de la lista de procesos a crear", proceso->pid);
    return proceso;
}

void encolar_proceso_en_ready(t_pcb * proceso){
	log_info(logger,"Se encola el proceso con pid %d a la cola de READY", proceso->pid);
    pthread_mutex_lock(mutex_procesos_cola_ready);
    list_add(procesos_cola_ready->lista_procesos, proceso);
    pthread_mutex_unlock(mutex_procesos_cola_ready);
    sem_post(sem_estado_procesos_cola_ready);
}

void encolar_hilo_principal_corto_plazo(t_pcb * proceso){

    t_tcb* hilo = (t_tcb*) proceso->listaTCB->head->data;
    t_peticion *peticion = malloc(sizeof(t_peticion));
    peticion->tipo = THREAD_CREATE_OP;
    peticion->proceso = NULL;
    peticion->hilo = hilo;
    sem_wait(sem_estado_respuesta_desde_memoria);
    encolar_peticion_memoria(peticion);
    if (strcmp(algoritmo, "FIFO") == 0) {
        log_info(logger, "Se encola el hilo según el algoritmo de FIFO");
        encolar_corto_plazo_fifo(hilo);
    } else if (strcmp(algoritmo, "PRIORIDADES") == 0) {
        log_info(logger, "Se encola el hilo según el algoritmo de Prioridades");
        encolar_corto_plazo_prioridades(hilo);
    } else if (strcmp(algoritmo, "CMN") == 0) {
        log_info(logger, "Se encola el hilo según el algoritmo de Colas Multinivel");
        encolar_corto_plazo_multinivel(hilo);
    } else {
        printf("Error: Algoritmo no reconocido.\n");
    }
}

/*
Planificador Largo Plazo
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
