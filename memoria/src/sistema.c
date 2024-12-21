#include "sistema.h"HAND

extern int socket_cliente_cpu;
extern int conexion_memoria_fs;
extern t_memoria *memoria_usuario;
extern pthread_mutex_t * mutex_pcb;
// extern pthread_mutex_t * mutex_tcb;
extern pthread_mutex_t * mutex_part_fijas;
extern pthread_mutex_t * mutex_huecos;
extern pthread_mutex_t * mutex_procesos_din;
extern pthread_mutex_t * mutex_espacio;

extern pthread_mutex_t * mutex_conexion_cpu;

bool obtener_pcb_y_tcb(int pid, int tid, t_pcb **pcb_out, t_tcb **tcb_out) {

    int index_pcb = buscar_pid(memoria_usuario->lista_pcb, pid);
    if (index_pcb == -1) {
        log_info(logger, "PID %d no encontrado en memoria.", pid);
        return false;
    }
    *pcb_out = list_get(memoria_usuario->lista_pcb, index_pcb); // Actualizamos el puntero al que apunta pcb_out

    int index_tcb = buscar_tid((*pcb_out)->listaTCB, tid); // Accedemos a listaTCB del PCB obtenido
    if (index_tcb == -1) {
        log_info(logger, "TID %d no encontrado en el proceso PID %d.", tid, pid);
        return false;
    }
    *tcb_out = list_get((*pcb_out)->listaTCB, index_tcb); // Actualizamos el puntero al que apunta tcb_out
    return true;
}


bool recibir_pid_tid(t_list *paquete_recv, int *pid, int *tid) {
    if (list_size(paquete_recv) < 2) {
        log_info(logger, "Paquete incompleto recibido para CONTEXTO_RECEIVE.");
        list_destroy_and_destroy_elements(paquete_recv, (void *)eliminar_paquete);
        return false;
    }
    // Extraer PID
    *pid = (int)list_remove(paquete_recv, 0);
    // Extraer TID
    *tid = (int)list_remove(paquete_recv, 0);
    list_destroy(paquete_recv);  // Liberar lista de paquetes
    return true;
}

void enviar_contexto(int pid, int tid) {
    t_pcb *pcb = NULL; 
    t_tcb *tcb = NULL;

    // Pasamos punteros a punteros para que la función pueda modificar pcb y tcb
    if (!obtener_pcb_y_tcb(pid, tid, &pcb, &tcb)) {
        log_info(logger, "No se pudo obtener el contexto para PID %d, TID %d.", pid, tid);
        return;
    }

    t_paquete *paquete = crear_paquete(CONTEXTO_RECEIVE);

    // Agregamos los datos del TCB y del PCB al paquete
    agregar_a_paquete(paquete, tcb->registro, sizeof(RegistroCPU));
    agregar_a_paquete(paquete, &pcb->base, sizeof(pcb->base));
    agregar_a_paquete(paquete, &pcb->limite, sizeof(pcb->limite));
    
    enviar_paquete(paquete, socket_cliente_cpu);
    eliminar_paquete(paquete);

    log_info(logger, "## Contexto solicitado enviado- (PID:TID) - (%d:%d)", pid, tid);
}


/// @brief Read memory
/// @param direccion 
/// @return devuelve el contenido de la direccion, o -1 para error
uint32_t read_memory(uint32_t direccion, int pid, int tid){
    uint8_t * aux;

    if(direccion < 0 || direccion > memoria_usuario->size){
        log_info(logger, "Direccion %d invalida", direccion);
        return -1;
    }

    aux = memoria_usuario->espacio;

    log_info(logger, "## Lectura - (PID:TID) - (%d:%d) - Dir. Física: %d - Tamaño: %d", pid, tid, direccion, sizeof(uint32_t));

    return aux[direccion];
}

/// @brief Write memory
/// @param direccion
/// @param valor
/// @return devuelve 0 para ok o -1 para error
int write_memory(uint32_t direccion, uint32_t valor, int pid, int tid){
    uint8_t * aux;
    
    if(direccion < 0 || direccion > memoria_usuario->size){
        log_info(logger, "Direccion %d invalida", direccion);
        return -1;
    }
    aux = memoria_usuario->espacio;
    memcpy(&aux[(uint8_t)direccion], &valor, sizeof(uint32_t));
    log_info(logger, "SE ESCRIBE ESTE VALOR: %d", valor);

    
    log_info(logger, "## Escritura - (PID:TID) - (%d:%d) - Dir. Física: %d - Tamaño: %d", pid, tid, direccion, sizeof(uint32_t));

    return 0;
}

