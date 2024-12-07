#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"

int pid=0;
<<<<<<< HEAD
int pc=0;
int pid_counter = 0;
int ultimo_tid = 0;
extern quantum;
=======
int ultimo_tid=0;
>>>>>>> 57fcc844cc2faa2a487b70b10dcafb577a83a10e

t_pcb* crear_pcb(int pid, int prioridadTID)
{
    t_pcb* pcb = (t_pcb*)(malloc(sizeof(t_pcb)));
    pcb->pid = pid;
    
    pcb->listaTCB = list_create();
    pcb->listaMUTEX = list_create();

    t_tcb* tcb_principal = crear_tcb(pid, 0, prioridadTID);
    list_add(pcb->listaTCB, tcb_principal);
    
    //t_list *listaMUTEX;
    pid++;
    return pcb;
}

t_tcb* crear_tcb(int pid,int tid, int prioridad)
{
    //falta agregar a la listaTCB
    t_tcb* tcb = (t_tcb*)(malloc(sizeof(t_tcb)));
    tcb->tid = tid;
    tcb->pid = pid;
    tcb->prioridad = prioridad;
    tcb->quantum_restante = quantum;
    cambiar_estado(tcb,NEW);
    tcb->instrucciones = list_create();
    tcb->lista_espera = list_create();  // Inicializa la lista de hilos en espera
    tcb->cant_hilos_block = malloc(sizeof(sem_t));
    tcb->registro = malloc(sizeof(RegistroCPU));
    if (tcb->cant_hilos_block == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    tcb->registro->PC=0;
    sem_init(tcb->cant_hilos_block, 0, 1);

    return tcb;
}

void cambiar_estado(t_tcb* tcb, t_estado estado)
{
    tcb->estado = estado;
}

int generar_pid_unico() {
    return ++pid;
}
