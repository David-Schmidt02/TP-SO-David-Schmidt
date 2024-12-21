#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <planificador_corto_plazo.h>
#include <syscalls.h>
#include <main.h>

extern t_cola_IO *colaIO;
extern pthread_mutex_t * mutex_colaIO;
extern sem_t * sem_estado_colaIO;
extern t_tcb * hilo_actual;
extern t_cola_hilo* hilos_cola_ready;
extern pthread_mutex_t * mutex_hilos_cola_ready;


//PASA EL PROCESO A BLOCKED, ESPERA EL TIEMPO Y DESBLOQUEA EL PROFCESO (READY)
void* acceder_Entrada_Salida(void *arg) {
    t_tcb * tcb_aux;
    while (1) {
        sem_wait(sem_estado_colaIO);

        pthread_mutex_lock(mutex_colaIO);

        // Verificar si la cola está vacía
        if (list_is_empty(colaIO->lista_io)) {
            log_info(logger, "Se intentó acceder a una cola vacía en IO.");
            pthread_mutex_unlock(mutex_colaIO);
            continue;
        }

        t_uso_io *peticion = list_remove(colaIO->lista_io, 0);

        tcb_aux = peticion->hilo;
        pthread_mutex_unlock(mutex_colaIO);
        if (peticion != NULL) {
            usleep(peticion->milisegundos);
            log_info(logger, "## (%d:%d) finalizó IO y pasa a READY\n", tcb_aux->pid, tcb_aux->tid);
            encolar_hilo_corto_plazo(tcb_aux);
            free(peticion);
        }
    }
}
