#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>

void *conexion_memoria(void * arg_memoria);
void *conexion_cpu_dispatch(void * arg_cpu_dispatch);
void *conexion_cpu_interrupt(void * arg_cpu_interrupt);

void levantar_conexiones();