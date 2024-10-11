#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <pthread.h>
#include <semaphore.h>

// permite identificar las colas de distintos estados por los nombres
typedef enum
{
    NEW,
    READY,
    EXEC,
    BLOCKED,
    EXIT
} t_id_estado;

// motivos por el cual el cpu devuelve el tid al kernel
typedef enum
{
    FINALIZACION,
    FIN_QUANTUM,
    SEGMENTATION_FAULT // no entiendo como funcionaría
}motivo;

// la cola de hilos de un estado (Ready, Blocked, Exec)
// por cada cola se tiene un semáforo que indica su estado y un mutex para garantizar mutua exclusión
typedef struct
{
    t_id_estado nombre_estado;
    t_list *lista_hilos;
    pthread_mutex_t *mutex_estado;
    sem_t *sem_estado;
} t_cola_hilo;


// la cola de procesos que llegan a New
typedef struct
{
    t_id_estado nombre_estado = NEW;
    t_list *lista_procesos;
    pthread_mutex_t *mutex_estado;
    sem_t *sem_estado;
} t_cola_proceso;

void *obtener_planificador_corto_plazo(void *);