void actualizar_contexto_ejecucion() {
    t_list *paquete_recv_list;
    t_pcb *pcb;
    t_tcb *tcb;
    int pid, tid;
    RegistroCPU *registros_actualizados;

    paquete_recv_list = recibir_paquete(socket_cliente_cpu);

    if (!paquete_recv_list || list_size(paquete_recv_list) < 3) {
        log_info(logger, "Paquete incompleto recibido para actualizar contexto.");
        liberar_lista_paquetes(paquete_recv_list);
        enviar_error_actualizacion(); // Envio error a la CPU
        return;
    }

    
    pid = *(int *)list_remove(paquete_recv_list, 0);
    tid = *(int *)list_remove(paquete_recv_list, 0);

    registros_actualizados = list_remove(paquete_recv_list, 0);
    
    list_destroy(paquete_recv_list);

    // Validar PID y TID
    int index_pcb = buscar_pid(memoria_usuario->lista_pcb, pid);
    if (index_pcb == -1) {
        log_info(logger, "No se encontró el TID %d en memoria para actualizar contexto.", tid);
        enviar_error_actualizacion();
        return;
    }

    pcb = list_get(memoria_usuario->lista_pcb, index_pcb);

    int index_tcb = buscar_tid(pcb->listaTCB, tid);
    if (index_tcb == -1) {
        log_info(logger, "No se encontró el TID %d en memoria para actualizar contexto.", tid);
        enviar_error_actualizacion();
        return;
    }
    
    // pthread_mutex_lock(mutex_tcb);
    tcb = list_get(pcb->listaTCB, index_tcb);

    // int index_tcb = buscar_tid(pcb->listaTCB, tid);
    // if (index_tcb == -1) {
    //     log_info(logger, "No se encontró el TID %d en el proceso PID %d para actualizar contexto.", tid, pid);
    //     enviar_error_actualizacion();
    //     return;
    // }
    // t_tcb *tcb = list_get(pcb->listaTCB, index_tcb);

    // Actualizar registros en el TCB
    tcb->registro->AX = registros_actualizados->AX;
    tcb->registro->BX = registros_actualizados->BX;
    tcb->registro->CX = registros_actualizados->CX;
    tcb->registro->DX = registros_actualizados->DX;
    tcb->registro->EX = registros_actualizados->EX;
    tcb->registro->FX = registros_actualizados->FX;
    tcb->registro->GX = registros_actualizados->GX;
    tcb->registro->HX = registros_actualizados->HX;
    tcb->registro->PC = registros_actualizados->PC;

    //memcpy((tcb->registro), registros_actualizados, sizeof(RegistroCPU));
    log_info(logger, "Registros actualizados para PID %d, TID %d.", pid, tid);
    // pthread_mutex_unlock(mutex_tcb);

    t_paquete *paquete_ok = crear_paquete(OK);
    char * ok_respuesta = "OK";
    agregar_a_paquete(paquete_ok, ok_respuesta, strlen(ok_respuesta)+1);
    enviar_paquete(paquete_ok, socket_cliente_cpu);
    eliminar_paquete(paquete_ok);
    log_info(logger, "Recibi contexto de CPU");
    log_info(logger, "## Contexto Actualizado - (PID:TID) - (%d:%d)", pid, tid);

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
            pthread_mutex_unlock(mutex_pcb);
            return list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(mutex_pcb);
    list_iterator_destroy(iterator);
    return -1;
}
//retorna index de tid en la lista de threads
int buscar_tid(t_list *lista, int tid){
    t_tcb *elemento;
    t_list_iterator * iterator = list_iterator_create(lista);

    pthread_mutex_lock(mutex_pcb);
    while(list_iterator_has_next(iterator)){
        elemento = list_iterator_next(iterator);
        if(elemento->tid == tid){
            pthread_mutex_unlock(mutex_pcb);
            return list_iterator_index(iterator);
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(mutex_pcb);
    return -1;
}
void error_contexto(char * error){
    log_info(logger, error);
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
    int index = -1;
    elemento_particiones_fijas *aux_best, *aux_worst;
    aux_best = malloc(sizeof(elemento_particiones_fijas));
    aux_worst = malloc(sizeof(elemento_particiones_fijas));
    aux_best = NULL;
    aux_worst = NULL;

    pthread_mutex_lock(mutex_part_fijas);
    while(list_iterator_has_next(iterator)) {
        aux = list_iterator_next(iterator);
        switch(memoria_usuario->fit){
            case FIRST_FIT:
                if (aux->libre_ocupado==0 && aux->size >= pcb->memoria_necesaria){
                    aux->libre_ocupado = pcb->pid; //no liberar aux, sino se pierde el elemento xd
                    index = list_iterator_index(iterator);
                    pthread_mutex_unlock(mutex_part_fijas);
                    list_iterator_destroy(iterator);
                    return index;
                }
                break;
            case BEST_FIT:
		if (aux->libre_ocupado==0 && aux->size >= pcb->memoria_necesaria){
                    if(aux_best==NULL){
                        aux_best = aux;
			index = list_iterator_index(iterator);
                    }else if(aux_best->size > aux->size){
                        aux_best = aux;
			index = list_iterator_index(iterator);
                    }
		}
		break;
            case WORST_FIT:
                if (aux->libre_ocupado==0 && aux->size >= pcb->memoria_necesaria){
                    if(aux_worst==NULL){
                        aux_worst = aux;
			index = list_iterator_index(iterator);
                    }else if(aux_worst->size < aux->size){
                        aux_worst = aux;
			index = list_iterator_index(iterator);
                    }
                }
                break;
        }
    }if(aux_best && memoria_usuario->fit == BEST_FIT){
        aux_best->libre_ocupado = pcb->pid; //no liberar aux, sino se pierde el elemento xd
        pthread_mutex_unlock(mutex_part_fijas);
        list_iterator_destroy(iterator);
        return index;
    }
    if(aux_worst && memoria_usuario->fit == WORST_FIT){
        aux_worst->libre_ocupado = pcb->pid; //no liberar aux, sino se pierde el elemento xdw
        pthread_mutex_unlock(mutex_part_fijas);
        list_iterator_destroy(iterator);
        return index;
    }
    pthread_mutex_unlock(mutex_part_fijas);
    list_iterator_destroy(iterator);
    return index;
}
int agregar_a_dinamica(t_pcb *pcb){
    
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_huecos);
    elemento_huecos *aux_hueco, *aux_hueco_nuevo;
    elemento_huecos *mejor_hueco, *peor_hueco;
    mejor_hueco = NULL;
    peor_hueco = NULL;
    elemento_procesos *aux_proceso=malloc(sizeof(elemento_procesos));
    int index;

    aux_proceso->pid = pcb->pid;
    
    pthread_mutex_lock(mutex_procesos_din);
    while(list_iterator_has_next(iterator)) {
        aux_hueco = list_iterator_next(iterator); // siguiente hueco
        switch (memoria_usuario->fit)
        {
        case FIRST_FIT:
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
                *aux_hueco = *aux_hueco_nuevo;

                //se me quemo el cerebro
                list_iterator_destroy(iterator);
                pthread_mutex_unlock(mutex_procesos_din);
                return index;
            }
            break;
        case BEST_FIT:
            if (aux_hueco->size >= pcb->memoria_necesaria){// entra mi proceso en el hueco?
                if(mejor_hueco==NULL){
                    mejor_hueco = aux_hueco;
                }else if(mejor_hueco->size > aux_hueco->size){
                    mejor_hueco = aux_hueco;
                }
            } 
            break;
        case WORST_FIT:
            if (aux_hueco->size >= pcb->memoria_necesaria){// entra mi proceso en el hueco?
                if(peor_hueco==NULL){
                    peor_hueco = aux_hueco;
                }else if(peor_hueco->size < aux_hueco->size){
                    peor_hueco = aux_hueco;
                }
            } 
            break;
        
        default:
            break;
        
        }
    }
    if(mejor_hueco && memoria_usuario->fit == BEST_FIT){
        //seteo variables del nuevo proceso
        aux_proceso->inicio = mejor_hueco->inicio;
        aux_proceso->size = pcb->memoria_necesaria;
        //add proceso a tabla de procesos
        index = list_add(memoria_usuario->tabla_procesos, aux_proceso);
        
        //creo un hueco nuevo a partir del usado
        aux_hueco_nuevo->inicio = mejor_hueco->inicio+aux_proceso->size;
        aux_hueco_nuevo->size = mejor_hueco->size-aux_proceso->size;
        //reemplazo el hueco usado por el nuevo (que es mas chico)
        *mejor_hueco = *aux_hueco_nuevo;
        //se me quemo el cerebro
        list_iterator_destroy(iterator);
        pthread_mutex_unlock(mutex_procesos_din);
        return index;
    }
    if(peor_hueco && memoria_usuario->fit == WORST_FIT){
        //seteo variables del nuevo proceso
        aux_proceso->inicio = peor_hueco->inicio;
        aux_proceso->size = pcb->memoria_necesaria;
        //add proceso a tabla de procesos
        index = list_add(memoria_usuario->tabla_procesos, aux_proceso);
        
        //creo un hueco nuevo a partir del usado
        aux_hueco_nuevo->inicio = peor_hueco->inicio+aux_proceso->size;
        aux_hueco_nuevo->size = peor_hueco->size-aux_proceso->size;
        //reemplazo el hueco usado por el nuevo (que es mas chico)
        *peor_hueco = *aux_hueco_nuevo;
        //se me quemo el cerebro
        list_iterator_destroy(iterator);
        pthread_mutex_unlock(mutex_procesos_din);
        return index;
    }
    pthread_mutex_unlock(mutex_procesos_din);
    log_info(logger, "No hay huecos libres");
    return -1;
}
int remover_proceso_de_tabla_dinamica(int pid){
    int index = buscar_en_dinamica(pid);
    int size;

    elemento_huecos *aux_hueco, *aux_hueco_iterator;
    elemento_procesos *aux_proceso;
    aux_hueco = malloc (sizeof(elemento_huecos));

    pthread_mutex_lock(mutex_procesos_din);

    aux_proceso = list_remove(memoria_usuario->tabla_procesos, index);
    aux_hueco->inicio = aux_proceso->inicio;
    aux_hueco->size = aux_proceso->size;

    size = aux_proceso->size;
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
        }if(!list_iterator_has_next(iterator)){
            list_add_in_index(memoria_usuario->tabla_huecos, 0, aux_hueco);
        }
    }
    
    pthread_mutex_unlock(mutex_huecos);
    consolidar_huecos();

    return size;
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
            pthread_mutex_unlock(mutex_part_fijas);
            return list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(mutex_part_fijas);
    return -1;
}
void inicializar_tabla_particion_fija(t_list *particiones){

    uint32_t acumulador = 0;
    t_list_iterator *iterator_particiones = list_iterator_create(particiones);
    t_list_iterator *iterator_tabla = list_iterator_create(memoria_usuario->tabla_particiones_fijas);

    while(list_iterator_has_next(iterator_particiones)){
        elemento_particiones_fijas * aux = malloc(sizeof(elemento_particiones_fijas));
        aux->libre_ocupado = 0;
        aux->base = acumulador;
        aux->size = (int)list_iterator_next(iterator_particiones);
	log_info(logger, "particion base: %d, size: %d", aux->base, aux->size);
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
            pthread_mutex_unlock(mutex_procesos_din);
            return list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(mutex_procesos_din);
    return -1;
}
int send_dump(int pid, int tid){

    protocolo_socket respuesta;
    uint32_t size;
    uint32_t base;
    uint8_t *contenido = NULL;
    uint8_t *contenido_segmento;
    int index_pid;
    elemento_procesos *aux_din;
    elemento_particiones_fijas *aux_fij;
    
    switch(memoria_usuario->tipo_particion){
        case DINAMICAS:
            index_pid = buscar_en_dinamica(pid);
            pthread_mutex_lock(mutex_procesos_din);
            aux_din = list_get(memoria_usuario->tabla_procesos, index_pid);
            size = aux_din->size;
            base = aux_din->inicio;
            pthread_mutex_unlock(mutex_procesos_din);
            pthread_mutex_lock(mutex_espacio);
            contenido = memoria_usuario->espacio;
            contenido_segmento = malloc(size);
            memcpy(contenido_segmento, &contenido[(uint8_t)base], size);

            pthread_mutex_unlock(mutex_espacio);
            break;

        case FIJAS:
            index_pid = buscar_en_tabla_fija(pid);
            pthread_mutex_lock(mutex_part_fijas);
            aux_fij = list_get(memoria_usuario->tabla_particiones_fijas, index_pid);
            size = aux_fij->size;
            base = aux_fij->base;
            pthread_mutex_unlock(mutex_part_fijas);
            pthread_mutex_lock(mutex_espacio);
            contenido = memoria_usuario->espacio;
            contenido_segmento = malloc(size);
            memcpy(contenido_segmento, &contenido[base],size);
            pthread_mutex_unlock(mutex_espacio);
            break;
    }

    struct timeval tiempo_actual;
    gettimeofday(&tiempo_actual, NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual.tv_sec);

    char *nombre_archivo = malloc(60);
    if (nombre_archivo == NULL) {
        perror("Error al asignar memoria");
        return EXIT_FAILURE;
    }
    snprintf(nombre_archivo, 60,
            "%d-%d-%02d:%02d:%02d:%03ld.dmp",
            pid,
            tid,
            tiempo_local->tm_hour,
            tiempo_local->tm_min,
            tiempo_local->tm_sec,
            tiempo_actual.tv_usec / 1000);

    log_info(logger, "Nombre del archivo de dump: %s", nombre_archivo);
    


    t_paquete * send = crear_paquete(DUMP_MEMORY_OP);
    agregar_a_paquete(send, nombre_archivo, strlen(nombre_archivo)+1);
    agregar_a_paquete(send, &size, sizeof(uint32_t));
    agregar_a_paquete(send, contenido_segmento, size);


    enviar_paquete(send, conexion_memoria_fs);

    eliminar_paquete(send);

    respuesta = recibir_operacion(conexion_memoria_fs);
    
    log_info(logger, "## Memory Dump solicitado - (PID:TID) - (%d:%d)", pid, tid);

    if(respuesta == OK){
        return 0;
    }
    else{
        return -1;
    }
    free(nombre_archivo);
}
int crear_proceso(t_pcb *pcb) {
    switch(memoria_usuario->tipo_particion) {
        case FIJAS:
            int index_fija = agregar_a_tabla_particion_fija(pcb);
            if (index_fija == -1){
                return index_fija;
            }
            elemento_particiones_fijas *aux_fija = list_get(memoria_usuario->tabla_particiones_fijas, index_fija);
            pcb->base = aux_fija->base;
            pcb->limite = aux_fija->size;
	    log_info(logger, "despues de encolar en tabla: base %d, size %d", pcb->base, pcb->limite);
            pthread_mutex_lock(mutex_pcb);
            list_add(memoria_usuario->lista_pcb, pcb);
            pcb->listaTCB = list_create();
            pcb->listaMUTEX = list_create();

            pthread_mutex_unlock(mutex_pcb);
            break;
        
        case DINAMICAS:
            int index_dinamica = agregar_a_dinamica(pcb);
            if (index_dinamica == -1) {
                return index_dinamica;
            }
            elemento_procesos *aux_din = list_get(memoria_usuario->tabla_procesos, index_dinamica);
            pthread_mutex_lock(mutex_pcb);
            list_add(memoria_usuario->lista_pcb, pcb);
            pcb->base = aux_din->inicio;
            pcb->limite = aux_din->size;
            pcb->listaTCB = list_create();
            pcb->listaMUTEX = list_create();

            pthread_mutex_unlock(mutex_pcb);
            break;
        
        default:
            log_info(logger, "Tipo de memoria no reconocido.");
            break;
    }

    log_info(logger, "## Proceso Creado - PID: %d - Tamaño: %d", pcb->pid, pcb->memoria_necesaria);

}

void crear_thread(t_tcb *tcb){
    int index_pid;
    t_pcb *pcb_aux;
    index_pid = buscar_pid(memoria_usuario->lista_pcb, tcb->pid);

    pthread_mutex_lock(mutex_pcb);
    // pthread_mutex_lock(mutex_tcb);
    pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);
    list_add(pcb_aux->listaTCB, tcb);
    
    // pthread_mutex_unlock(mutex_tcb);
    pthread_mutex_unlock(mutex_pcb);
    
    log_info(logger, "## Hilo Creado - (PID:TID) - (%d:%d)", tcb->pid, tcb->tid);

}

