#ifndef MAIN_H
#define MAIN_H

#include <pcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
typedef struct 
{
    protocolo_socket tipo;
    t_pcb *proceso;
    t_tcb *hilo;
    bool respuesta_recibida;
    bool respuesta_exitosa;
}t_peticion;

typedef struct
{
    int socket;
    t_peticion *peticion;
}t_paquete_peticion;


typedef enum  
{
    CREATE_PROCESS,
    EXIT_PROCESS,
    CREATE_THREAD,
    EXIT_THREAD,
    DUMP_MEMORY
}protocolo_peticion;

typedef struct 
{
    protocolo_peticion tipo;
    t_pcb *proceso;
    t_tcb *hilo;
    bool respuesta_recibida;
    bool respuesta_exitosa;
}t_peticion;

typedef struct
{
    int socket;
    t_peticion *peticion;
}t_paquete_peticion;


typedef struct{
    int tid;
    int milisegundos;
}t_uso_io;

typedef struct
{
    t_list *lista_io;
}t_cola_IO;

void *conexion_memoria(void * arg_memoria);
void *conexion_cpu_dispatch(void * arg_cpu_dispatch);
void *conexion_cpu_interrupt(void * arg_cpu_interrupt);
void *administrador_peticiones_memoria(void* arg_server);
void *peticion_kernel(void * args);
void encolar_peticion_memoria(t_peticion * peticion);
void inicializar_estructuras();
void inicializar_semaforos();
void inicializar_semaforos_conexion_cpu();
void inicializar_semaforos_peticiones();
void inicializar_colas_largo_plazo();
void inicializar_colas_corto_plazo();
//void levantar_conexiones();

#endif