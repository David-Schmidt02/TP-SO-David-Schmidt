#include <planificador_largo_plazo.h>

/*
Planificador Largo Plazo
    -> Creación de procesos
        -> Cola new administrada por FIFO 
        -> Si la cola está llena porque memoria no tiene más espacio se espera a que un proceso finalice
    -> Finalizacion de procesos
        -> Se le avisa a memoria (que confirma), se libera el PCB asociado (donde se almacenan?) y 
        se intenta inicializar otro proceso
    -> Creación de hilos
        -> Kernel avisa a memoria y lo ingresa a la cola de Ready según su prioridad 
        (se una lista global de TCBs ordenada por prioridades)
    -> Finalizar hilos
        -> Se le informa a memoria se libera el TCB asociado (se tiene una lista global de TCBs) 
            y se desbloquean todos los hilos bloqueados por este hilo
            ahora finalizado
*/
/*
void encolar_largo_plazo()
{
    
    sem_wait(procesos_a_crear_NEW->sem_estado);
    pthread_mutex_lock(procesos_a_crear_NEW->mutex_estado);
    //encolo en la cola de procesos a crear 

    //acá tengo que comunicarme con memoria para ver si puedo crear el proceso
    //espero la respuesta de memoria
    //if puedo crearlo:
        //encolo en la cola de procesos en ready y creo/encolo su hilo
        // Debería encolarlo en una cola de EXEC o modificar su estado solamente
        pthread_mutex_unlock(cola_ready->mutex_estado);
        sem_post(cola_ready->sem_estado);
        //enviar_a_cpu_dispatch(hilo->tid, hilo->pid);
        //recibir_motivo_devolucion();
    //else no puedo crearlo:
    
}
*/