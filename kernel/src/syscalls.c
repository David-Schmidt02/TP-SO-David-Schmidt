#include <main.h>
#include <pcb.h>
#include "syscalls.h"

extern t_list* lista_global_tcb;  // Lista global de TCBs
extern t_tcb* hilo_actual;          // Variable global que guarda el hilo actual
extern t_list* lista_mutexes; // Lista global de mutexes
extern t_list* lista_procesos; // Lista global de procesos

//PROCESOS

void PROCESS_CREATE(FILE* archivo_instrucciones, int tam_proceso, int prioridadTID)
{
    //separar el archivo de instrucciones;
    int pid=0; 
    int pc=0;
    crear_pcb(pid,pc);
    
    // D- No debería interpretarse el archivo en cada process create y dentro de este mismo (y por cada instruccion) crear los hilos?
    // los hilos no se almacenan en el proceso sino en la lista global, pero se crean a medida que se leen las intrucciones del archivo dado (?)

    // Agrega el PCB a la lista de procesos
    //list_add(lista_global_tcb, nuevo_pcb);

    log_info(logger, "Creación de Proceso: ## (<pid>:<%d>) Se crea el Proceso - Estado: NEW", pid);

    THREAD_CREATE(archivo_instrucciones, pid);//hilo a crear me falta pasar
    //asociarTCB(PCB,prioridad,TID=0,t_estado -> new);

}

void PROCESS_EXIT(t_tcb* tcb)
{
    
    //matar PCB correspondiente al tcb
    //todos los TCB a t_estado a EXIT
    //cambiar_estado(tcb, EXIT);
    //SOLO SI EL TCB ESTA EN 0
}


//HILOS

void THREAD_CREATE(FILE* archivo_instrucciones, int pid)
{
    int tid=0;
    int prioridad=0;

    //por lo que entendí estas creando un hilo para tratar TODAS las instrucciones de un archivo

    interpretarArchivo(archivo_instrucciones);
    t_tcb* tcb = crear_tcb(tid, prioridad);
    cambiar_estado(tcb,READY);
    //log_info(logger, "Creación de Hilo: ## (<%s>:<%d>) Se crea el Hilo - Estado: %d", pid, tid, tcb->estado);
    log_info(logger, "Creación de Hilo: ## (<%d>:<%d>) Se crea el Hilo - Estado: %d", pid, tid, tcb->estado);

}

