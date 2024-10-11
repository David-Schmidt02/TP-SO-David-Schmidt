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
void* acceder_Entrada_Salida(void * arg)
{
    while (1)
    {
        // Debo esperar a tener un elemento en la lista
        sem_wait(colaIO->sem_estado);
        // Utilizo el mutex
        pthread_mutex_lock(colaIO->mutex_estado);
        // Desencolo
        t_uso_io *peticion = list_remove(colaIO->lista_io, 0);
        // Debería encolarlo en una cola de EXEC
        pthread_mutex_unlock(colaIO->mutex_estado);
        sleep(peticion->milisegundos)
        //Falta -> poner en ready el tid
    }
}