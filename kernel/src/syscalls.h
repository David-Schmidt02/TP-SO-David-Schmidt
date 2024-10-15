#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <stdint.h> 
#include <utils/utils.h>
#include "pcb.h"

// Códigos de operación
#define HANDSHAKE 1
#define PROCESS_CREATE_OP 2
#define FIN_HILO 3
#define THREAD_CREATE_OP 4
#define THREAD_JOIN_OP 5
#define THREAD_EXIT_OP 6
#define MUTEX_CREATE_OP 7
#define MUTEX_LOCK_OP 8
#define MUTEX_UNLOCK_OP 9
#define DUMP_MEMORY_OP 10
#define OK 11
#define FIN_PROCESO 12

extern t_list* lista_mutexes;
extern int ultimo_tid;

// Declaración de funciones para manejo de instrucciones
void element_destroyer(void* elemento);

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID);
void PROCESS_EXIT();

// Funciones que implementan las syscalls
void THREAD_CREATE(FILE* archivo_instrucciones, int prioridad);
void THREAD_JOIN(int tid_a_esperar);
void THREAD_CANCEL(int tid_hilo_a_cancelar);
void THREAD_EXIT();

void MUTEX_CREATE(char* nombre_mutex,t_pcb* pcb);
void MUTEX_LOCK(char* nombre_mutex, t_tcb* hilo_actual,t_pcb *pcb);
void MUTEX_UNLOCK(char* nombre_mutex,t_pcb *pcb);

void DUMP_MEMORY(int pid);

int generar_pid_unico();
t_tcb* obtener_hilo_por_tid(int tid_to_cancel);
int obtener_tid_del_paquete(t_paquete *paquete);

// Funciones de utilidad para interpretar archivos y manejar instrucciones
t_list* interpretarArchivo(FILE* archivo);
void liberarInstrucciones(t_list* instrucciones);

// Funciones de gestión de hilos
t_tcb* obtener_tcb_por_tid(int tid);
t_tcb* obtener_tcb_actual();
void agregar_hilo_a_lista_de_espera(t_tcb* hilo_a_esperar, t_tcb* hilo_actual);
t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo);

// Funciones para gestionar la finalización de hilos
void notificar_memoria_fin_hilo(t_tcb* hilo);
void eliminar_tcb(t_tcb* hilo);

#endif