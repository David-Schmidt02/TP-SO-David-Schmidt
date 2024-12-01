#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <main.h>

#define SIZE_PARTICION 40

//cpu
void enviar_contexto(void);
void recibir_contexto();
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);
int agregar_a_tabla_particion_fija(t_tcb *tcb);
int buscar_en_tabla_fija(int tid);
void crear_proceso();
void fin_proceso(int tid);
int obtener_instruccion(int PC, int tid);
void buscar_vacio(void* ptr);
void inicializar_tabla_particion_fija(t_list *particiones);

#endif