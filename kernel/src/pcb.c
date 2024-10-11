#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"

int pid=0;
int pc=0;

t_pcb* crear_pcb(int pid,int pc,int prioridadTID)
{
    t_pcb* pcb = (t_pcb*)(malloc(sizeof(t_pcb)));
    pcb->pid = pid;
    pcb->pc = pc;
    t_list *listaTCB;
    list_add(listaTCB,crear_tcb(0,prioridadTID));
    t_list *listaMUTEX;
    pid++;
    pc++;
    return pcb;
}

t_tcb* crear_tcb(int tid, int prioridad)
{
    //falta agregar a la listaTCB
    t_tcb* tcb = (t_tcb*)(malloc(sizeof(t_tcb)));
    tcb->tid = tid;
    tcb->prioridad = prioridad;
    cambiar_estado(tcb,NEW);
    tcb->lista_espera = list_create();  // Inicializa la lista de hilos en espera
    pc++;
    return tcb;
}

void cambiar_estado(t_tcb* tcb, t_estado estado)
{
    tcb->estado = estado;
}
