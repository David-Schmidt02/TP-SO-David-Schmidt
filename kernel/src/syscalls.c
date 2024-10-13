#include <main.h>
#include <pcb.h>
#include "syscalls.h"

extern t_list* lista_global_tcb;  // Lista global de TCBs
extern t_tcb* hilo_actual;          
extern t_pcb* proceso_actual;
extern t_list* lista_mutexes; // Lista global de mutexes
extern t_list* lista_procesos; // Lista global de procesos
extern int ultimo_tid;

t_pcb* obtener_pcb_por_tid(int tid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_pcb* pcb = list_get(lista_procesos, i);
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
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_pcb* pcb = list_get(lista_procesos, i);
        if (pcb->pid== pid)
            return pcb;
    }
    return NULL;
}
//PROCESOS

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso, int prioridadTID) {
    int pid = generar_pid_unico();
    int pc = 0;

    t_pcb* nuevo_pcb = crear_pcb(pid, pc, prioridadTID);

    nuevo_pcb->quantum = prioridadTID;

    log_info(logger, "Creación de Proceso: ## (<pid>:<%d>) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    t_tcb* tcb_principal = crear_tcb(pid, 0, prioridadTID);
    tcb_principal->estado = NEW;
    
    log_info(logger, "Creación del hilo principal para el proceso PID: %d", nuevo_pcb->pid);

    nuevo_pcb->listaTCB = list_create();
    nuevo_pcb->listaMUTEX = list_create();

    list_add(nuevo_pcb->listaTCB, tcb_principal);
    
    t_list* lista_instrucciones = interpretarArchivo(archivo_instrucciones);

    list_add(lista_procesos, nuevo_pcb);

    log_info(logger, "Proceso PID %d agregado a la lista de procesos.", nuevo_pcb->pid);

    liberarInstrucciones(lista_instrucciones);

    log_info(logger, "## (<PID>:<TID>) - Solicitó syscall: PROCESS_CREATE");
}

void notificar_memoria_fin_proceso(int pid) {
    t_paquete* paquete = crear_paquete(FIN_PROCESO);
    agregar_a_paquete(paquete, &pid, sizeof(int));

    enviar_paquete(paquete, conexion_memoria);

    protocolo_socket respuesta = recibir_operacion(conexion_memoria);
    if (respuesta == OK) {
        log_info(logger, "Finalización del proceso con PID %d confirmada por la Memoria.", pid);
    } else {
        log_error(logger, "Error al finalizar el proceso con PID %d en la Memoria.", pid);
    }

    eliminar_paquete(paquete);
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

    if (pcb->registro != NULL) {
        free(pcb->registro);
    }

    free(pcb);

    log_info(logger, "PCB eliminado exitosamente.");
}

void PROCESS_EXIT() {
    // Usamos el pid del hilo actual
    int pid_buscado = hilo_actual->pid;
    t_pcb* pcb_encontrado = NULL;

    // Buscar el PCB que coincide con el pid del hilo actual
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_pcb* pcb = list_get(lista_procesos, i);
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
        log_info(logger, "TCB TID: %d del proceso PID: %d ha cambiado a estado EXIT", tcb_asociado->tid, pcb_encontrado->pid);
    }

    // Notificar a la memoria que el proceso ha finalizado
    notificar_memoria_fin_proceso(pcb_encontrado->pid);

    // Eliminar el PCB de la lista de procesos
    list_remove_and_destroy_by_condition(lista_procesos, (void*) (pcb_encontrado->pid == pcb_encontrado->pid), (void*) eliminar_pcb);

    log_info(logger, "Proceso con PID: %d ha sido eliminado.", pcb_encontrado->pid);
    log_info(logger, "## Finaliza el proceso %d", pcb_encontrado->pid);
}



//HILOS

void THREAD_CREATE(FILE* archivo_instrucciones, int prioridad) {
    int tid = ++ultimo_tid;

    t_tcb* nuevo_tcb = crear_tcb(proceso_actual->pid, tid, prioridad);
    cambiar_estado(nuevo_tcb, READY);

    nuevo_tcb->instrucciones = interpretarArchivo(archivo_instrucciones);
    list_add(proceso_actual->listaTCB, nuevo_tcb);

    log_info(logger, "## (%d:%d) Se crea el Hilo - Estado: READY", proceso_actual->pid, tid);
}

