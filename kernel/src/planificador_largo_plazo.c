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

extern sem_t * sem_hilo_principal_process_create_encolado;
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

    sem_hilo_principal_process_create_encolado = malloc(sizeof(sem_t));
    if (sem_hilo_principal_process_create_encolado == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_hilo_principal_process_create_encolado, 0, 0);
}

void largo_plazo_fifo()
{
    while (1)
    {
        sem_wait(sem_estado_procesos_a_crear);
        pthread_mutex_lock(mutex_procesos_a_crear);
        t_pcb *proceso = desencolar_proceso_a_crear();
    // Crear petición
        t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = PROCESS_CREATE_OP;
        peticion->proceso = proceso;
        peticion->hilo = NULL; // No aplica en este caso
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
        if (peticion->respuesta_exitosa) {
            proceso->estado = READY;
            encolar_proceso_en_ready(proceso);
            encolar_hilo_principal_corto_plazo(proceso);
            if (proceso->pid != 1)
            {
                sem_post(sem_hilo_principal_process_create_encolado);
            }
            pthread_mutex_unlock(mutex_procesos_a_crear);
        } else {
            log_warning(logger, "Memoria no tiene espacio suficiente, reintentando cuando un proceso se termine...");
            //probar como funciona
            while(!peticion->respuesta_exitosa){
                pthread_mutex_unlock(mutex_procesos_a_crear);
                sem_post(sem_hilo_principal_process_create_encolado);
                sem_wait(sem_proceso_finalizado);
                pthread_mutex_lock(mutex_procesos_a_crear);
                list_add_in_index(procesos_a_crear->lista_procesos, 0, proceso);
                pthread_mutex_unlock(mutex_procesos_a_crear);
                sem_post(sem_estado_procesos_a_crear);
            }
        }
        free(peticion);
    }
}

t_pcb* desencolar_proceso_a_crear(){
    t_pcb *proceso = list_remove(procesos_a_crear->lista_procesos, 0);
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
    t_tcb * hilo = list_get(proceso->listaTCB, 0);
    t_peticion *peticion = malloc(sizeof(t_peticion));
    peticion->tipo = THREAD_CREATE_OP;
    peticion->proceso = NULL;
    peticion->hilo = hilo;
    encolar_peticion_memoria(peticion);
    sem_wait(sem_estado_respuesta_desde_memoria);
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

