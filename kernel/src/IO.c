#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <main.h>

extern t_cola_IO *colaIO;
extern pthread_mutex_t * mutex_colaIO;
extern sem_t * sem_estado_colaIO;
extern t_tcb * hilo_actual;


//PASA EL PROCESO A BLOCKED, ESPERA EL TIEMPO Y DESBLOQUEA EL PROFCESO (READY)
void* acceder_Entrada_Salida(void *arg) {
    while (1) {
        sem_wait(sem_estado_colaIO);

        pthread_mutex_lock(mutex_colaIO);

        // Verificar si la cola está vacía
        if (list_is_empty(colaIO->lista_io)) {
            log_error(logger, "Se intentó acceder a una cola vacía en IO.");
            pthread_mutex_unlock(mutex_colaIO);
            continue;
        }

        t_uso_io *peticion = list_remove(colaIO->lista_io, 0);

        pthread_mutex_unlock(mutex_colaIO);

        if (peticion != NULL) {
            usleep(peticion->milisegundos);
            free(peticion);
        }
    }
}
