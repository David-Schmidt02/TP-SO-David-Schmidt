#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <stdint.h> 
#include <utils/utils.h>
#include "pcb.h"


void element_destroyer(void* elemento);

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID);
void PROCESS_EXIT(t_tcb* tcb);

void THREAD_CREATE(FILE* archivo_instrucciones,int pid);
void THREAD_JOIN();
void THREAD_CANCEL();
void THREAD_EXIT();

void MUTEX_CREATE();
void MUTEX_LOCK();
void MUTEX_UNLOCK();

void DUMP_MEMORY();

t_list* interpretarArchivo(FILE* archivo);
void liberarInstrucciones(t_list* instrucciones);

//THREAD_JOIN
t_tcb* obtener_tcb_por_tid(int tid);
t_tcb* obtener_tcb_actual();
void agregar_hilo_a_lista_de_espera(t_tcb* hilo_a_esperar, t_tcb* hilo_actual);
t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo);

//THREAD_CANCEL


#endif