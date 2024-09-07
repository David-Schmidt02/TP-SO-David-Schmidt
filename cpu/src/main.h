#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
typedef enum  
{
    SET,
    SUB,
    SUM,
    JNZ,
    READ_MEM,
    WRITE_MEM,
    LOG
}t_operaciones; // 

void *conexion_kernel_dispatch(void * arg_kernelD);
void *conexion_kernel_interrupt(void* arg_kernelI);
void *cliente_conexion_memoria(void * arg_memoria);

void levantar_conexiones();