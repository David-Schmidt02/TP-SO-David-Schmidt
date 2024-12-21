#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <instrucciones.h>
#include "../../kernel/src/pcb.h"


void inicializar_estructuras_cpu();
void *conexion_kernel_dispatch(void * arg_kernelD);
void *conexion_kernel_interrupt(void* arg_kernelI);
void *cliente_conexion_memoria(void * arg_memoria);
void *ciclo_instruccion(void* args);


