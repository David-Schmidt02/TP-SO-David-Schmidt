#ifndef PLANIFICADOR_LARGO_PLAZO_H
#define PLANIFICADOR_LARGO_PLAZO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <pthread.h>
#include <semaphore.h>
#include <pcb.h>
#include <syscall.h>

//La cola de procesos que llegan a New
typedef struct
{
    t_estado nombre_estado;
    t_list *lista_procesos;
} t_cola_proceso;

typedef struct {
    t_estado nombre_estado;
    t_list *lista_procesos;  // Lista de procesos a crear
} t_cola_procesos_a_crear;

t_cola_proceso* inicializar_cola_procesos_ready();
void encolar_hilo_principal_corto_plazo(t_pcb * proceso);
void inicializar_cola_procesos_a_crear();
void inicializar_semaforos_largo_plazo();
t_pcb* desencolar_proceso_a_crear();
void encolar_proceso_en_ready(t_pcb * proceso);
void* largo_plazo_fifo(void* args);


void* planificador_corto_plazo_hilo(void* arg);
void *simular_respuesta_memoria(void *arg);


#endif