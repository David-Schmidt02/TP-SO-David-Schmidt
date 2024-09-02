#include <main.h>
#include <pcb.h>
#include "syscalls.h"


//PROCESOS

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso,int prioridadTID)
{
    //separar el archivo de instrucciones;
    int pid=0; 
    int pc=0;
    crear_pcb(pid,pc);
    THREAD_CREATE(archivo_instrucciones);//hilo a crear me falta pasar
    //asociarTCB(PCB,prioridad,TID=0,t_estado -> new);
}

void PROCESS_EXIT(tcb* tcb)
{
    //matar PCB correspondiente al tcb
    //todos los TCB a t_estado a EXIT
    //SOLO SI EL TCB ESTA EN 0
}

//HILOS

void THREAD_CREATE(FILE* archivo_instrucciones)
{
    int tid=0;
    int prioridad=0;
    interpretarArchivo(archivo_instrucciones);
    tcb* tcb_p = crear_tcb(tid, prioridad);
    cambiar_estado(tcb_p,READY);
}

void THREAD_JOIN()
{

}

void THREAD_CANCEL()
{

}

void THREAD_EXIT()
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

