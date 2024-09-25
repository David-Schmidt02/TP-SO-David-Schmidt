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

