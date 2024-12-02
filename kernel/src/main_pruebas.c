#include <main.h>
#include <IO.h>
#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>


t_log *logger;

//Config
extern t_config *config;
char * algoritmo;
int quantum;
//

//Conexion con Memoria
int conexion_kernel_memoria;

t_list *lista_t_peticiones;
sem_t * sem_lista_t_peticiones;
pthread_mutex_t *mutex_lista_t_peticiones;

sem_t * sem_proceso_finalizado; // utilizado para reintentar crear procesos

pthread_mutex_t * mutex_respuesta_desde_memoria;
pthread_cond_t * cond_respuesta_desde_memoria;
//

//Conexion con CPU
int conexion_kernel_cpu;

pthread_mutex_t *mutex_socket_cpu_dispatch;
pthread_mutex_t *mutex_socket_cpu_interrupt;
//

//Colas y semáforos del planificador de largo plazo
t_cola_proceso* procesos_cola_ready;
pthread_mutex_t * mutex_procesos_cola_ready;
sem_t * sem_estado_procesos_cola_ready;

t_cola_procesos_a_crear* procesos_a_crear;
pthread_mutex_t * mutex_procesos_a_crear;
sem_t * sem_estado_procesos_a_crear;
//

//Colas y semáforos del planificador de corto plazo
t_cola_hilo* hilos_cola_ready;
pthread_mutex_t * mutex_hilos_cola_ready;
sem_t * sem_estado_hilos_cola_ready;

t_cola_hilo* hilos_cola_exit;
pthread_mutex_t * mutex_hilos_cola_exit;
sem_t * sem_estado_hilos_cola_exit;

t_cola_hilo* hilos_cola_bloqueados;
pthread_mutex_t * mutex_hilos_cola_bloqueados;
sem_t * sem_estado_hilos_cola_bloqueados;

t_colas_multinivel *colas_multinivel;
pthread_mutex_t * mutex_colas_multinivel;
sem_t * sem_estado_multinivel;
//

t_list* lista_global_tcb; // lista para manipular los hilos
t_list* lista_mutexes;

t_cola_IO *colaIO;


int main (){
    
    inicializar_estructuras();
    log_info(logger,"Inicio el main correctamente\n");
    algoritmo = "CMN"; // O "PRIORIDADES", según lo que quieras probar
    sleep(1);

    log_info(logger,"Creacion del hilo para la conexion con memoria\n");
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria, NULL, simular_respuesta_memoria, NULL);
    sleep(1);

    log_info(logger,"Creacion del hilo para el planificador de largo plazo\n");
    pthread_t hilo_largo_plazo;
    pthread_create(&hilo_largo_plazo, NULL, (void *)largo_plazo_fifo, NULL);
    sleep(1);

    log_info(logger,"Creacion del hilo para el planificador de corto plazo\n");
    pthread_t hilo_corto_plazo;
    pthread_create(&hilo_corto_plazo, NULL, (void *)planificador_corto_plazo_hilo, NULL);
    

    int pid1 = 1;
    int pc1 = 0;
    int prioridadTID1 = 5;  // O cualquier valor que quieras asignar

    t_pcb *proceso1 = crear_pcb(pid1, pc1, prioridadTID1);
    log_info(logger, "Se crea el primer proceso, su hilo principal tiene prioridad: %d",
         ((t_tcb *)proceso1->listaTCB->head->data)->prioridad);


    int pid2 = 2;
    int pc2 = 0;
    int prioridadTID2 = 3;

    t_pcb *proceso2 = crear_pcb(pid2, pc2, prioridadTID2);


    pthread_mutex_lock(mutex_procesos_a_crear);
    list_add(procesos_a_crear->lista_procesos, proceso1);
    sem_post(sem_estado_procesos_a_crear);
    list_add(procesos_a_crear->lista_procesos, proceso2);
    sem_post(sem_estado_procesos_a_crear);
    pthread_mutex_unlock(mutex_procesos_a_crear);

    pthread_join(hilo_memoria, NULL);
    pthread_join(hilo_largo_plazo, NULL);

}

void* planificador_corto_plazo_hilo(void* arg) {
    if (strcmp(algoritmo, "FIFO") == 0) {
        corto_plazo_fifo();
    } else if (strcmp(algoritmo, "PRIORIDADES") == 0) {
        corto_plazo_prioridades();
    } else if (strcmp(algoritmo, "CMN") == 0) {
        corto_plazo_colas_multinivel();
    } else {
        printf("Error: Algoritmo no reconocido.\n");
    }

}
//Simula la respuesta desde memoria
void *simular_respuesta_memoria(void *arg) {
    while (1) {
        log_info(logger,"El administrador de peticiones se queda esperando una peticion");
        sem_wait(sem_lista_t_peticiones);
        log_info(logger,"Llegó una peticion");
        pthread_mutex_lock(mutex_lista_t_peticiones);
        t_peticion *peticion = list_remove(lista_t_peticiones, 0);
        peticion->respuesta_exitosa = true; // Simular respuesta exitosa
        peticion->respuesta_recibida = true;
        if (peticion->proceso)
        {
            peticion->proceso->estado = READY;
        }
        if (peticion->hilo)
        {
           peticion->hilo->estado = READY;
        }
    
        // Notificar al planificador de largo plazo
        log_info(logger,"Se remueve la peticion y se responde true");
        pthread_cond_signal(cond_respuesta_desde_memoria);
        log_info(logger,"Se hace un post del mutex cond_respuesta");
        pthread_mutex_unlock(mutex_respuesta_desde_memoria);
        pthread_mutex_unlock(mutex_lista_t_peticiones);
        log_info(logger,"Se deslockea el mutex de la lista de peticiones\n");
    }
}