void fin_proceso(int pid) {
    int index_pid, index_tid;
    t_pcb *pcb_aux;
    t_tcb *tcb_aux;
    int memoria_liberada = 0;
    t_list_iterator * iterator;

    switch(memoria_usuario->tipo_particion) {
        case FIJAS:
            elemento_particiones_fijas *aux;
            index_pid = buscar_en_tabla_fija(pid);
            pthread_mutex_lock(mutex_part_fijas);
            if(index_pid!=(-1)){
                aux = list_get(memoria_usuario->tabla_particiones_fijas, index_pid);
                memoria_liberada = aux->size;
                aux->libre_ocupado = 0;
            }
            pthread_mutex_unlock(mutex_part_fijas);
            
            //borro el proceso y sus threads
            index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
            pthread_mutex_lock(mutex_pcb);
            pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);
            
            iterator = list_iterator_create(pcb_aux->listaTCB);
            while(list_iterator_has_next(iterator)){
                tcb_aux = list_iterator_next(iterator);
                list_destroy(tcb_aux->instrucciones);
                list_iterator_remove(iterator);
                free(tcb_aux);
            }
            pcb_aux = list_remove(memoria_usuario->lista_pcb, index_pid);
            free(pcb_aux);
            pthread_mutex_unlock(mutex_pcb);
            list_iterator_destroy(iterator);
            //

            break;
            
        case DINAMICAS:
            memoria_liberada = remover_proceso_de_tabla_dinamica(pid);
            log_info(logger, "Proceso PID %d liberado de memoria dinámica.", pid);

            //borro el proceso y sus threads
            index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
            pthread_mutex_lock(mutex_pcb);
            pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);
            
            iterator = list_iterator_create(pcb_aux->listaTCB);
            while(list_iterator_has_next(iterator)){
                tcb_aux = list_iterator_next(iterator);
                list_destroy(tcb_aux->instrucciones);
                list_iterator_remove(iterator);
                free(tcb_aux);
            }
            pcb_aux = list_remove(memoria_usuario->lista_pcb, index_pid);
            free(pcb_aux);
            pthread_mutex_unlock(mutex_pcb);
            list_iterator_destroy(iterator);
            //
            break;
        
        default:
            log_info(logger, "Tipo de memoria no reconocido.");
            break;
    }

    log_info(logger, "## Proceso Destruido - PID: %d - Tamaño: %d", pid, memoria_liberada);
}

