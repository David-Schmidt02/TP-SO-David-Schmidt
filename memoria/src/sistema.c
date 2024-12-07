#include "sistema.h"

extern int socket_cliente_cpu;
extern int conexion_memoria_fs;
extern t_memoria *memoria_usuario;
extern pthread_mutex_t * mutex_pcb;
extern pthread_mutex_t * mutex_tcb;
extern pthread_mutex_t * mutex_part_fijas;
extern pthread_mutex_t * mutex_huecos;
extern pthread_mutex_t * mutex_procesos_din;
extern pthread_mutex_t * mutex_espacio;


// Funcion auxiliar para buscar y validar el proceso (PCB) y el hilo (TCB) asociados al PID y TID
// t_pcb **pcb_out y t_tcb **tcb_out: Son punteros de salida. Al encontrar el PCB y el TCB, la función los almacena en estas variables para que la función que llama a obtener_pcb_y_tcb pueda utilizarlos.
bool obtener_pcb_y_tcb(int pid, int tid, t_pcb *pcb_out, t_tcb *tcb_out) {
    // Buscar el PCB
    int index_pcb = buscar_pid(memoria_usuario->lista_pcb, pid);
    if (index_pcb == -1) {
        log_error(logger, "PID %d no encontrado en memoria.", pid);
        return false;
    }
    pcb_out = list_get(memoria_usuario->lista_pcb, index_pcb);

    // Buscar el TCB dentro del PCB
    int index_tcb = buscar_tid((pcb_out)->listaTCB, tid);
    if (index_tcb == -1) {
        log_error(logger, "TID %d no encontrado en el proceso PID %d.", tid, pid);
        return false;
    }
    tcb_out = list_get((pcb_out)->listaTCB, index_tcb);
    
    return true;
} // TODO: verificar manejo de list_get con Santi

// Función que recibe el paquete y extrae PID y TID
bool recibir_pid_tid(t_list *paquete_recv, int *pid, int *tid) {
    if (list_size(paquete_recv) < 2) {
        log_error(logger, "Paquete incompleto recibido para CONTEXTO_RECEIVE.");
        list_destroy_and_destroy_elements(paquete_recv, (void *)eliminar_paquete);
        return false;
    }

    // Extraer PID
    pid = list_remove(paquete_recv, 0);

    // Extraer TID
    tid = list_remove(paquete_recv, 0);

    list_destroy(paquete_recv);  // Liberar lista de paquetes
    return true;
}

void enviar_contexto(int pid, int tid) {
    t_pcb *pcb;
    t_tcb *tcb;

    // Busco el PCB y TCB
    pthread_mutex_lock(mutex_tcb);
    pthread_mutex_lock(mutex_pcb);
    if (!obtener_pcb_y_tcb(pid, tid, pcb, tcb)) {
        log_error(logger, "No se pudo obtener el contexto para PID %d, TID %d.", pid, tid);
        return; // Si no se encuentran, se registra el error y se termina
    }

    // Creo un paquete para enviar el contexto de ejecución
    t_paquete *paquete = crear_paquete(CONTEXTO_SEND);

    // Agrego los registros del TCB al paquete
    //agregar_a_paquete(paquete, &tcb->registro, sizeof(tcb->registro));

    // Agrego el registro entero del PCB
    agregar_a_paquete(paquete, tcb->registro, sizeof(tcb->registro));

    pthread_mutex_unlock(mutex_tcb);
    pthread_mutex_unlock(mutex_pcb);

    // Envio el paquete al cliente (CPU)
    enviar_paquete(paquete, socket_cliente_cpu);

    // Libero memoria del paquete
    eliminar_paquete(paquete);

    // Registramos en el log que se envió el contexto
    log_info(logger, "Contexto enviado para PID %d, TID %d.", pid, tid);
}

/// @brief Read memory
/// @param direccion 
/// @return devuelve el contenido de la direccion, o -1 para error
uint32_t read_memory(uint32_t direccion){
    uint32_t * aux;

    if(direccion < 0 || direccion > memoria_usuario->size){
        log_error(logger, "Direccion %d invalida", direccion);
        return -1;
    }

    aux = memoria_usuario->espacio;
    return aux[direccion];
}
/// @brief Write memory
/// @param direccion
/// @param valor
/// @return devuelve 0 para ok o -1 para error
int write_memory(uint32_t direccion, uint32_t valor){
    uint32_t * aux;
    
    if(direccion < 0 || direccion > memoria_usuario->size){
        log_error(logger, "Direccion %d invalida", direccion);
        return -1;
    }
    aux = memoria_usuario->espacio;
    aux[direccion] = valor;

    return 0;
}

