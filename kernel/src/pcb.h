#ifndef PCB_H_
#define PCB_H_

#include <stdint.h> 
#include <utils/utils.h>


typedef enum  
{
    NEW,
    READY,
    EXEC,
    BLOCK,
    EXIT
}t_estado;

typedef struct{
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
}RegistroCPU;

typedef struct {
    uint32_t parametros[2];
    char* ID_instruccion;
    int parametros_validos;
} t_instruccion;

typedef struct {
    int pid;
    int pc;
    int quantum;
    t_list *listaTCB;
    t_list *listaMUTEX;
    RegistroCPU *registro;
} t_pcb;

typedef struct {
    int tid;
    int prioridad;
    t_estado estado;
    t_list* lista_espera; // Lista de hilos que están esperando a que el hilo corriendo termine
} t_tcb;

t_pcb* crear_pcb();
t_tcb* crear_tcb(int tid, int prioridad);
void cambiar_estado(t_tcb* tcb, t_estado estado);

#endif