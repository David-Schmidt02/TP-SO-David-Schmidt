#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <main.h>

extern t_list * colaIO;
void interfaz() {
    
    while(milisec)
    {
        char *linea;
        while (1) {
            linea = readline(">");
            if (!linea) {
                break;
            }
            if (linea) {
                add_history(linea);
            }
            if (!strncmp(linea, "exit", 4)) {
                free(linea);
                break;
            }
            printf("%s\n", linea);
            free(linea);
        }
    return 0;    
    }
    
}

//PLANIFICADOR IO
//FIFO
//PASA EL PROCESO A BLOCKED, ESPERA EL TIEMPO Y DESBLOQUEA EL PROFCESO (READY)
void* acceder_Entrada_Salida()
{
    while (1)
    {
        // Debo esperar a tener un elemento en la lista
        sem_wait(cola_ready->sem_estado);
        // Utilizo el mutex
        sem_wait(cola_ready->mutex_estado);
        // Desencolo
        t_tcb *hilo = list_remove(cola->lista_hilos, 0);
        // Debería encolarlo en una cola de EXEC
        sem_wait(cola_ready->mutex_estado);
        sem_post(cola_ready->sem_estado);
        // Transiciona a EXEC
        enviar_a_cpu_dispatch(hilo);
        // Espera el motivo de la devolución
    }
}