#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <main.h>

#define SIZE_PARTICION 40
#define OK_MEMORIA 1

//cpu
void enviar_contexto(void);
void recibir_contexto();
t_paquete *obtener_contexto(int pid, int tid);
void actualizar_contexto_ejecucion(void);
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);
int agregar_a_tabla_particion_fija(t_pcb *pcb);
int buscar_en_tabla_fija(int tid);
void crear_proceso(t_pcb *pcb);
void fin_proceso(int tid);
void fin_thread(int tid);
int obtener_instruccion(int PC, int tid);
void buscar_vacio(void* ptr);
void inicializar_tabla_particion_fija(t_list *particiones);

typedef struct arg_peticion_memoria{
    int socket;
    protocolo_socket cod_op;
}arg_peticion_memoria;

#endif