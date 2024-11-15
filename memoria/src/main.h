#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>

void enviar_contexto();
void *conexion_kernel(void * arg_kernel);
void *conexion_cpu(void * arg_cpu);
void *cliente_conexion_filesystem(void * arg_fs);
void *server_multihilo_kernel(void* arg_server);
void *peticion_kernel(void* arg_peticion);
void inicializar_memoria();
void levantar_conexiones();

enum particiones{
    FIJAS,
    DINAMICAS
};

struct t_memoria{
    void *espacio;
    enum particiones tipo_particion;
    int fija_size;
    t_list *tabla_particiones_fijas;
    t_list *tabla_huecos;
    t_list *tabla_procesos;
}typedef t_memoria;

struct elemento_procesos{
    int PID;
    int inicio;
    int size;
}typedef elemento_procesos;

struct elemento_huecos{
    int inicio;
    int size;
}typedef elemento_huecos;

struct elemento_particiones_fijas{
    int libre_ocupado;
}typedef elemento_particiones_fijas;

struct t_proceso{
    char *pseudocodigo;
    int size;
}typedef t_proceso;