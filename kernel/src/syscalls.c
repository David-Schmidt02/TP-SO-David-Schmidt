#include <main.h>
#include <pcb.h>
#include <syscalls.h>
#include "planificador_corto_plazo.h"
#include "planificador_largo_plazo.h"

extern t_list* lista_global_tcb;  // Lista global de TCBs

extern t_tcb* hilo_actual;          
extern t_pcb* proceso_actual;

//Estructuras provenientes del planificador_largo_plazo
extern t_cola_proceso* procesos_cola_ready;
extern pthread_mutex_t * mutex_procesos_cola_ready;
extern sem_t * sem_estado_procesos_cola_ready;

extern t_cola_hilo* hilos_cola_exit;
extern pthread_mutex_t * mutex_hilos_cola_exit;
extern sem_t * sem_estado_hilos_cola_exit;

extern t_cola_procesos_a_crear* procesos_a_crear;
extern pthread_mutex_t * mutex_procesos_a_crear;
extern sem_t * sem_estado_procesos_a_crear;
//

//Estructuras provenientes del planificador_corto_plazo
extern t_cola_hilo* hilos_cola_ready;
extern pthread_mutex_t * mutex_hilos_cola_ready;
extern sem_t * sem_estado_hilos_cola_ready;

extern t_cola_hilo* hilos_cola_bloqueados;
extern pthread_mutex_t * mutex_hilos_cola_bloqueados;
extern sem_t * sem_estado_hilos_cola_bloqueados;

extern t_colas_multinivel *colas_multinivel;
extern pthread_mutex_t * mutex_colas_multinivel;
extern sem_t * sem_estado_multinivel;

extern char* algoritmo;
//

extern t_list* lista_mutexes; // Lista global de mutexes
extern t_list* lista_procesos; // Lista global de procesos
extern int ultimo_tid;
//

extern sem_t * sem_estado_respuesta_desde_memoria;

extern t_cola_IO *colaIO;
extern pthread_mutex_t * mutex_colaIO;
extern sem_t * sem_estado_colaIO;

extern sem_t * sem_proceso_finalizado;

t_pcb* obtener_pcb_por_tid(int tid) {
    for (int i = 0; i < list_size(procesos_cola_ready->lista_procesos); i++) {
        t_pcb* pcb = list_get(procesos_cola_ready->lista_procesos, i);
        for (int j = 0; j < list_size(pcb->listaTCB); j++) {
            t_tcb* tcb_actual = list_get(pcb->listaTCB, j);
            if (tcb_actual->tid == tid) {
                return pcb;
            }
        }
    }
    return NULL;
}

t_pcb* obtener_pcb_por_pid(int pid) {
    for (int i = 0; i < list_size(procesos_cola_ready->lista_procesos); i++) {
        t_pcb* pcb = list_get(procesos_cola_ready->lista_procesos, i);
        if (pcb->pid== pid)
            return pcb;
    }
    return NULL;
}

