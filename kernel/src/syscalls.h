#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <stdint.h> 
#include <utils/utils.h>
#include "pcb.h"
#include <main.h>

// Códigos de operación
//#define HANDSHAKE 1
//#define PROCESS_CREATE_OP 2
//#define FIN_HILO 3
//#define THREAD_CREATE_OP 4
//#define THREAD_JOIN_OP 5
//#define THREAD_EXIT_OP 6
//
//#define MUTEX_CREATE_OP 7
//#define MUTEX_LOCK_OP 8
//#define MUTEX_UNLOCK_OP 9
//
//#define DUMP_MEMORY_OP 10


extern t_list* lista_mutexes;
extern int ultimo_tid;
/*
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

// Funciones de gestión de hilos y procesos
t_tcb* obtener_tcb_por_tid(int tid);
t_tcb* obtener_tcb_actual();
void agregar_hilo_a_lista_de_espera(t_tcb* hilo_a_esperar, t_tcb* hilo_actual);
t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo);

// Funciones para gestionar la finalización de hilos
void notificar_memoria_fin_hilo(t_tcb* hilo);

void encolar_peticion_memoria(t_peticion * peticion);

void eliminar_tcb(t_tcb* hilo);
*/

t_pcb* obtener_pcb(int pid);
t_tcb* obtener_tcb(int tid, int pid);
void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso, int prioridadTID);
void eliminar_mutex(t_mutex* mutex);
void eliminar_pcb(t_pcb* pcb);
void PROCESS_EXIT();
void notificar_memoria_fin_proceso(int pid);
void eliminar_hilo_de_cola_fifo_prioridades_thread_exit(t_tcb* tcb_asociado);
void eliminar_hilo_de_cola_fifo_prioridades_thread_cancel(t_tcb* tcb_asociado);
void eliminar_hilo_de_cola_multinivel_thread_exit(t_tcb* tcb_asociado);
void eliminar_hilo_de_cola_multinivel_cancel(t_tcb* tcb_asociado);
void encolar_en_exit(t_tcb * hilo);
void encolar_en_block(t_tcb * hilo);
void THREAD_CREATE(FILE* archivo_instrucciones, int prioridad);
void THREAD_JOIN(int tid_a_esperar) ;
void finalizar_hilo(t_tcb* hilo);
void THREAD_CANCEL(int tid_hilo_a_cancelar);
void notificar_memoria_creacion_hilo(t_tcb* hilo);
void notificar_memoria_fin_hilo(t_tcb* hilo);
void eliminar_tcb(t_tcb* hilo);
void THREAD_EXIT();
void IO(float milisec, int tcb_id);
void MUTEX_CREATE(char* nombre_mutex);
void MUTEX_LOCK(char* nombre_mutex);
void MUTEX_UNLOCK(char* nombre_mutex);
void DUMP_MEMORY(int pid);
void element_destroyer(void* elemento);
t_list* interpretarArchivo(FILE* archivo);
void liberarInstrucciones(t_list* instrucciones);
t_tcb* obtener_tcb_lista(t_list * lista, int tid, int pid);
t_tcb* obtener_tcb_actual();
void agregar_hilo_a_lista_de_espera(t_tcb* hilo_a_esperar, t_tcb* hilo_actual);
t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo);



#endif