void actualizar_contexto_ejecucion() {
    t_list *paquete_recv_list;
    t_pcb *pcb;
    t_tcb *tcb;
    int pid, tid;
    RegistroCPU *registros_actualizados;

    // Recibo el paquete de la CPU
    paquete_recv_list = recibir_paquete(socket_cliente_cpu);

    // Valido que el paquete contiene los elementos necesarios
    if (!paquete_recv_list || list_size(paquete_recv_list) < 3) {
        log_error(logger, "Paquete incompleto recibido para actualizar contexto.");
        liberar_lista_paquetes(paquete_recv_list);
        enviar_error_actualizacion(); // Envio error a la CPU
        return;
    }

    // Obtengo el PID
    pid = (intptr_t)list_remove(paquete_recv_list, 0);
   
    // Obtengo el TID
    tid = (intptr_t)list_remove(paquete_recv_list, 0);

    // Obtengo los registros actualizados
    registros_actualizados = list_remove(paquete_recv_list, 0);
    
    // Libero lista de paquetes
    list_destroy(paquete_recv_list);

    // Validar PID y TID
    pthread_mutex_lock(mutex_tcb);
    int index_tcb = buscar_pid(memoria_usuario->lista_tcb, tid);
    if (index_tcb == -1) {
        log_error(logger, "No se encontró el TID %d en memoria para actualizar contexto.", tid);
        enviar_error_actualizacion();
        return;
    }
    tcb = list_get(memoria_usuario->lista_tcb, index_tcb);

    // int index_tcb = buscar_tid(pcb->listaTCB, tid);
    // if (index_tcb == -1) {
    //     log_error(logger, "No se encontró el TID %d en el proceso PID %d para actualizar contexto.", tid, pid);
    //     enviar_error_actualizacion();
    //     return;
    // }
    // t_tcb *tcb = list_get(pcb->listaTCB, index_tcb);

    // Actualizar registros en el TCB
    memcpy((tcb->registro), registros_actualizados, sizeof(RegistroCPU));
    log_info(logger, "Registros actualizados para PID %d, TID %d.", pid, tid);
    pthread_mutex_unlock(mutex_tcb);

    // Respondemos a la CPU con un paquete de confirmación
    t_paquete *paquete_ok = crear_paquete(OK_MEMORIA);
    const char *mensaje_ok = "Actualización exitosa";
    //agregar_a_paquete(paquete_ok, mensaje_ok, strlen(mensaje_ok) + 1);
    agregar_a_paquete(paquete_ok, (void *)mensaje_ok, strlen(mensaje_ok) + 1);
    enviar_paquete(paquete_ok, socket_cliente_cpu);
    eliminar_paquete(paquete_ok);
}

void enviar_error_actualizacion() {
    t_paquete *paquete_error = crear_paquete(ERROR_MEMORIA);
    const char *mensaje_error = "Error al actualizar contexto de ejecución";
    //agregar_a_paquete(paquete_error, mensaje_error, strlen(mensaje_error) + 1);
    agregar_a_paquete(paquete_error, (void *)mensaje_error, strlen(mensaje_error) + 1);
    enviar_paquete(paquete_error, socket_cliente_cpu);
    eliminar_paquete(paquete_error);
}

void liberar_lista_paquetes(t_list *lista) {
    if (!lista) return;
    list_destroy_and_destroy_elements(lista, (void *)eliminar_paquete);
}

