#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <kernel/pcb.h>

typedef struct {
    t_instrucciones num_instruccion[18]; /* Instrucciones, en el ejemple con el PROCESS_CREATE le tira un 256, calculo que ese es el numero maximo
                                            pero en total hay 18 si no conte mal*/
    int cont;  // Contador de instrucciones
} memoria;

// Función auxiliar para obtener el puntero a un registro 
uint32_t* registro_aux(RegistroCPU *cpu, uint32_t reg) {
    switch (reg) {
        case AX: return &(cpu->AX);
        case BX: return &(cpu->BX);
        case CX: return &(cpu->CX);
        case DX: return &(cpu->DX);
        case EX: return &(cpu->EX);
        case FX: return &(cpu->FX);
        case GX: return &(cpu->GX);
        case HX: return &(cpu->HX);
        default: return NULL; // En caso de que el registro no sea válido
    }
}
