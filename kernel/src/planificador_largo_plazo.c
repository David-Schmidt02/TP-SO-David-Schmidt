#include "planificador_largo_plazo.h"

/*
Planificador Largo Plazo
    -> Creación de procesos
        -> Cola new administrada por FIFO (si está vacía se le solicita a memoria inicializarla) 
        -> Si la cola está llena porque memoria no tiene más espacio (como se valida esto?) se espera a que un proceso finalice
    -> Finalizacion de procesos
        -> Se le avisa a memoria (que confirma), se libera el PCB asociado (donde se almacenan?) y se intenta inicializar otro proceso
    -> Creación de hilos
        -> Kernel avisa a memoria y lo ingresa a la cola de Ready según su prioridad (se una lista global de TCBs ordenada por prioridades)
    -> Finalizar hilos
        -> Se le informa a memoria (esto es repetitivo, y no se espera que memoria conteste en ningún caso, puede usarse la misma funcion), se
            libera el TCB asociado (se tiene una lista global de TCBs) y se desbloquean todos los hilos bloqueados por este hilo
            ahora finalizado
*/
// Planificador largo plazo
void planificador_largo_plazo():
    pthread_t hilo_planificador_largo_plazo;
    pthread_create(&hilo_planificador_largo_plazo, NULL, (void*)planificador_largo_plazo, NULL);
    pthread_detach(hilo_planificador_largo_plazo);
