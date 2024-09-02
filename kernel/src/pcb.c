#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"

int pid=0;
int pc=0;
pcb* crear_pcb()
{
    pcb* pcb_p = (pcb*)(malloc(sizeof(pcb)));
    pcb_p->pid = pid;
    pcb_p->pc = pc;
    t_list *listaTCB;
    t_list *listaMUTEX;
    pid++;
    pc++;
    return pcb_p;
}

tcb* crear_tcb(int tid, int prioridad)
{
    //falta agregar a la listaTCB
    tcb* tcb_p = (tcb*)(malloc(sizeof(tcb)));
    tcb_p->tid = tid;
    tcb_p->prioridad = prioridad;
    cambiar_estado(tcb_p,NEW);
    pc++;
    return tcb_p;
}

void cambiar_estado(tcb* tcb_p, t_estado estado)
{
    tcb_p->estado = estado;
}
