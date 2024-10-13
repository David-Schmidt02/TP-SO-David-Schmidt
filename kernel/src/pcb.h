#ifndef PCB_H_
#define PCB_H_

#include <stdint.h> 
#include <utils/utils.h>
#include <commons/collections/list.h>

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
    char* parametros[2]; // almacena los parámetros que acompañan la instrucción. Por ejemplo, en la instrucción SET AX, 5, el primer parámetro (índice 0) sería AX y el segundo parámetro (índice 1) sería 5.
    char* ID_instruccion; // Almacena el nombre de la instrucción (por ejemplo, "SET", "SUM", "READ_MEM", etc.).
    int parametros_validos; // indica cuántos parámetros han sido asignados a la instrucción. Esto es útil para asegurarte de que cuando intentas acceder a parametros[0] o parametros[1], estos realmente contengan datos válidos.
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
    int pid;
    int tid;
    int prioridad;
    t_estado estado;
    t_list* lista_espera; // Lista de hilos que están esperando a que el hilo corriendo termine
    t_list* instrucciones;  // Lista de instrucciones para el hilo
} t_tcb;

// Estructura para manejar el mutex
typedef struct {
    char* nombre;
    int estado; // 0 = libre, 1 = bloqueado
    t_list* hilos_esperando; // Lista de hilos esperando el mutex
} t_mutex;


t_pcb* crear_pcb(int pid,int pc,int prioridadTID);
t_tcb* crear_tcb(int pid,int tid, int prioridad);
void cambiar_estado(t_tcb* tcb, t_estado estado);

#endif