void THREAD_JOIN(int tid_a_esperar)
{
    t_tcb* hilo_a_esperar = obtener_tcb_por_tid(tid_a_esperar);
    
    if (hilo_a_esperar == NULL || hilo_a_esperar->estado == EXIT) {
        log_info(logger, "THREAD_JOIN: Hilo TID %d no encontrado o ya finalizado.", tid_a_esperar);
        return;
    }
    
    t_tcb* hilo_actual = obtener_tcb_actual();
    cambiar_estado(hilo_actual, BLOCK);
    agregar_hilo_a_lista_de_espera(hilo_a_esperar, hilo_actual);

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


void THREAD_CANCEL(t_paquete *paquete, int socket_cliente_kernel) {
    int tid_to_cancel = obtener_tid_del_paquete(paquete);

    t_tcb* hilo_a_cancelar = obtener_hilo_por_tid(tid_to_cancel);
    if (hilo_a_cancelar == NULL) {
        log_warning(logger, "TID %d no existe o ya fue finalizado.", tid_to_cancel);
        return;
    }

    hilo_a_cancelar->estado = EXIT;
    log_info(logger, "Hilo TID %d movido a EXIT.", tid_to_cancel);

    // Log obligatorio: Finalización del hilo
    log_info(logger, "## (%d:%d) Finaliza el hilo", hilo_a_cancelar->tid);

    notificar_memoria_fin_hilo(hilo_a_cancelar);

    eliminar_tcb(hilo_a_cancelar);
}

int obtener_tid_del_paquete(t_paquete *paquete) {
    // Si el TID es el primer dato en el buffer del paquete
    int tid;
    memcpy(&tid, paquete->buffer->stream, sizeof(int));  // Primeros 4 bytes como el TID
    return tid;
}

t_tcb* obtener_hilo_por_tid(int tid_to_cancel) {
    for (int i = 0; i < list_size(lista_global_tcb); i++) {
        t_tcb* hilo = list_get(lista_global_tcb, i);
        if (hilo->tid == tid_to_cancel) {
            return hilo;
        }
    }
    return NULL;
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

void THREAD_EXIT(int tid) {
    // Obtengo el TCB correspondiente al TID
    t_tcb* hilo_a_salir = obtener_tcb_por_tid(tid);
    if (hilo_a_salir == NULL) {
        log_warning(logger, "No se encontró el TID %d para finalizar el hilo.", tid);
        return;
    }

    hilo_a_salir->estado = EXIT;
    log_info(logger, "Hilo TID %d finalizado.", tid);

    notificar_memoria_fin_hilo(hilo_a_salir);

    eliminar_tcb(hilo_a_salir);
}

void IO(float milisec, int tcb_id)
{
    
}

void MUTEX_CREATE(char* nombre_mutex) {
    // Verificar si el mutex ya existe
    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            log_warning(logger, "El mutex %s ya existe.", nombre_mutex);
            return;
        }
    }

    // Crear el nuevo mutex
    t_mutex* nuevo_mutex = malloc(sizeof(t_mutex));
    nuevo_mutex->nombre = strdup(nombre_mutex);
    nuevo_mutex->estado = 0; // Mutex empieza libre
    nuevo_mutex->hilos_esperando = list_create();

    // Agregar el mutex a la lista global
    list_add(lista_mutexes, nuevo_mutex);

    log_info(logger, "Mutex %s creado exitosamente.", nombre_mutex);
}

void MUTEX_LOCK(char* nombre_mutex, t_tcb* hilo_actual) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            mutex_encontrado = mutex;
            break;
        }
    }

    // Si el mutex no existe, reportar error
    if (mutex_encontrado == NULL) {
        log_warning(logger, "El mutex %s no existe.", nombre_mutex);
        return;
    }

    // Si el mutex está libre, se bloquea
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

void MUTEX_UNLOCK(char* nombre_mutex) {
    t_mutex* mutex_encontrado = NULL;

    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        if (strcmp(mutex->nombre, nombre_mutex) == 0) {
            mutex_encontrado = mutex;
            break;
        }
    }

    // Si el mutex no existe, reportar error
    if (mutex_encontrado == NULL) {
        log_warning(logger, "El mutex %s no existe.", nombre_mutex);
        return;
    }

    // Si el mutex está bloqueado
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

void DUMP_MEMORY() {
    log_info(logger, "=== DUMP DE MEMORIA ===");

    // Imprimir información sobre los TCBs (hilos)
    log_info(logger, "Estado de los Hilos:");
    for (int i = 0; i < list_size(lista_global_tcb); i++) {
        t_tcb* hilo = list_get(lista_global_tcb, i);
        log_info(logger, "Hilo TID: %d, Estado: %d, Prioridad: %d", hilo->tid, hilo->estado, hilo->prioridad);
    }

    // Imprimir información sobre los mutexes
    log_info(logger, "Estado de los Mutexes:");
    for (int i = 0; i < list_size(lista_mutexes); i++) {
        t_mutex* mutex = list_get(lista_mutexes, i);
        log_info(logger, "Mutex: %s, Estado: %d, Hilos en espera: %d", mutex->nombre, mutex->estado, list_size(mutex->hilos_esperando));
    }

    // Imprimir información sobre los procesos
    log_info(logger, "Estado de los Procesos:");
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_pcb* proceso = list_get(lista_procesos, i);
        log_info(logger, "Proceso PID: %d, PC: %d, Quantum: %d", proceso->pid, proceso->pc, proceso->quantum);
    }

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