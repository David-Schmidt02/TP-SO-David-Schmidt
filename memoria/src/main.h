#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"
#include <sistema.h>
#include <time.h>
#include <sys/time.h>

typedef enum particiones{
    DINAMICAS,
    FIJAS
}particiones;

typedef enum fit{
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
}fit;

void enviar_contexto(int pid, int tid);
void *conexion_kernel(void * arg_kernel);
void *peticion_kernel_DUMP(void *arg_peticion);
void *conexion_cpu(void *arg_cpu);
void *cliente_conexion_filesystem(void * arg_fs);
void *server_multihilo_kernel(void* arg_server);
void *peticion_kernel_NEW_PROCESS(void* arg_peticion);
void *peticion_kernel_NEW_THREAD(void *arg_peticion);
void *peticion_kernel_END_PROCESS(void *arg_peticion);
void *peticion_kernel_END_THREAD(void *arg_peticion);
void inicializar_memoria(particiones tipo_particion, int size, t_list *, fit fit); // hacer para dinamicas
void levantar_conexiones(void); //
void cargar_lista_particiones(t_list *, char **); //solo para fijas

typedef struct t_memoria{
    void *espacio;
    int size;
    enum particiones tipo_particion;
    enum fit fit;
    t_list *tabla_particiones_fijas;
    t_list *tabla_huecos;
    t_list *tabla_procesos;
    t_list *lista_pcb;
    t_list *lista_tcb;
    
}t_memoria;

typedef struct elemento_procesos{
    int pid;
    uint32_t inicio;
    int size;
}elemento_procesos;

typedef struct elemento_huecos{
    uint32_t inicio;
    int size;
}elemento_huecos;

typedef struct elemento_particiones_fijas{
    int libre_ocupado;
    uint32_t base;
    int size;
}elemento_particiones_fijas;

#endif