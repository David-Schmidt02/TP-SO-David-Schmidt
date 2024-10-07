#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"
typedef struct {
    t_instruccion num_instruccion[18]; /* Instrucciones, en el ejemple con el PROCESS_CREATE le tira un 256, calculo que ese es el numero maximo
                                            pero en total hay 18 si no conte mal*/
    int cont;  // Contador de instrucciones
} memoria;

// Función auxiliar para obtener el puntero a un registro 
uint32_t* registro_aux(RegistroCPU *cpu, char *reg);

void enviar_contexto_de_memoria (RegistroCPU *registro, int pid);
void obtener_contexto_de_memoria (RegistroCPU *registro, int pid);
