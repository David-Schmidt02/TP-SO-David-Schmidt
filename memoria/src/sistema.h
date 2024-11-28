#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <main.h>

//cpu
void enviar_contexto(void);
void recibir_contexto();
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);
void crear_proceso();
int obtener_instruccion(int PC, int tid);
void agregar_a_lista_particion_fija(void);
void buscar_vacio(void* ptr);
void inicializar_tabla_particion_fija(void);

#endif