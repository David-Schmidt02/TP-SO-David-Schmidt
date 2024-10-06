#include "planificador_corto_plazo.h"
/*
Planificador Corto Plazo
    -> Leer el archivo .config para obtener el algoritmo de planificación
        -> FIFO
        -> Prioridades
            -> Se elige el hilo a ejecutar según el que tenga la prioridad más baja
        -> Multicolas
            -> Una cola por cada nivel de prioridad
            -> Entre colas prioridades sin desalojo
    -> Seleccion de hilos a ejecutar
        -> Transicionarlo al estado EXEC
        -> Mandarlo a CPU por el dispatch
        -> Esperar recibirlo con un motivo (si es necesario replanificarlo)
        -> En caso de que se necesite replanificar (por fin de q x ej) se manda a CPU por interrupt
*/
void obtener_planificador_corto_plazo()
{
    if (strcmp(ALGORITMO_PLANIFICACION, "FIFO") == 0)
    {
        corto_plazo_fifo();
    }
    else if (strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0)
    {
        corto_plazo_prioridades();
    }
    else if (strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0)
    {
        corto_plazo_colas_multinivel();
    }
    else
    {
        // mensaje de error
    }

    void corto_plazo_fifo(t_list * lista_global_tcb)
    {
        while (1)
        {
            // la lista no puede estar vacia para desencolar -> aplicar un wait
            t_tcb *tcb = desencolar(lista_global_tcb)
            //exec_hilo(tcb); -> mandarlo a cpu por el dispatch 
            // -> esperar a que cpu indique el motivo

        }
    }

    t_tcb *desencolar(t_list * lista)
    {
        if (lista->head == NULL)
            return NULL; // No hay nada que desencolar

        // Guardar el primer elemento
        t_link_element *elemento_a_eliminar = lista->head;
        t_tcb *tcb = (t_tcb *)elemento_a_eliminar->data;

        // Mover la cabeza de la lista y actualizar contador
        lista->head = elemento_a_eliminar->next;
        lista->elements_count--;

        //free(elemento_a_eliminar); -> se debe liberar memoria acá?

        // Devolver el puntero al TCB desencolado
        return tcb;
    }