//retorna index de pid en la lista de PCB
int buscar_pid(t_list *lista, int pid){
    t_pcb *elemento;
    t_list_iterator * iterator = list_iterator_create(lista);

    pthread_mutex_lock(mutex_pcb);
    while(list_iterator_has_next(iterator)){
        elemento = list_iterator_next(iterator);
        if(elemento->pid == pid){
            return list_iterator_index(iterator);
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(mutex_pcb);
    return -1;
}
//retorna index de tid en la lista de threads
int buscar_tid(t_list *lista, int tid){
    t_tcb *elemento;
    t_list_iterator * iterator = list_iterator_create(lista);

    pthread_mutex_lock(mutex_tcb);
    while(list_iterator_has_next(iterator)){
        elemento = list_iterator_next(iterator);
        if(elemento->tid == tid){
            return list_iterator_index(iterator);
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(mutex_tcb);
    return -1;
}
void error_contexto(char * error){
    log_error(logger, error);
    t_paquete *send = crear_paquete(ERROR_MEMORIA);
    enviar_paquete(send, socket_cliente_cpu);
    //agregar_a_paquete(send, error, sizeof(error)); se puede mandar error sin mensaje. Sino se complejiza el manejo de la respuesta del lado de cpu
    eliminar_paquete(send);
    return;
}
/// @brief busca un lugar vacio en la tabla y agrega el tid
/// @returns index donde guardo el tid
/// @param tcb 
int agregar_a_tabla_particion_fija(t_pcb *pcb){
    
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_particiones_fijas);
    elemento_particiones_fijas *aux;
    
    pthread_mutex_lock(mutex_part_fijas);
    while(list_iterator_has_next(iterator)) {
        aux = list_iterator_next(iterator);
        if (aux->libre_ocupado==0){
            aux->libre_ocupado = pcb->pid; //no liberar aux, sino se pierde el elemento xd
            break;
        }
    }pthread_mutex_unlock(mutex_part_fijas);
    int index = list_iterator_index(iterator);
    list_iterator_destroy(iterator);
    return index;
}
int agregar_a_dinamica(t_pcb *pcb){
    
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_huecos);
    elemento_huecos *aux_hueco, *aux_hueco_nuevo;
    elemento_procesos *aux_proceso;
    int index;

    aux_proceso->pid = pcb->pid;
    
    pthread_mutex_lock(mutex_procesos_din);
    while(list_iterator_has_next(iterator)) {
        aux_hueco = list_iterator_next(iterator); // siguiente hueco
        if (aux_hueco->size >= pcb->memoria_necesaria){ // entra mi proceso en el hueco?
            
            //seteo variables del nuevo proceso
            aux_proceso->inicio = aux_hueco->inicio;
            aux_proceso->size = pcb->memoria_necesaria;

            //add proceso a tabla de procesos
            index = list_add(memoria_usuario->tabla_procesos, aux_proceso);
            
            //creo un hueco nuevo a partir del usado
            aux_hueco_nuevo->inicio = aux_hueco->inicio+aux_proceso->size;
            aux_hueco_nuevo->size = aux_hueco->size-aux_proceso->size;

            //reemplazo el hueco usado por el nuevo (que es mas chico)
            aux_hueco = aux_hueco_nuevo;

            //se me quemo el cerebro
            list_iterator_destroy(iterator);
            pthread_mutex_unlock(mutex_procesos_din);
            return index;
        }
    }pthread_mutex_unlock(mutex_procesos_din);
    log_error(logger, "No hay huecos libres");
    return -1;
}
void remover_proceso_de_tabla_dinamica(int pid){
    int index = buscar_en_dinamica(pid);

    elemento_huecos *aux_hueco, *aux_hueco_iterator;
    elemento_procesos *aux_proceso;

    pthread_mutex_lock(mutex_procesos_din);

    aux_proceso = list_remove(memoria_usuario->tabla_procesos, index);
    aux_hueco->inicio = aux_proceso->inicio;
    aux_hueco->size = aux_proceso->size;

    free(aux_proceso);

    pthread_mutex_unlock(mutex_procesos_din);

    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_huecos);

    pthread_mutex_lock(mutex_huecos);
    while(list_iterator_has_next(iterator)){
        aux_hueco_iterator = list_iterator_next(iterator);
        if(aux_hueco_iterator->inicio < aux_hueco->inicio){
            list_iterator_add(iterator, aux_hueco);
            list_iterator_destroy(iterator);
            break;
        }
    }
    pthread_mutex_unlock(mutex_huecos);
    consolidar_huecos();
}
void consolidar_huecos(){
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_huecos);
    elemento_huecos *aux_iterator, *aux_previous;

    pthread_mutex_lock(mutex_huecos);
    aux_previous = list_iterator_next(iterator);
    while(list_iterator_has_next(iterator)){
        aux_iterator = list_iterator_next(iterator);
        if (aux_iterator->inicio == aux_previous->inicio + aux_previous->size){
            log_info(logger, "Se unieron 2 huecos");
            aux_previous->size += aux_iterator->size;

            list_iterator_remove(iterator);
            free(aux_iterator);
        }else{
            aux_previous = aux_iterator;
        }
    }
    pthread_mutex_unlock(mutex_huecos);
    return;
}
/// @brief busca el TID en la tabla de particiones fijas y devuelve el index
/// @param tid 
/// @return index o -1 -> error
int buscar_en_tabla_fija(int pid){
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_particiones_fijas);
    elemento_particiones_fijas *aux;

    pthread_mutex_lock(mutex_part_fijas);
    while(list_iterator_has_next(iterator)){
        aux = list_iterator_next(iterator);
        if(aux->libre_ocupado == pid){
            return list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(mutex_part_fijas);
    return -1;
}
void inicializar_tabla_particion_fija(t_list *particiones){
    elemento_particiones_fijas * aux = malloc(sizeof(elemento_particiones_fijas));
    aux->libre_ocupado = 0; // elemento libre
    aux->base = 0;
    aux->size = 0;
    uint32_t acumulador = 0;
    t_list_iterator *iterator_particiones = list_iterator_create(particiones);
    t_list_iterator *iterator_tabla = list_iterator_create(memoria_usuario->tabla_particiones_fijas);

    while(list_iterator_has_next(iterator_particiones)){
        aux->libre_ocupado = 0;
        aux->base = acumulador;
        aux->size = (int)list_iterator_next(iterator_particiones);
        acumulador += (uint32_t)aux->size;
        list_iterator_add(iterator_tabla, aux);
    }

    list_iterator_destroy(iterator_particiones);
    list_iterator_destroy(iterator_tabla);
    return;
}
void init_tablas_dinamicas(){
    elemento_huecos * aux = malloc(sizeof(elemento_huecos));
    aux->inicio = 0;
    aux->size = memoria_usuario->size;

    memoria_usuario->tabla_huecos = list_create();
    list_add(memoria_usuario->tabla_huecos, aux);

    memoria_usuario->tabla_procesos = list_create();

    return;
}
int buscar_en_dinamica(int pid){
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_procesos);
    elemento_procesos *aux;

    pthread_mutex_lock(mutex_procesos_din);
    while(list_iterator_has_next(iterator)){
        aux = list_iterator_next(iterator);
        if(aux->pid == pid){
            return list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(mutex_procesos_din);
    return -1;
}
int send_dump(int pid, int tid){

    protocolo_socket respuesta;
    uint32_t size;
    void *contenido = NULL;
    uint32_t *contenido_segmento;
    int index_pid;
    elemento_procesos *aux_din;
    elemento_particiones_fijas *aux_fij;
    
    switch(memoria_usuario->tipo_particion){
        case DINAMICAS:
            index_pid = buscar_en_dinamica(pid);
            pthread_mutex_lock(mutex_procesos_din);
            aux_din = list_get(memoria_usuario->tabla_procesos, index_pid);
            size = aux_din->size;
            pthread_mutex_unlock(mutex_procesos_din);
            pthread_mutex_lock(mutex_espacio);
            contenido = memoria_usuario->espacio;
            contenido_segmento = malloc(sizeof(uint32_t) * size);
            memcpy(contenido_segmento, contenido, sizeof(uint32_t) * size);
            pthread_mutex_unlock(mutex_espacio);
            break;

        case FIJAS:
            index_pid = buscar_en_tabla_fija(pid);
            pthread_mutex_lock(mutex_part_fijas);
            aux_fij = list_get(memoria_usuario->tabla_particiones_fijas, index_pid);
            size = aux_fij->size;
            pthread_mutex_unlock(mutex_part_fijas);
            pthread_mutex_lock(mutex_espacio);
            contenido = memoria_usuario->espacio;
            contenido_segmento = malloc(sizeof(uint32_t) * size);
            memcpy(contenido_segmento, contenido, sizeof(uint32_t) * size);
            pthread_mutex_unlock(mutex_espacio);
            break;
    }

    struct timeval tiempo_actual;
    gettimeofday(&tiempo_actual, NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual.tv_sec);
    char * nombre_archivo = NULL;
    snprintf(nombre_archivo, sizeof(nombre_archivo), 
             "%d-%d-%02d:%02d:%02d:%03ld.dmp", 
             string_itoa(pid),
             string_itoa(tid),
             tiempo_local->tm_hour,
             tiempo_local->tm_min,
             tiempo_local->tm_sec,
             tiempo_actual.tv_usec / 1000);
    log_info(logger, nombre_archivo);
    t_paquete * send = crear_paquete(DUMP_MEMORY_OP);
    agregar_a_paquete(send, nombre_archivo, strlen(nombre_archivo)+1);
    agregar_a_paquete(send, &size, sizeof(uint32_t));
    agregar_a_paquete(send, contenido_segmento, sizeof(contenido_segmento));


    enviar_paquete(send, conexion_memoria_fs);

    eliminar_paquete(send);

    respuesta = recibir_operacion(conexion_memoria_fs);
    
    
    if(respuesta == OK){
        return 0;
    }
    else{
        return -1;
    }
    
}
void crear_proceso(t_pcb *pcb) {
    switch(memoria_usuario->tipo_particion) {
        case FIJAS:
            int index_fija = agregar_a_tabla_particion_fija(pcb);
            elemento_particiones_fijas *aux_fija = list_get(memoria_usuario->tabla_particiones_fijas, index_fija);
            pcb->base = aux_fija->base;  // Guardo la dirección de inicio en el registro base
            pcb->limite = aux_fija->size;
            
            // Agrego el PCB a la lista global
            pthread_mutex_lock(mutex_pcb);
            list_add(memoria_usuario->lista_pcb, pcb);
            
            // Agrego TCBs de este PCB a la lista global
            t_list_iterator *iterator_fija = list_iterator_create(pcb->listaTCB);
            t_tcb *tcb_aux_fija;
            while (list_iterator_has_next(iterator_fija)) {
                tcb_aux_fija = list_iterator_next(iterator_fija);
                list_add(memoria_usuario->lista_tcb, tcb_aux_fija);
            }
            list_iterator_destroy(iterator_fija);
            pthread_mutex_unlock(mutex_pcb);
            break;
        
        case DINAMICAS:
            int index_dinamica = agregar_a_dinamica(pcb);
            if (index_dinamica == -1) {
                log_error(logger, "No se pudo asignar memoria para el proceso PID %d en memoria dinámica.", pcb->pid);
                return; // Si no se pudo asignar, finaliza
            }

            // Agrego PCB a la lista global
            pthread_mutex_lock(mutex_pcb);
            list_add(memoria_usuario->lista_pcb, pcb);

            // Agrego TCBs de este PCB a la lista global
            t_list_iterator *iterator_dinamica = list_iterator_create(pcb->listaTCB);
            t_tcb *tcb_aux_dinamica;
            while (list_iterator_has_next(iterator_dinamica)) {
                tcb_aux_dinamica = list_iterator_next(iterator_dinamica);
                list_add(memoria_usuario->lista_tcb, tcb_aux_dinamica);
            }
            list_iterator_destroy(iterator_dinamica);
            pthread_mutex_unlock(mutex_pcb);
            break;
        
        default:
            log_error(logger, "Tipo de memoria no reconocido.");
            break;
    }
}

void crear_thread(t_tcb *tcb){
    int index_pid;
    t_pcb *pcb_aux;
    index_pid = buscar_pid(memoria_usuario->lista_pcb, tcb->pid);

    pthread_mutex_lock(mutex_pcb);
    pthread_mutex_lock(mutex_tcb);
    pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);
    list_add(pcb_aux->listaTCB, tcb);
    list_add(memoria_usuario->lista_tcb, tcb);

    pthread_mutex_unlock(mutex_pcb);
    pthread_mutex_unlock(mutex_tcb);

}

void fin_proceso(int pid) {
    int index_pid, index_tid;
    t_pcb *pcb_aux;
    t_tcb *tcb_aux;

    switch(memoria_usuario->tipo_particion) {
        case FIJAS:
            elemento_particiones_fijas *aux;
            index_pid = buscar_en_tabla_fija(pid);
            pthread_mutex_lock(mutex_part_fijas);
            if(index_pid!=(-1)){
                aux = list_get(memoria_usuario->tabla_particiones_fijas, index_pid);
                aux->libre_ocupado = 0;
            }
            pthread_mutex_unlock(mutex_part_fijas);
            index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
            pthread_mutex_lock(mutex_pcb);
            pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);
            
            t_list_iterator *iterator = list_iterator_create(pcb_aux->listaTCB);
            while(list_iterator_has_next(iterator)){
                tcb_aux = list_iterator_next(iterator);
                index_tid = buscar_tid(memoria_usuario->lista_tcb, tcb_aux->tid);
                tcb_aux = list_remove(memoria_usuario->lista_tcb, index_tid);
            }
            pcb_aux = list_remove(memoria_usuario->lista_pcb, index_pid);
            list_iterator_destroy(iterator);
            free(tcb_aux);
            free(pcb_aux);
            pthread_mutex_unlock(mutex_pcb);
            break;
            
        case DINAMICAS:
            remover_proceso_de_tabla_dinamica(pid);
            log_info(logger, "Proceso PID %d liberado de memoria dinámica.", pid);
            //remover pcb de lista_pcb
            int index_pcb;
            index_pcb = buscar_pid(memoria_usuario->lista_pcb, pid);
            
            pthread_mutex_lock(mutex_pcb);
            t_pcb * pcb = list_remove(memoria_usuario->lista_pcb, index_pcb); // saco el pid de la tabla_pcb
            free(pcb);
            pthread_mutex_unlock(mutex_pcb);
            //
            //remover tcb's de lista_tcb
            t_list_iterator *iterator_tcb = list_iterator_create(memoria_usuario->lista_tcb);
            t_tcb *aux_tcb;
            pthread_mutex_lock(mutex_tcb);
            while(list_iterator_has_next(iterator_tcb)){
                aux_tcb = list_iterator_next(iterator_tcb);
                if(aux_tcb->pid == pid){
                    list_iterator_remove(iterator_tcb);
                    free(aux_tcb);
                }
            }
            pthread_mutex_unlock(mutex_tcb);
            list_iterator_destroy(iterator_tcb);
            break;
        
        default:
            log_error(logger, "Tipo de memoria no reconocido.");
            break;
    }
}

