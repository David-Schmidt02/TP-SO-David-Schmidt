#include <main.h>
#include <pcb.h>
#include "syscalls.h"

extern t_list* lista_global_tcb;  // Lista global de TCBs
extern t_tcb* hilo_actual;          // Variable global que guarda el hilo actual

//PROCESOS

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID)
{
    //separar el archivo de instrucciones;
    int pid=0; 
    int pc=0;
    crear_pcb(pid,pc);
    THREAD_CREATE(archivo_instrucciones,pid);//hilo a crear me falta pasar
    //asociarTCB(PCB,prioridad,TID=0,t_estado -> new);

    log_info(logger, "Creación de Proceso: ## (<pid>:<%d>) Se crea el Proceso - Estado: NEW", pid);
    
}

void PROCESS_EXIT(t_tcb* tcb)
{
    
    //matar PCB correspondiente al tcb
    //todos los TCB a t_estado a EXIT
    //cambiar_estado(tcb, EXIT);
    //SOLO SI EL TCB ESTA EN 0
}


//HILOS

void THREAD_CREATE(FILE* archivo_instrucciones,int pid)
{
    int tid=0;
    int prioridad=0;
    interpretarArchivo(archivo_instrucciones);
    t_tcb* tcb = crear_tcb(tid, prioridad);
    cambiar_estado(tcb,READY);
    //log_info(logger, "Creación de Hilo: ## (<%s>:<%d>) Se crea el Hilo - Estado: %d", pid, tid, tcb->estado);
    log_info(logger, "Creación de Hilo: ## (<%d>:<%d>) Se crea el Hilo - Estado: %d", pid, tid, tcb->estado);

}

void THREAD_JOIN(int tid_a_esperar)
{
    t_tcb* hilo_a_esperar = obtener_tcb_por_tid(tid_a_esperar);
    
    if (hilo_a_esperar == NULL || hilo_a_esperar->estado == EXIT) {
        log_info(logger, "THREAD_JOIN: Hilo TID %d no encontrado o ya finalizado.", tid_a_esperar);
        return;
    }
    
    t_tcb* hilo_actual = obtener_tcb_actual();
    cambiar_estado(hilo_actual, BLOCK);
    agregar_hilo_a_lista_de_espera(hilo_a_esperar, hilo_actual);

    log_info(logger, "THREAD_JOIN: Hilo TID %d se bloquea esperando a Hilo TID %d.", hilo_actual->tid, tid_a_esperar);
}

void finalizar_hilo(t_tcb* hilo) {
    cambiar_estado(hilo, EXIT);
    t_list* lista_hilos_en_espera = obtener_lista_de_hilos_que_esperan(hilo);
    for (int i = 0; i < list_size(lista_hilos_en_espera); i++) {
        t_tcb* hilo_en_espera = list_get(lista_hilos_en_espera, i);
        cambiar_estado(hilo_en_espera, READY);
    }
}


void THREAD_CANCEL(int tid) {
    
}


void THREAD_EXIT()
{

}

void IO(float milisec, int tcb_id)
{
    
}

void MUTEX_CREATE()
{

}

void MUTEX_LOCK()
{

}

void MUTEX_UNLOCK()
{

}

void DUMP_MEMORY()
{

}

void element_destroyer(void* elemento) 
{
    t_instruccion* instruccion = (t_instruccion*)elemento;
    free(instruccion->ID_instruccion);
    free(instruccion);
}

// interpretar un archivo y crear una lista de instrucciones
t_list* interpretarArchivo(FILE* archivo) 
{
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return NULL;
    }

    char linea[100];
    t_list* instrucciones = list_create();
    if (instrucciones == NULL) {
        perror("Error de asignación de memoria");
        return NULL;
    }

    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        linea[strcspn(linea, "\n")] = 0; 

        t_instruccion* instruccion = malloc(sizeof(t_instruccion));
        if (instruccion == NULL) {
            perror("Error de asignación de memoria para la instrucción");
            list_destroy_and_destroy_elements(instrucciones, element_destroyer);
            return NULL;
        }

        char* token = strtok(linea, " ");
        if (token != NULL) {
            instruccion->ID_instruccion = strdup(token);

            // Si la instrucción es "SALIR", no asignar parámetros
            if (strcmp(instruccion->ID_instruccion, "SALIR") == 0) {
                instruccion->parametros_validos = 0;
                instruccion->parametros[0] = 0;
                instruccion->parametros[1] = 0;
            } else {
                instruccion->parametros_validos = 1;
                for (int i = 0; i < 2; i++) {
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        instruccion->parametros[i] = atoi(token);
                    } else {
                        instruccion->parametros[i] = 0;
                    }
                }
            }

            list_add(instrucciones, instruccion);
        } else {
            free(instruccion);
        }
    }

    return instrucciones;
}

void liberarInstrucciones(t_list* instrucciones) 
{
    list_destroy_and_destroy_elements(instrucciones, element_destroyer);
}

t_tcb* obtener_tcb_por_tid(int tid) {
    for (int i = 0; i < list_size(lista_global_tcb); i++) {
        t_tcb* hilo = list_get(lista_global_tcb, i);
        if (hilo->tid == tid) {
            return hilo;
        }
    }
    return NULL;
}

t_tcb* obtener_tcb_actual() {
    return hilo_actual;
}

void agregar_hilo_a_lista_de_espera(t_tcb* hilo_a_esperar, t_tcb* hilo_actual) {
    if (hilo_a_esperar->lista_espera == NULL) {
        hilo_a_esperar->lista_espera = list_create();
    }
    list_add(hilo_a_esperar->lista_espera, hilo_actual);
}

t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo) {
    return hilo->lista_espera;
}