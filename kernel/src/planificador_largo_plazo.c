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

extern t_cola_procesos_a_crear* lista_procesos_a_crear_reintento;
extern pthread_mutex_t * mutex_lista_procesos_a_crear_reintento;
extern sem_t * sem_estado_lista_procesos_a_crear_reintento;

extern t_list * lista_t_peticiones;
extern pthread_mutex_t * mutex_lista_t_peticiones;
extern sem_t * sem_lista_t_peticiones; 
extern sem_t * sem_estado_respuesta_desde_memoria;

extern sem_t * sem_hilo_actual_encolado;
extern sem_t *  sem_hilo_nuevo_encolado;
//extern pthread_mutex_t * mutex_respuesta_desde_memoria;
//extern pthread_cond_t * cond_respuesta_desde_memoria;

extern int conexion_kernel_cpu_dispatch;
extern int conexion_kernel_cpu_interrupt;

extern sem_t * sem_proceso_finalizado;

extern char* algoritmo;

extern pthread_mutex_t * mutex_socket_memoria;

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

void inicializar_cola_procesos_a_crear(){
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
}

void inicializar_semaforos_largo_plazo(){

    mutex_lista_procesos_a_crear_reintento = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_lista_procesos_a_crear_reintento, NULL);

    mutex_procesos_cola_ready = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_procesos_cola_ready, NULL);

    mutex_procesos_a_crear = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_procesos_a_crear, NULL);

    sem_estado_procesos_cola_ready = malloc(sizeof(sem_t));
    sem_init(sem_estado_procesos_cola_ready, 0, 0);

    sem_estado_procesos_a_crear = malloc(sizeof(sem_t));
    sem_init(sem_estado_procesos_a_crear, 0, 0);

    sem_proceso_finalizado = malloc(sizeof(sem_t));
    sem_init(sem_proceso_finalizado, 0, 0);

    sem_hilo_actual_encolado = malloc(sizeof(sem_t));
    sem_init(sem_hilo_actual_encolado, 0, 0);

    sem_hilo_nuevo_encolado = malloc(sizeof(sem_t));
    sem_init(sem_hilo_nuevo_encolado, 0, 0);

    sem_estado_lista_procesos_a_crear_reintento = malloc(sizeof(sem_t));
    sem_init(sem_estado_lista_procesos_a_crear_reintento, 0, 0);
}

void* largo_plazo_fifo(void* args){
    while (1)
    {
        sem_wait(sem_estado_procesos_a_crear);
        log_info(logger, "Llego otro proceso a crear, generando la peticion para memoria...");
        pthread_mutex_lock(mutex_procesos_a_crear);
        t_pcb *proceso = desencolar_proceso_a_crear();
        
    // Crear petición
        t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = PROCESS_CREATE_OP;
        peticion->proceso = proceso;
        peticion->hilo = NULL;
        pthread_mutex_lock(mutex_socket_memoria);
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
        if (peticion->respuesta_exitosa) {
            proceso->estado = READY;
            encolar_proceso_en_ready(proceso);
            encolar_hilo_principal_corto_plazo(proceso);
            sem_post(sem_hilo_nuevo_encolado);
            pthread_mutex_unlock(mutex_socket_memoria);
            pthread_mutex_unlock(mutex_procesos_a_crear);
        } 
        else {
            pthread_mutex_unlock(mutex_socket_memoria);
            pthread_mutex_unlock(mutex_procesos_a_crear);
            log_warning(logger, "No se pudo crear el proceso, reitentando cuando otro proceso finalice...");
            sem_post(sem_hilo_nuevo_encolado);
            list_add(lista_procesos_a_crear_reintento->lista_procesos, proceso);
            sem_post(sem_estado_lista_procesos_a_crear_reintento);
            //reintentar_creacion_proceso(proceso);
            }
        free(peticion);
    }
}

t_pcb* desencolar_proceso_a_crear(){
    t_pcb *proceso = list_remove(procesos_a_crear->lista_procesos, 0);
    return proceso;
}

void encolar_proceso_en_ready(t_pcb * proceso){
    pthread_mutex_lock(mutex_procesos_cola_ready);
    list_add(procesos_cola_ready->lista_procesos, proceso);
    pthread_mutex_unlock(mutex_procesos_cola_ready);
    sem_post(sem_estado_procesos_cola_ready);
}

void encolar_hilo_principal_corto_plazo(t_pcb * proceso){
    t_tcb * hilo = list_get(proceso->listaTCB, 0);
    t_peticion *peticion = malloc(sizeof(t_peticion));
    peticion->tipo = THREAD_CREATE_OP;
    peticion->proceso = NULL;
    peticion->hilo = hilo;
    encolar_peticion_memoria(peticion);
    log_error(logger, "Se encoló la peticion del hilo a memoria ...");
    sem_wait(sem_estado_respuesta_desde_memoria);
    if (hilo->pid == 1 && hilo->tid == 0)
        sem_post(sem_hilo_actual_encolado);
    if (!(hilo->pid == 1 && hilo->tid == 0))
        sem_post(sem_hilo_nuevo_encolado);
    if (strcmp(algoritmo, "FIFO") == 0) {
        encolar_corto_plazo_fifo(hilo);
    } else if (strcmp(algoritmo, "PRIORIDADES") == 0) {
        encolar_corto_plazo_prioridades(hilo);
    } else if (strcmp(algoritmo, "CMN") == 0) {
        encolar_corto_plazo_multinivel(hilo);
    } else {
        printf("Error: Algoritmo no reconocido.\n");
    }
}