//PROCESOS

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso, int prioridadTID) {
    
    int pid = generar_pid_unico();

    t_pcb* nuevo_pcb = crear_pcb(pid, prioridadTID);
    nuevo_pcb->memoria_necesaria = tam_proceso;

    log_info(logger, "## (%d) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    t_tcb* tcb_principal = crear_tcb(pid, ultimo_tid, prioridadTID);
    tcb_principal->estado = NEW;


    log_info(logger, "## (%d:%d) Se crea el Hilo - Estado: READY", nuevo_pcb->pid, tcb_principal->tid);

    nuevo_pcb->listaTCB = list_create();
    nuevo_pcb->listaMUTEX = list_create();


    list_add(nuevo_pcb->listaTCB, tcb_principal);
    
    t_list* lista_instrucciones = interpretarArchivo(archivo_instrucciones);
    
    if (lista_instrucciones == NULL) {
        log_error(logger, "Error al interpretar el archivo de instrucciones.");
        return; // O maneja el error de otra forma
    }

    tcb_principal->instrucciones = lista_instrucciones;
    
    pthread_mutex_lock(mutex_procesos_a_crear);
    list_add(procesos_a_crear->lista_procesos, nuevo_pcb);
    sem_post(sem_estado_procesos_a_crear);
    pthread_mutex_unlock(mutex_procesos_a_crear);

}

void eliminar_mutex(t_mutex* mutex) {
    if (mutex != NULL) {
        if (mutex->hilos_esperando != NULL) {
            list_destroy(mutex->hilos_esperando);
        }
        free(mutex->nombre);
        free(mutex);
    }
}

void eliminar_pcb(t_pcb* pcb) {
    if (pcb->listaTCB != NULL) {
        list_destroy_and_destroy_elements(pcb->listaTCB, (void*) eliminar_tcb);
    }

    if (pcb->listaMUTEX != NULL) {
        list_destroy_and_destroy_elements(pcb->listaMUTEX, (void*) eliminar_mutex); // <-- Asegúrate de tener esta función
    }

    log_info(logger, "## Finaliza el proceso %d", pcb->pid);

    free(pcb);
}

void PROCESS_EXIT() {
    // Usamos el pid del hilo actual
    int pid_buscado = hilo_actual->pid;
    t_pcb* pcb_encontrado = NULL;

    // Buscar el PCB que coincide con el pid del hilo actual
    proceso_actual->estado = EXIT;

    for (int i = 0; i < list_size(procesos_cola_ready->lista_procesos); i++) {
        t_pcb* pcb = list_get(procesos_cola_ready->lista_procesos, i);
        if (pid_buscado == pcb->pid) {
            pcb_encontrado = pcb;
            break;
        }
    }
    // Validar si se encontró el PCB
    if (pcb_encontrado == NULL) {
        log_warning(logger, "No se encontró el PCB para el proceso con PID: %d", pid_buscado);
        return;
    }

    log_info(logger, "Finalizando el proceso con PID: %d", pcb_encontrado->pid);

    // Cambiar el estado de todos los TCBs asociados a EXIT
    for (int i = 0; i < list_size(pcb_encontrado->listaTCB); i++) {
        t_tcb* tcb_asociado = list_get(pcb_encontrado->listaTCB, i);
        cambiar_estado(tcb_asociado, EXIT);
        log_info(logger, "## (%d:%d) Finaliza el hilo", pcb_encontrado->pid, tcb_asociado->tid);
        //acá hay que hacer dos cosas, eliminarlos de la cola de hilos y moverlos a una cola de exit
        if (strcmp(algoritmo, "FIFO") == 0 || strcmp(algoritmo, "PRIORIDADES") == 0) {
            eliminar_hilo_de_cola_fifo_prioridades_thread_exit(tcb_asociado);
            encolar_en_exit(tcb_asociado);//se agrega a la cola de exit
        } else if (strcmp(algoritmo, "CMN") == 0) {
            eliminar_hilo_de_cola_multinivel_thread_exit(tcb_asociado);
            encolar_en_exit(tcb_asociado);//se agrega a la cola de exit
        } else {
            log_info(logger,"Error: Algoritmo no reconocido.\n");
        }
        
    }
    log_info(logger, "Todos los hilos del proceso %d eliminados de READY.", pid_buscado);

    // Notificar a la memoria que el proceso ha finalizado
    notificar_memoria_fin_proceso(pcb_encontrado->pid);
    usleep(1);

    // Eliminar el PCB de la lista de procesos
    pthread_mutex_lock(mutex_procesos_cola_ready);
    // linea anterior porque no estoy seguro de como funciona//
    //list_remove_and_destroy_by_condition(procesos_cola_ready->lista_procesos, (void*) (pcb_encontrado->pid == pid_buscado), (void*) eliminar_pcb);
    t_list_iterator * iterator = list_iterator_create(procesos_cola_ready->lista_procesos);
    t_pcb *aux;
    log_info(logger, "## Finaliza el proceso %d", pcb_encontrado->pid);
    while (list_iterator_has_next(iterator))
    {
        aux = list_iterator_next(iterator);
        if(aux->pid == pid_buscado){
            list_iterator_remove(iterator);
            free(aux);
            break;
        }
    }
    
    sem_wait(sem_estado_procesos_cola_ready);
    pthread_mutex_unlock(mutex_procesos_cola_ready);
    sem_post(sem_proceso_finalizado);
}

void notificar_memoria_fin_proceso(int pid) {

    t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = PROCESS_EXIT_OP;
        peticion->proceso = obtener_pcb_por_pid(pid); 
        peticion->hilo = NULL; // No aplica en este caso  
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
    if (peticion->respuesta_exitosa) {
        log_info(logger, "Finalización del proceso con PID %d confirmada por la Memoria.", pid);
    } else {
        log_info(logger, "Error al finalizar el proceso con PID %d en la Memoria.", pid);
        }   
}

//Ademas de eliminar el hilo de acá me tengo que asegurar de que memoria destruya las estructuras de los mismos
void eliminar_hilo_de_cola_fifo_prioridades_thread_exit(t_tcb* tcb_asociado) {
    pthread_mutex_lock(mutex_hilos_cola_ready);
    
    for (int i = 0; i < list_size(hilos_cola_ready->lista_hilos); i++) {
        t_tcb *hilo = list_get(hilos_cola_ready->lista_hilos, i);
        if (hilo->pid == tcb_asociado->pid && hilo->tid == tcb_asociado->tid) {
            list_remove(hilos_cola_ready->lista_hilos, i);
            i--;
            //sem_wait(sem_estado_hilos_cola_ready);
            break;
        }
    }
    pthread_mutex_unlock(mutex_hilos_cola_ready);
}

void eliminar_hilo_de_cola_fifo_prioridades_thread_cancel(t_tcb* tcb_asociado) {
    pthread_mutex_lock(mutex_hilos_cola_ready);
    
    for (int i = 0; i < list_size(hilos_cola_ready->lista_hilos); i++) {
        t_tcb *hilo = list_get(hilos_cola_ready->lista_hilos, i);
        if (hilo->pid == tcb_asociado->pid && hilo->tid == tcb_asociado->tid) {
            list_remove(hilos_cola_ready->lista_hilos, i);
            i--;
            sem_wait(sem_estado_hilos_cola_ready);
            break;
        }
    }
    pthread_mutex_unlock(mutex_hilos_cola_ready);
}

void eliminar_hilo_de_cola_multinivel_thread_exit(t_tcb* tcb_asociado) {
    pthread_mutex_lock(mutex_colas_multinivel);

    // Iterar sobre cada nivel de prioridad
    for (int i = 0; i < list_size(colas_multinivel->niveles_prioridad); i++) {
        t_nivel_prioridad *nivel = list_get(colas_multinivel->niveles_prioridad, i);

        pthread_mutex_lock(mutex_colas_multinivel);
        for (int j = 0; j < list_size(nivel->cola_hilos->lista_hilos); j++) {
            t_tcb *hilo = list_get(nivel->cola_hilos->lista_hilos, j);
            if (hilo->pid == tcb_asociado->pid) {
                list_remove(nivel->cola_hilos->lista_hilos, j);
                j--;
                //sem_wait(sem_estado_multinivel);
                break;
            }
        }
        pthread_mutex_unlock(mutex_colas_multinivel);
    }
    pthread_mutex_unlock(mutex_colas_multinivel);
}

void eliminar_hilo_de_cola_multinivel_cancel(t_tcb* tcb_asociado) {
    pthread_mutex_lock(mutex_colas_multinivel);

    // Iterar sobre cada nivel de prioridad
    for (int i = 0; i < list_size(colas_multinivel->niveles_prioridad); i++) {
        t_nivel_prioridad *nivel = list_get(colas_multinivel->niveles_prioridad, i);

        pthread_mutex_lock(mutex_colas_multinivel);
        for (int j = 0; j < list_size(nivel->cola_hilos->lista_hilos); j++) {
            t_tcb *hilo = list_get(nivel->cola_hilos->lista_hilos, j);
            if (hilo->pid == tcb_asociado->pid) {
                list_remove(nivel->cola_hilos->lista_hilos, j);
                j--;
                sem_wait(sem_estado_multinivel);
                break;
            }
        }
        pthread_mutex_unlock(mutex_colas_multinivel);
    }
    pthread_mutex_unlock(mutex_colas_multinivel);
}

void encolar_en_exit(t_tcb * hilo){
    pthread_mutex_lock(mutex_hilos_cola_exit);
    list_add(hilos_cola_exit->lista_hilos, hilo);
    pthread_mutex_unlock(mutex_hilos_cola_exit);
    sem_post(sem_estado_hilos_cola_exit);
}

void encolar_en_block(t_tcb * hilo){
    pthread_mutex_lock(mutex_hilos_cola_bloqueados);
    list_add(hilos_cola_bloqueados->lista_hilos, hilo);
    pthread_mutex_unlock(mutex_hilos_cola_bloqueados);
    sem_post(sem_estado_hilos_cola_bloqueados);
}

//HILOS

void THREAD_CREATE(FILE* archivo_instrucciones, int prioridad) {
    int tid = ++ultimo_tid;
    pthread_mutex_lock(mutex_procesos_cola_ready);
    t_tcb* nuevo_tcb = crear_tcb(proceso_actual->pid, tid, prioridad);
    cambiar_estado(nuevo_tcb, READY);
    nuevo_tcb->instrucciones = interpretarArchivo(archivo_instrucciones);
    list_add(proceso_actual->listaTCB, nuevo_tcb);
    notificar_memoria_creacion_hilo(nuevo_tcb);
    encolar_hilo_corto_plazo(nuevo_tcb);
    pthread_mutex_unlock(mutex_procesos_cola_ready);
    log_info(logger, "## (%d:%d) Se crea el Hilo - Estado: READY", proceso_actual->pid, tid);
}

void THREAD_JOIN(int tid_a_esperar) {
    t_tcb* hilo_a_esperar = obtener_tcb_por_tid(hilos_cola_ready->lista_hilos, tid_a_esperar);

    if (hilo_a_esperar == NULL || hilo_a_esperar->estado == EXIT) {
        log_info(logger, "THREAD_JOIN: Hilo TID %d no encontrado o ya finalizado.", tid_a_esperar);
        encolar_hilo_corto_plazo(hilo_actual);
        return;
    }

    //trabajar con los semáforos
    cambiar_estado(hilo_actual, BLOCK);
    agregar_hilo_a_lista_de_espera(hilo_a_esperar, hilo_actual);
    //agregar una interrupcion a CPU para que deje de ejecutarlo y ejecute el siguiente hilo de la cola

    // Obtengo el PCB correspondiente al hilo actual
    encolar_en_block(hilo_actual);
    //free(pcb_hilo_actual);/
}

void finalizar_hilo(t_tcb* hilo) {
    cambiar_estado(hilo, EXIT);
    t_list* lista_hilos_en_espera = obtener_lista_de_hilos_que_esperan(hilo);
    for (int i = 0; i < list_size(lista_hilos_en_espera); i++) {
        t_tcb* hilo_en_espera = list_get(lista_hilos_en_espera, i);
        cambiar_estado(hilo_en_espera, READY);
    }
}

void THREAD_CANCEL(int tid_hilo_a_cancelar) { // Esta sys recibe el tid solamente del hilo a cancelar
    t_tcb* hilo_a_cancelar = obtener_tcb_por_tid(hilos_cola_ready->lista_hilos,tid_hilo_a_cancelar);
    if (hilo_a_cancelar == NULL) {
        log_warning(logger, "TID %d no existe o ya fue finalizado.", tid_hilo_a_cancelar);
        return;
    }

    hilo_a_cancelar->estado = EXIT;
    log_info(logger, "Hilo TID %d movido a EXIT.", tid_hilo_a_cancelar);
    t_pcb * proceso = obtener_pcb_por_tid(tid_hilo_a_cancelar);
    for (int i = 0; i < list_size(proceso->listaTCB); i++) {
        t_tcb *hilo = list_get(hilos_cola_ready->lista_hilos, i);
        if (hilo->pid == tid_hilo_a_cancelar) {
            list_remove(proceso->listaTCB, i);
            i--;
            break;
        }
    }

    log_info(logger, "## (%d) Finaliza el hilo", hilo_a_cancelar->tid);

    // Elimina el hilo de la cola correspondiente y lo encola en la cola de EXIT
    if (strcmp(algoritmo, "FIFO") == 0 || strcmp(algoritmo, "PRIORIDADES")) {
            eliminar_hilo_de_cola_fifo_prioridades_thread_cancel(hilo_a_cancelar);
        } else if (strcmp(algoritmo, "CMN") == 0) {
            eliminar_hilo_de_cola_multinivel_cancel(hilo_a_cancelar);
        } else {
            log_info(logger,"Error: Algoritmo no reconocido.\n");
        }

    notificar_memoria_fin_hilo(hilo_a_cancelar);

    eliminar_tcb(hilo_a_cancelar);
}

void notificar_memoria_creacion_hilo(t_tcb* hilo) {
    t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = THREAD_CREATE_OP;
        peticion->proceso = NULL; // No aplica en este caso
        peticion->hilo = hilo; 
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
    if (peticion->respuesta_exitosa) {
        log_info(logger, "Información sobre la creacion del hilo con TID %d enviada a Memoria.", hilo->tid);
    } else {
        log_info(logger, "Error al informar sobre la creacion del hilo con TID %d a Memoria.", hilo->tid);
        }
    }

void notificar_memoria_fin_hilo(t_tcb* hilo) {
    t_peticion *peticion = malloc(sizeof(t_peticion));
    peticion->tipo = THREAD_EXIT_OP;
    peticion->proceso = NULL; // No aplica en este caso
    peticion->hilo = hilo; 
    encolar_peticion_memoria(peticion);
    sem_wait(sem_estado_respuesta_desde_memoria);
    if (peticion->respuesta_exitosa) {
        log_info(logger, "Finalización del hilo con TID %d confirmada por la Memoria.", hilo->tid);
    } else {
        log_info(logger, "Error al finalizar el hilo con TID %d en la Memoria.", hilo->tid);
        }   
}

void eliminar_tcb(t_tcb* hilo) {
    if (hilo == NULL) {
        return;
    }
    if (hilo->registro != NULL) {
        free(hilo->registro);
    }
    t_pcb * pcb = malloc(sizeof(t_pcb));
    pcb = obtener_pcb_por_tid(hilo->tid);
    for (int i = 0; i < list_size(proceso_actual->listaTCB); i++) {
        t_tcb * tcb = list_get(proceso_actual->listaTCB, i);
        if (hilo->tid == tcb->tid) {
            list_remove(proceso_actual->listaTCB,i);
            free(tcb);
            break;
        }
    } // Libero el TCB
}

void THREAD_EXIT() {// No recibe ningún parámetro, trabaja con hilo_actual
    t_tcb* hilo_a_salir = hilo_actual;
    if (hilo_a_salir == NULL) {
        log_warning(logger, "No se encontró el TID %d para finalizar el hilo.", hilo_a_salir->tid);
        return;
    }

    hilo_a_salir->estado = EXIT;

    // Obtengo el PCB correspondiente al hilo
    t_pcb* pcb_hilo_a_salir = obtener_pcb_por_tid(hilo_a_salir->tid);
    if (pcb_hilo_a_salir == NULL) {
        log_warning(logger, "No se encontró el PCB para el TID %d.", hilo_a_salir->tid);
        return;
    }
    // Elimina el hilo de la cola correspondiente y lo encola en la cola de EXIT
    if (strcmp(algoritmo, "FIFO") == 0 || strcmp(algoritmo, "PRIORIDADES") == 0) {
            eliminar_hilo_de_cola_fifo_prioridades_thread_exit(hilo_a_salir);
        } else if (strcmp(algoritmo, "CMN") == 0) {
            eliminar_hilo_de_cola_multinivel_thread_exit(hilo_a_salir);
        } else {
            log_info(logger,"Error: Algoritmo no reconocido.\n");
        }

    log_info(logger, "Hilo TID %d finalizado.", hilo_a_salir->tid);

    notificar_memoria_fin_hilo(hilo_a_salir);
    eliminar_tcb(hilo_a_salir);
}

void IO(float milisec, int tcb_id) {
    pthread_mutex_lock(mutex_colaIO);
    
    // Crear la petición y asignar memoria
    t_uso_io *peticion = malloc(sizeof(t_uso_io));
    if (peticion == NULL) {
        log_error(logger, "Error al asignar memoria para la petición de IO.");
        pthread_mutex_unlock(mutex_colaIO);
        return;
    }

    peticion->milisegundos = milisec;
    peticion->hilo = hilo_actual;

    list_add(colaIO->lista_io, peticion);

    pthread_mutex_unlock(mutex_colaIO);
    sem_post(sem_estado_colaIO);
} 

void MUTEX_CREATE(char* nombre_mutex) {
    for (int i = 0; i < list_size(proceso_actual->listaMUTEX); i++) {
        t_mutex* mutex = list_get(proceso_actual->listaMUTEX, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            log_warning(logger, "El mutex %s ya existe.", nombre_mutex);
            return;
        }
    }

    t_mutex* nuevo_mutex = malloc(sizeof(t_mutex));
    nuevo_mutex->nombre = strdup(nombre_mutex);
    nuevo_mutex->estado = 0; // Mutex empieza libre
    nuevo_mutex->hilo_asignado = NULL;
    nuevo_mutex->hilos_esperando = list_create();

    list_add(proceso_actual->listaMUTEX, nuevo_mutex);
    list_add(lista_mutexes, nuevo_mutex);
    log_info(logger, "Mutex %s creado exitosamente.", nombre_mutex);
}

void MUTEX_LOCK(char* nombre_mutex) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            mutex_encontrado = mutex;
            break;
        }
    }

    if (mutex_encontrado == NULL) {
        log_warning(logger, "El mutex %s no existe.", nombre_mutex);
        return;
    }

    if (mutex_encontrado->estado == 0) {
        mutex_encontrado->estado = 1; // Bloqueo
        mutex_encontrado->hilo_asignado = hilo_actual;
        cambiar_estado(hilo_actual, BLOCK);
        log_info(logger, "Mutex %s adquirido por el hilo TID %d que entra en espera.", nombre_mutex, hilo_actual->tid);

        if (strcmp(algoritmo, "FIFO") == 0 || strcmp(algoritmo, "PRIORIDADES")) {
            eliminar_hilo_de_cola_fifo_prioridades_thread_exit(hilo_actual);
            encolar_en_exit(hilo_actual);//se agrega a la cola de exit
        } else if (strcmp(algoritmo, "CMN") == 0) {
            eliminar_hilo_de_cola_multinivel_thread_exit(hilo_actual);
            encolar_en_block(hilo_actual);//se agrega a la cola de exit
        } else {
            log_info(logger, "Error: Algoritmo no reconocido.\n");
        }

    } 
    else {
        list_add(mutex_encontrado->hilos_esperando, hilo_actual);
        cambiar_estado(hilo_actual, BLOCK);
        log_info(logger, "Mutex %s ya está addquirido. El hilo TID %d entra en espera en la cola del mutex.", nombre_mutex, hilo_actual->tid);

        if (strcmp(algoritmo, "FIFO") == 0 || strcmp(algoritmo, "PRIORIDADES")) {
            eliminar_hilo_de_cola_fifo_prioridades_thread_exit(hilo_actual);
            encolar_en_exit(hilo_actual);//se agrega a la cola de exit
        } else if (strcmp(algoritmo, "CMN") == 0) {
            eliminar_hilo_de_cola_multinivel_thread_exit(hilo_actual);
            encolar_en_block(hilo_actual);//se agrega a la cola de exit
        } else {
            log_info(logger, "Error: Algoritmo no reconocido.\n");
        }

    }   
}

