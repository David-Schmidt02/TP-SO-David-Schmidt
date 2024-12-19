#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"


typedef struct {
    protocolo_socket tipo;      
    int prioridad;                  
    char** parametro;
                        
} t_interrupcion;

typedef struct
{
    char ** texto_partido;
    protocolo_socket tipo;
    protocolo_socket operacion;
    protocolo_socket syscall;
    int prioridad;
}
t_instruccion_partida;

void inicializar_estructuras_cpu();
void inicializar_semaforos_cpu();
void *conexion_kernel_dispatch(void * arg_kernelD);
void *conexion_kernel_interrupt(void* arg_kernelI);
void *cliente_conexion_memoria(void * arg_memoria);
void *ciclo_instruccion(void* args);
void levantar_conexiones();

void devolver_motivo_a_kernel(protocolo_socket cod_op, char** texto);
void enviar_contexto_de_memoria();
void obtener_contexto_de_memoria();
void fetch();
t_instruccion_partida * decode(char *inst);
void execute(t_instruccion_partida *instruccion_partida);
void manejar_syscall(protocolo_socket syscall, int prioridad, char ** texto);
void encolar_interrupcion(protocolo_socket tipo, int prioridad, char** texto);
void checkInterrupt();
t_interrupcion* obtener_interrupcion();
uint32_t* registro_aux(char* reg);
void traducir_direccion( uint32_t dir_logica, uint32_t *dir_fisica);

