#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>

typedef struct{
    int tid;
    int milisegundos;
}t_uso_io;

typedef struct
{
    t_list *lista_io;
    pthread_mutex_t *mutex_estado;
    sem_t *sem_estado;
}t_cola_IO;

void *conexion_memoria(void * arg_memoria);
void *conexion_cpu_dispatch(void * arg_cpu_dispatch);
void *conexion_cpu_interrupt(void * arg_cpu_interrupt);

void levantar_conexiones();