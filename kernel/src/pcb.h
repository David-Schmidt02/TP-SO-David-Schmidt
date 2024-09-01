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
    int pid;
    int pc;
    t_list *listaTCB;
} pcb;

typedef struct {
    int tid;
    int prioridad;
} tcb;

#endif