void MUTEX_UNLOCK(char* nombre_mutex) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            mutex_encontrado = mutex;
            break;
        }
    }

    if (mutex_encontrado == NULL) {
        log_warning(logger, "El mutex %s no existe.", nombre_mutex);
        return;
    }

    if (mutex_encontrado->hilo_asignado = hilo_actual){
        if (mutex_encontrado->estado == 1) {
            log_info(logger, "Se libera el primer hilo bloqueado por el Mutex %s.", nombre_mutex);

            // Despertar un hilo de la lista de espera, si hay alguno
            if (list_size(mutex_encontrado->hilos_esperando) > 0) {
                t_tcb* hilo_despertar = list_remove(mutex_encontrado->hilos_esperando, 0); // Quitar el primer hilo
                cambiar_estado(hilo_despertar, READY); // Cambiar su estado a READY
                encolar_hilo_corto_plazo(hilo_despertar);
                enviar_a_cpu_dispatch(hilo_despertar->pid, hilo_despertar->tid);
                log_info(logger, "Hilo TID %d ha sido despertado y ahora tiene el mutex %s.", hilo_despertar->tid, nombre_mutex);
            }
        } 
        else {
            log_warning(logger, "El mutex %s ya estaba libre.", nombre_mutex);
            }
    }
    else{
        log_warning(logger, "El mutex %s tenía asignado otro hilo", nombre_mutex);
    }

}


