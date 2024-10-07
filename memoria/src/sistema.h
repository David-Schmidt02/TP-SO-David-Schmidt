#ifndef SISTEMA_H_
#define SISTEMA_H_

#include <stdint.h> 
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"

t_list *lista_pcb_memoria;
extern int socket_cliente_cpu; //lista de pcb

//cpu
int enviar_contexto();
int recibir_contexto();
int buscar_pid(t_list *lista, int pid);
int buscar_tid(t_list *lista, int tid);

void error_contexto(char *error);

#endif