void THREAD_JOIN(int tid_a_esperar) {
    t_tcb* hilo_a_esperar = obtener_tcb_por_tid(tid_a_esperar);

    if (hilo_a_esperar == NULL || hilo_a_esperar->estado == EXIT) {
        log_info(logger, "THREAD_JOIN: Hilo TID %d no encontrado o ya finalizado.", tid_a_esperar);
        return;
    }

    cambiar_estado(hilo_actual, BLOCK);
    agregar_hilo_a_lista_de_espera(hilo_a_esperar, hilo_actual);

    // Obtengo el PCB correspondiente al hilo actual
    t_pcb* pcb_hilo_actual = obtener_pcb_por_pid(hilo_actual->pid);
    if (pcb_hilo_actual != NULL) {
        log_info(logger, "## (%d:%d) - Bloqueado por: THREAD_JOIN", pcb_hilo_actual->pid, hilo_actual->tid);
    } else {
        log_warning(logger, "No se encontró el PCB para el TID %d.", hilo_actual->tid);
    }

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


void THREAD_CANCEL(int tid_hilo_a_cancelar) { // Esta sys recibe el tid solamente del hilo a cancelar
    t_tcb* hilo_a_cancelar = obtener_tcb_por_tid(tid_hilo_a_cancelar);
    if (hilo_a_cancelar == NULL) {
        log_warning(logger, "TID %d no existe o ya fue finalizado.", tid_hilo_a_cancelar);
        return;
    }

    hilo_a_cancelar->estado = EXIT;
    log_info(logger, "Hilo TID %d movido a EXIT.", tid_hilo_a_cancelar);

    // Log obligatorio: Finalización del hilo
    log_info(logger, "## (%d:%d) Finaliza el hilo", hilo_a_cancelar->tid);

    notificar_memoria_fin_hilo(hilo_a_cancelar);

    eliminar_tcb(hilo_a_cancelar);
}

void notificar_memoria_fin_hilo(t_tcb* hilo) {
    t_config *config = config_create("config/kernel.config");

    char* PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
    char* IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");

    int conexion_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (conexion_kernel_memoria == -1) {
        log_error(logger, "No se pudo establecer conexión con Memoria para finalizar el hilo.");
        return;
    }

    t_paquete* paquete = crear_paquete(FIN_HILO);

    agregar_a_paquete(paquete, &hilo->tid, sizeof(int));

    enviar_paquete(paquete, conexion_kernel_memoria);

    protocolo_socket respuesta = recibir_operacion(conexion_kernel_memoria);
    if (respuesta == OK) {  // Define OK antes de usar
        log_info(logger, "Finalización del hilo TID %d en Memoria confirmada.", hilo->tid);
    } else {
        log_error(logger, "Error al finalizar el hilo TID %d en Memoria.", hilo->tid);
    }

    eliminar_paquete(paquete);
    liberar_conexion(conexion_kernel_memoria);
}

void eliminar_tcb(t_tcb* hilo) {
    if (hilo == NULL) {
        return;
    }

    // Implementar lógica para liberar otros recursos?
    free(hilo);  // Libero el TCB
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

    log_info(logger, "Hilo TID %d finalizado.", hilo_a_salir->tid);
    log_info(logger, "## (%d:%d) Finaliza el hilo", pcb_hilo_a_salir->pid, hilo_a_salir->tid);

    notificar_memoria_fin_hilo(hilo_a_salir);
    eliminar_tcb(hilo_a_salir);
}

void IO(float milisec, int tcb_id)
{
 //log_info(logger, "## (%d:%d) finalizó IO y pasa a READY", hilo_actual->pid, hilo_actual->tid);   
}

void MUTEX_CREATE(char* nombre_mutex,t_pcb *pcb) {
    
    for (int i = 0; i < list_size(pcb->listaMUTEX); i++) {
        t_mutex* mutex = list_get(pcb->listaMUTEX, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            log_warning(logger, "El mutex %s ya existe.", nombre_mutex);
            return;
        }
    }

    t_mutex* nuevo_mutex = malloc(sizeof(t_mutex));
    nuevo_mutex->nombre = strdup(nombre_mutex);
    nuevo_mutex->estado = 0; // Mutex empieza libre
    nuevo_mutex->hilos_esperando = list_create();

    list_add(pcb->listaMUTEX, nuevo_mutex);
    log_info(logger, "Mutex %s creado exitosamente.", nombre_mutex);
}

void MUTEX_LOCK(char* nombre_mutex, t_tcb* hilo_actual,t_pcb *pcb) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(pcb->listaMUTEX); i++) {
        t_mutex* mutex = list_get(pcb->listaMUTEX, i);
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
        log_info(logger, "Mutex %s adquirido por el hilo TID %d.", nombre_mutex, hilo_actual->tid);
    } 

    // Si el mutex está bloqueado, el hilo entra en espera
    else {
        log_info(logger, "Mutex %s ya está bloqueado. El hilo TID %d entra en espera.", nombre_mutex, hilo_actual->tid);
        list_add(mutex_encontrado->hilos_esperando, hilo_actual);
        cambiar_estado(hilo_actual, BLOCK);
    }
}

void MUTEX_UNLOCK(char* nombre_mutex,t_pcb *pcb) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(pcb->listaMUTEX); i++) {
        t_mutex* mutex = list_get(pcb->listaMUTEX, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            mutex_encontrado = mutex;
            break;
        }
    }

    if (mutex_encontrado == NULL) {
        log_warning(logger, "El mutex %s no existe.", nombre_mutex);
        return;
    }

    if (mutex_encontrado->estado == 1) {
        mutex_encontrado->estado = 0; // Liberar el mutex
        log_info(logger, "Mutex %s liberado.", nombre_mutex);

        // Despertar un hilo de la lista de espera, si hay alguno
        if (list_size(mutex_encontrado->hilos_esperando) > 0) {
            t_tcb* hilo_despertar = list_remove(mutex_encontrado->hilos_esperando, 0); // Quitar el primer hilo
            cambiar_estado(hilo_despertar, READY); // Cambiar su estado a READY
            log_info(logger, "Hilo TID %d ha sido despertado y ahora tiene el mutex %s.", hilo_despertar->tid, nombre_mutex);
        }
    } else {
        log_warning(logger, "El mutex %s ya estaba libre.", nombre_mutex);
    }
}

void DUMP_MEMORY(int pid) {
    log_info(logger, "=== DUMP DE MEMORIA ===");

    log_info(logger, "Envio un mensaje a memoria que vacie el proceso con el pid %d",pid);
    log_info(logger, "Espero respuesta de memoria");

    log_info(logger, "FIN DEL DUMP DE MEMORIA");
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