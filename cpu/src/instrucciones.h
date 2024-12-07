#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"
typedef struct {
    protocolo_socket tipo;       // Tipo de interrupción (por ejemplo, "SEGMENTATION_FAULT", "INTERRUPCION(en este caso solo kernel)")
    int prioridad;                  // Prioridad de la interrupción (1 = alta, 2 = media, etc.)
    char** parametro;
                        
} t_interrupcion;

// Función auxiliar para obtener el puntero a un registro 
uint32_t* registro_aux( char *reg);

void enviar_contexto_de_memoria();
void obtener_contexto_de_memoria ();

void fetch();
void traducir_direccion( uint32_t dir_logica, uint32_t *dir_fisica);
void decode( char *inst);
void execute( int instruccion, char **texto);

void checkInterrupt();
int recibir_interrupcion();
void devolver_motivo_a_kernel(protocolo_socket cod_op, char** texto) ;
void agregar_interrupcion(protocolo_socket tipo, int prioridad,char**texto);
t_interrupcion* obtener_interrupcion_mayor_prioridad();
void liberar_interrupcion(t_interrupcion* interrupcion);

void inicializar_cpu_contexto(RegistroCPU * cpu);
void inicializar_lista_interrupciones();


void manejar_motivo(protocolo_socket tipo, char** texto);
void manejar_finalizacion(protocolo_socket tipo, char** texto);
void liberar_interrupcion(t_interrupcion* interrupcion);