void fin_thread(int tid, int pid){
    int index_tid, index_pid;
    t_tcb *tcb_aux;
    t_pcb *pcb_aux;

    index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
    pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);

    index_tid = buscar_tid(pcb_aux->listaTCB, tid);

    pthread_mutex_lock(mutex_pcb);
    tcb_aux = list_remove(pcb_aux->listaTCB, index_tid);
    free(tcb_aux);
    pthread_mutex_unlock(mutex_pcb);

    log_info(logger, "## Hilo Destruido - (PID:TID) - (%d:%d)", pid, tid);
}

int obtener_instruccion(int PC, int tid, int pid){ // envia el paquete instruccion a cpu. Si falla, retorna -1
	if(PC<0){
        log_info(logger, "PC invalido");
		return -1;
	}
	
	t_paquete *paquete_send = malloc(sizeof(t_paquete));
    t_tcb *tcb_aux;
    t_pcb *pcb_aux;
	int index_tid, index_pid;
	char * instruccion;



    index_pid = buscar_pid(memoria_usuario->lista_pcb, pid);
    //LO MODIFIQUÉ 18/12 1:42
    pcb_aux = list_get(memoria_usuario->lista_pcb, index_pid);

    index_tid = buscar_tid(pcb_aux->listaTCB, tid);
    
    pthread_mutex_lock(mutex_pcb);
    tcb_aux = list_get(pcb_aux->listaTCB, index_tid);

	instruccion = list_get(tcb_aux->instrucciones, PC);
    pthread_mutex_unlock(mutex_pcb);
    
	paquete_send = crear_paquete(OBTENER_INSTRUCCION);

    pthread_mutex_lock(mutex_conexion_cpu);
	agregar_a_paquete(paquete_send, instruccion, strlen(instruccion)+1);
	enviar_paquete(paquete_send, socket_cliente_cpu);
    pthread_mutex_unlock(mutex_conexion_cpu);
	eliminar_paquete(paquete_send);

    log_info(logger, "## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %s", tcb_aux->pid, tcb_aux->tid, instruccion);
    log_info(logger, "Se envia instruccion %d a CPU", PC);

    return 0;
}
