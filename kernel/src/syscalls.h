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

// Declaración global de la lista de mutexes
extern t_list* lista_mutexes;

// Declaración de funciones para manejo de instrucciones
void element_destroyer(void* elemento);

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID);
void PROCESS_EXIT(t_tcb* tcb);

// Funciones que implementan las syscalls
void THREAD_CREATE(FILE* archivo_instrucciones,int pid);
void THREAD_JOIN(int tid_a_esperar);
void THREAD_CANCEL(t_paquete *paquete, int socket_cliente_kernel);
void THREAD_EXIT(int tid);

void MUTEX_CREATE(char* nombre_mutex);
void MUTEX_LOCK(char* nombre_mutex, t_tcb* hilo_actual);
void MUTEX_UNLOCK(char* nombre_mutex);

void DUMP_MEMORY();

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