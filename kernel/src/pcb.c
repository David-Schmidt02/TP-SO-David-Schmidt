#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"

int pid=0;
int pc=0;
int pid_counter = 0;
int ultimo_tid = 0;

t_pcb* crear_pcb(int pid,int pc,int prioridadTID)
{
    t_pcb* pcb = (t_pcb*)(malloc(sizeof(t_pcb)));
    pcb->pid = pid;
    pcb->pc = pc;

    pcb->listaTCB = list_create();
    pcb->listaMUTEX = list_create();

    t_tcb* tcb_principal = crear_tcb(0, prioridadTID);
    list_add(pcb->listaTCB, tcb_principal);
    
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

int generar_pid_unico() {
    return ++pid_counter;
}
