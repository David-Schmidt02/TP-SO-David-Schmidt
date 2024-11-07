#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"
typedef struct {
    char* tipo;       // Tipo de interrupción (por ejemplo, "SEGMENTATION_FAULT", "INTERRUPCION(en este caso solo kernel)")
    int prioridad;    // Prioridad de la interrupción (1 = alta, 2 = media, etc.)
} t_interrupcion;

// Función auxiliar para obtener el puntero a un registro 
uint32_t* registro_aux(RegistroCPU *cpu, char *reg);

void enviar_contexto_de_memoria (RegistroCPU *registro, int pid);
void obtener_contexto_de_memoria (RegistroCPU *registro, int pid);
