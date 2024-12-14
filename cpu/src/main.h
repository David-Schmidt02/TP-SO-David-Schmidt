#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <instrucciones.h>
#include "../../kernel/src/pcb.h"
typedef enum  
{
    SET,
    SUB,
    SUM,
    JNZ,
    LOG
}t_operaciones;  

void inicializar_estructuras_cpu();
void *conexion_kernel_dispatch(void * arg_kernelD);
void *conexion_kernel_interrupt(void* arg_kernelI);
void *cliente_conexion_memoria(void * arg_memoria);

void levantar_conexiones();

/*
SET AX 1
SET BX 1
SET PC 5
SUM AX BX
SUB AX BX 
READ_MEM AX BX
WRITE_MEM AX BX
JNZ AX BX
LOG AX
MUTEX_CREATE RECURSO_1
MUTEX_LOCK RECURSO_1
MUTEX_UNLOCK RECURSO_1
DUMP_MEMORY
IO 1500
PROCESS_CREATE PROCESO_1 256 1
THREAD_CREATE HILO_1 3
THREAD_CANCEL 1
THREAD_JOIN 1
THREAD_EXIT 
PROCESS_EXIT
*/