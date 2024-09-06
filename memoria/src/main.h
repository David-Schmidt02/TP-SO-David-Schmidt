#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>

void *conexion_kernel(void * arg_kernel);
void *conexion_cpu(void * arg_cpu);
void *cliente_conexion_filesystem(void * arg_fs);

void levantar_conexiones();