void fin_thread(int tid){
    int index_tid, index_pid;
    int pid;
    t_tcb *tcb_aux;
    t_pcb *pcb_aux;
    index_tid = buscar_tid(memoria_usuario->lista_tcb, tid);

    pthread_mutex_lock(mutex_tcb);
    tcb_aux = list_get(memoria_usuario->lista_tcb, index_tid);
    pid = tcb_aux->pid;

    index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
    pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);

    t_list_iterator *iterator = list_iterator_create(pcb_aux->listaTCB);
    while(list_iterator_has_next(iterator)){
        tcb_aux = list_iterator_next(iterator);
        if (tcb_aux->tid == tid){
            list_iterator_remove(iterator);
            break;
        }
    }
    list_remove(memoria_usuario->lista_tcb, index_tid);
    free(tcb_aux);
    pthread_mutex_unlock(mutex_tcb);
}
int obtener_instruccion(int PC, int tid){ // envia el paquete instruccion a cpu. Si falla, retorna -1
	if(PC<0){
        log_error(logger, "PC invalido");
		return -1;
	}
	
	t_paquete *paquete_send = malloc(sizeof(t_paquete));
    t_tcb *tcb_aux;
	int index_tid;
	char * instruccion;
    int pid;

    index_tid = buscar_tid(memoria_usuario->lista_tcb, tid); //consigo tcb
    
    pthread_mutex_lock(mutex_tcb);
    tcb_aux = list_get(memoria_usuario->lista_tcb, index_tid);

	instruccion = list_get(tcb_aux->instrucciones, PC);
    pthread_mutex_unlock(mutex_tcb);
	paquete_send = crear_paquete(OBTENER_INSTRUCCION);
	agregar_a_paquete(paquete_send, instruccion, sizeof(instruccion));
	enviar_paquete(paquete_send, socket_cliente_cpu);
	eliminar_paquete(paquete_send);

    return 0;
}