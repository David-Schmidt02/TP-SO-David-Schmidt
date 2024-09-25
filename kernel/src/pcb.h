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
    uint32_t PC; // Program Counter, indica la próxima instrucción a ejecutar
    uint32_t AX; // Registro Numérico de propósito general
    uint32_t BX; // Registro Numérico de propósito general
    uint32_t CX; // Registro Numérico de propósito general
    uint32_t DX; // Registro Numérico de propósito general
    uint32_t EX; // Registro Numérico de propósito general
    uint32_t FX; // Registro Numérico de propósito general
    uint32_t GX; // Registro Numérico de propósito general
    uint32_t HX; // Registro Numérico de propósito general
    uint32_t base; // Indica la dirección base de la partición del proceso
    uint32_t limite; // Indica el tamaño de la partición del proceso

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

