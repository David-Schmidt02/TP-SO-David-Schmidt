#ifndef H_CORTO_PLAZO_
#define H_CORTO_PLAZO_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <pthread.h>
#include <semaphore.h>
#include <main.h>
#include <pcb.h>

// permite identificar las colas de distintos estados por los nombres
// Variables del config

extern char* ALGORITMO_PLANIFICACION;
extern int QUANTUM;
extern char* LOG_LEVEL;

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
    t_estado nombre_estado;
    t_list *lista_hilos;
    pthread_mutex_t *mutex_estado;
    sem_t *sem_estado;
} t_cola_hilo;

// la cola de procesos que llegan a New
typedef struct
{
    t_estado nombre_estado;
    t_list *lista_procesos;
    pthread_mutex_t *mutex_estado;
    sem_t *sem_estado;
} t_cola_proceso;

//estructura para las listas del algoritmo multinivel, con su cola de hilos y nivel de prioridad
typedef struct {
    int nivel_prioridad;
    t_cola_hilo *cola_hilos; // Cola de hilos para esta prioridad
    int quantum; // Por si el quantum es distinto para cada cola
} t_nivel_prioridad;

// Estructura principal de colas multinivel, permitiría agregar nuevas colas de distintas prioridades
typedef struct {
    t_list *niveles_prioridad; // Lista de niveles de prioridad
} t_colas_multinivel;

void obtener_planificador_corto_plazo();
void corto_plazo_fifo(t_cola_hilo *cola_ready);
void corto_plazo_prioridades(t_cola_hilo *cola_ready);
int comparar_prioridades(t_tcb *a, t_tcb *b);
void inicializar_colas_multinivel(t_colas_multinivel *multinivel);
t_cola_hilo *obtener_o_crear_cola(t_colas_multinivel *multinivel, int prioridad);
void encolar_hilo_multinivel(t_colas_multinivel *multinivel, t_tcb *hilo);
void corto_plazo_colas_multinivel(t_colas_multinivel *multinivel);
void *comparar_prioridades_colas(t_nivel_prioridad *a, t_nivel_prioridad *b);
void ejecutar_round_robin(t_cola_hilo *cola, int nivel_prioridad);
void enviar_a_cpu_dispatch(int tid, int pid);
void recibir_motivo_devolucion();

#endif