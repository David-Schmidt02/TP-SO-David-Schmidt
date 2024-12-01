#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"
#include <sistema.h>

void enviar_contexto(void);
void *conexion_kernel(void * arg_kernel);
void *conexion_cpu(void * arg_cpu);
void *cliente_conexion_filesystem(void * arg_fs);
void *server_multihilo_kernel(void* arg_server);
void *peticion_kernel(void* arg_peticion);
void inicializar_memoria(int tipo_particion, int size, t_list *);
void levantar_conexiones(void);
void cargar_lista_particiones(t_list *, char **);


enum particiones{
    FIJAS,
    DINAMICAS
};

typedef struct t_memoria{
    void *espacio;
    int size;
    enum particiones tipo_particion;
    t_list *tabla_particiones_fijas;
    t_list *tabla_huecos;
    t_list *tabla_procesos;
    t_list *lista_pcb;
    t_list *lista_tcb;
}t_memoria;

typedef struct elemento_procesos{
    int TID;
    int inicio;
    int size;
}elemento_procesos;

typedef struct elemento_huecos{
    int inicio;
    int size;
}elemento_huecos;

typedef struct elemento_particiones_fijas{
    int libre_ocupado;
    uint32_t base;
    int size;
}elemento_particiones_fijas;

struct t_proceso{
    t_tcb *tcb;
    int size;
}typedef t_proceso;

#endif