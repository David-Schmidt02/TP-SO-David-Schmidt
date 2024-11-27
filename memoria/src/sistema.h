#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <main.h>

extern t_list *lista_pcb_memoria; //lista de pcb
extern int socket_cliente_cpu;
extern t_memoria *memoria_usuario;

//cpu
void enviar_contexto(void);
void recibir_contexto();
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);
void crear_proceso();
int obtener_instruccion(int PC, int tid);
void agregar_a_lista_particion_fija(void);

#endif