void DUMP_MEMORY(int pid) {
    log_info(logger, "=== DUMP DE MEMORIA ===");

    log_info(logger, "Envio un mensaje a memoria que vacie el proceso con el pid %d",pid);

    t_peticion *peticion = malloc(sizeof(t_peticion));
    peticion->tipo = DUMP_MEMORY_OP;
    peticion->proceso = obtener_pcb_por_pid(pid);
    peticion->hilo = NULL; 
    encolar_peticion_memoria(peticion);
    sem_wait(sem_estado_respuesta_desde_memoria);
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

    char *lineas[1000];
    char *instruccion=malloc(100);
    t_list* instrucciones = list_create();
    if (instrucciones == NULL) {
        perror("Error de asignación de memoria");
        return NULL;
    }
    
    for (int i=0;fgets(instruccion, 100, archivo) != NULL;i++){
        instruccion[strcspn(instruccion, "\n")] = 0;
        lineas[i]=malloc(100);
        strcpy(lineas[i], instruccion);
    }
    for(int j=0;j<string_array_size(lineas);j++){
        list_add(instrucciones, lineas[j]);
    }

    return instrucciones;
}


void liberarInstrucciones(t_list* instrucciones) {
    if (instrucciones != NULL) {
        list_destroy_and_destroy_elements(instrucciones, element_destroyer);
    }
}


t_tcb* obtener_tcb_por_tid(t_list * lista, int tid) {
    t_tcb* hilo;

    for (int i = 0; i < list_size(lista); i++) {
        hilo = list_get(lista, i);
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
    hilo_actual->contador_joins--;
}

t_list* obtener_lista_de_hilos_que_esperan(t_tcb* hilo) {
    return hilo->lista_espera;
}