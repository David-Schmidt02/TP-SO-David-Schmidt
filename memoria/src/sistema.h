#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <main.h>

#define SIZE_PARTICION 40
#define OK_MEMORIA 1

//cpu
void enviar_contexto(void);
uint32_t read_memory(uint32_t direccion);
int write_memory(uint32_t direccion, uint32_t valor);
void recibir_contexto();
t_paquete *obtener_contexto(int pid, int tid);
void actualizar_contexto_ejecucion(void);
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);

//falta dinamica de:
int agregar_a_tabla_particion_fija(t_pcb *pcb);
void inicializar_tabla_particion_fija(t_list *particiones);
int buscar_en_tabla_fija(int tid);
void crear_proceso(t_pcb *pcb);
void fin_proceso(int tid);
//

void crear_thread(t_tcb *tcb);
void fin_thread(int tid);

int obtener_instruccion(int PC, int tid);


typedef struct arg_peticion_memoria{
    int socket;
    protocolo_socket cod_op;
}arg_peticion_memoria;

#endif