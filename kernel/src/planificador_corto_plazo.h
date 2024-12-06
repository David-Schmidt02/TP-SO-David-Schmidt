#ifndef PLANIFICADOR_CORTO_PLAZO_H
#define PLANIFICADOR_CORTO_PLAZO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include "utils/utils.h"
#include <pthread.h>
#include <semaphore.h>
#include <pcb.h>

// motivos por el cual el cpu devuelve el tid al kernel

/* Se agregaron a protocolo_socket
typedef enum
{
    FINALIZACION,
    FIN_QUANTUM,
    THREAD_JOIN_OP
}t_motivo;
*/
// la cola de hilos de un estado (Ready, Blocked, Exec)
// por cada cola se tiene un semáforo que indica su estado y un mutex para garantizar mutua exclusión
typedef struct
{
    t_estado nombre_estado;
    t_list *lista_hilos;
} t_cola_hilo;

//estructura para las listas del algoritmo multinivel, con su cola de hilos y nivel de prioridad
typedef struct {
    int nivel_prioridad;
    t_cola_hilo *cola_hilos; // Cola de hilos para esta prioridad
} t_nivel_prioridad;

// Estructura principal de colas multinivel, permitiría agregar nuevas colas de distintas prioridades
typedef struct {
    t_list *niveles_prioridad; // Lista de niveles de prioridad
} t_colas_multinivel;


void* planificador_corto_plazo_hilo(void* arg);
void encolar_hilo_corto_plazo(t_tcb * hilo);
void corto_plazo_fifo();
void encolar_corto_plazo_fifo(t_tcb * hilo);
t_tcb* desencolar_hilos_fifo();
void corto_plazo_prioridades();
void encolar_corto_plazo_prioridades(t_tcb * hilo);
t_tcb* desencolar_hilos_prioridades();
int comparar_prioridades(t_tcb *a, t_tcb *b);
void corto_plazo_colas_multinivel();
t_cola_hilo* buscar_cola_menor_prioridad(t_colas_multinivel *multinivel, t_nivel_prioridad **nivel_a_ejecutar);
void ejecutar_round_robin(t_tcb * hilo_a_ejecutar);
void contar_quantum(void *hilo_void);
void encolar_corto_plazo_multinivel(t_tcb* hilo);
bool nivel_existe_por_prioridad(void* elemento, void* contexto);
void enviar_a_cpu_dispatch(int tid, int pid);
void enviar_a_cpu_interrupt(int tid, protocolo_socket motivo);
void recibir_motivo_devolucion_cpu();
void desbloquear_hilos(int tid);
void esperar_desbloqueo_ejecutar_hilo(int tid);
t_cola_hilo* inicializar_cola_hilo(t_estado estado);
void inicializar_semaforos_corto_plazo();
t_colas_multinivel* inicializar_colas_multinivel();


#endif