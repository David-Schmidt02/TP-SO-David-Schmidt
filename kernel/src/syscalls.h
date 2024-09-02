#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <stdint.h> 
#include <utils/utils.h>
#include "pcb.h"


void element_destroyer(void* elemento);

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID);
void PROCESS_EXIT(tcb* tcb);

void THREAD_CREATE(FILE* archivo_instrucciones);
void THREAD_JOIN();
void THREAD_CANCEL();
void THREAD_EXIT();

t_list* interpretarArchivo(FILE* archivo);
void liberarInstrucciones(t_list* instrucciones);



#endif