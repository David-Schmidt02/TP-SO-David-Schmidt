#include "sistema.h"

extern t_list *lista_pcb_memoria; //lista de pcb
extern int socket_cliente_cpu;
extern t_memoria *memoria_usuario;

void enviar_contexto(){
    t_list *paquete_recv_list;
    t_paquete *paquete_recv;
    t_paquete *paquete_send;
    int pid;
    int tid;
    int PC;

    
    paquete_recv_list = recibir_paquete(socket_cliente_cpu);

    paquete_recv = list_remove(paquete_recv_list, 0);
    pid = (int)paquete_recv->buffer->stream;
    paquete_recv = list_remove(paquete_recv_list, 0);
    tid = (int)paquete_recv->buffer->stream;
    paquete_recv = list_remove(paquete_recv_list, 0);
    PC = (int)paquete_recv->buffer->stream;

    if(pid <= 0 || tid < 0 || PC < 0){
        log_error(logger, "CPU envio parametros incorrectos pidiendo el contexto de ejecucion");
    }
    int index_pcb = buscar_pid(lista_pcb_memoria, pid);
    if(index_pcb==-1){
        error_contexto("No se encontro el PID en memoria");
    }
    t_pcb *pcb_aux = list_get(lista_pcb_memoria, index_pcb);
    int index_thread = buscar_tid(pcb_aux->listaTCB, tid);
    if(index_thread==-1){
        error_contexto("No se encontro el TID en el proceso");
    }
    t_tcb *tcb_aux = list_get(pcb_aux->listaTCB, index_thread);

    agregar_a_paquete(paquete_send, pcb_aux->registro, sizeof(pcb_aux->registro));
    enviar_paquete(paquete_send, socket_cliente_cpu);
    eliminar_paquete(paquete_send);
}
void recibir_contexto(){

    t_list *paquete_recv_list;
    t_paquete *paquete_recv;
    t_paquete *paquete_send;
    RegistroCPU *registro;
    int pid;
    int index_pcb;
    t_pcb *aux_pcb;

    paquete_recv_list = recibir_paquete(socket_cliente_cpu);
    paquete_recv = list_remove(paquete_recv_list, 0);
    memcpy(registro, paquete_recv->buffer->stream, sizeof(RegistroCPU));
    paquete_recv = list_remove(paquete_recv_list, 0);
    memcpy(&pid, paquete_recv->buffer->stream, sizeof(int));
    
    index_pcb = buscar_pid(lista_pcb_memoria, pid);
    if(index_pcb==-1){
        error_contexto("No se encontro el PID en memoria");
    }
    aux_pcb = list_get(lista_pcb_memoria, index_pcb);
    aux_pcb->registro = registro;

    paquete_send = crear_paquete(CONTEXTO_SEND);
    char *msj;
    strcpy(msj, "contexto recibido");
    agregar_a_paquete(paquete_send, msj, sizeof(msj));
    log_info(logger, msj);

}
//retorna index de pid en la lista de PCB
int buscar_pid(t_list *lista, int pid){
    t_pcb *elemento;
    t_list_iterator * iterator = list_iterator_create(lista);

    while(list_iterator_has_next(iterator)){
        elemento = list_iterator_next(iterator);
        if(elemento->pid == pid){
            return list_iterator_index(iterator);
        }
    }
    return -1;
}
//retorna index de tid en la lista de threads
int buscar_tid(t_list *lista, int tid){
    t_tcb *elemento;
    t_list_iterator * iterator = list_iterator_create(lista);

    while(list_iterator_has_next(iterator)){
        elemento = list_iterator_next(iterator);
        if(elemento->tid == tid){
            return list_iterator_index(iterator);
        }
    }
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
void agregar_a_tabla_particion_fija(t_tcb *tcb){
    int index;
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_particiones_fijas);

    elemento_particiones_fijas *aux;
    //buscar lugar vacio
    while(list_iterator_has_next(iterator)) {
        aux = list_iterator_next(iterator);
        if (aux->libre_ocupado==0){
            aux->libre_ocupado = tcb->tid; //no liberar aux, sino se pierde el elemento xd
            break;
        }
    }
}
void inicializar_tabla_particion_fija(){
    elemento_particiones_fijas * aux;
    aux->libre_ocupado = 0; // elemento libre
    t_list_iterator *iterator = list_iterator_create(memoria_usuario->tabla_particiones_fijas);

    for(int i=0;(int)(memoria_usuario->size/memoria_usuario->fija_size);i++){ //i = index de particion. llena la tabla de particiones "libres"
        list_iterator_add(iterator, aux);
    }
    return;
}
void crear_proceso(){

}
int obtener_instruccion(int PC, int tid){ // envia el paquete instruccion a cpu. Si falla, retorna -1
	if(PC<0){
        log_error(logger, "PC invalido");
		return -1;
	}
	
	t_paquete *paquete_send;
	t_tcb *tcb_aux;
	int index;
	char * instruccion;

	index = buscar_tid(lista_pcb_memoria, tid);
	tcb_aux = list_get(lista_pcb_memoria, index);
	instruccion = list_get(tcb_aux->instrucciones, PC);

	paquete_send = crear_paquete(OBTENER_INSTRUCCION);
	agregar_a_paquete(paquete_send, instruccion, sizeof(instruccion));
	enviar_paquete(paquete_send, socket_cliente_cpu);
	eliminar_paquete(paquete_send);
}