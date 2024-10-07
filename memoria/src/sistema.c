#include "sistema.h"

int enviar_contexto(){
    t_list *paquete_recv;
    t_paquete *paquete_send;
    int pid;
    int tid;
    int PC;
    
    paquete_recv = recibir_paquete(socket_cliente_cpu);
    pid = (int)list_remove(paquete_recv, 0)->buffer;
    tid = (int)list_remove(paquete_recv, 0)->buffer;
    PC = (int)list_remove(paquete_recv, 0)->buffer;

    if(pid <= 0 || tid < 0 || PC < 0){
        log_error(logger, "CPU envio parametros incorrectos pidiendo el contexto de ejecucion");
    }
    int index_pcb = buscar_pid(lista_pcb_memoria, pid);
    if(index_pcb==-1){
        error_contexto("No se encontro el PID %d en memoria", pid);
    }
    t_pcb *pcb_aux = list_get(lista_pcb_memoria, index_pcb);
    int index_thread = buscar_tid(pcb_aux->listaTCB, tid);
    if(index_thread==-1){
        error_contexto("No se encontro el TID %d en el proceso %d", tid, pid);
    }
    t_tcb *tcb_aux = list_get(pcb_aux->listaTCB, index_thread);

    agregar_a_paquete(paquete_send, pcb_aux->registro, sizeof(pcb_aux->registro));

}
int recibir_contexto(){

    t_list *paquete_recv_list;
    t_paquete *paquete_recv;
    t_paquete *paquete_send;
    RegistroCPU *registro;
    int pid;
    int index_pcb;
    t_pcb *aux_pcb;

    paquete_recv_list = recibir_paquete(socket_cliente_cpu);
    registro = (RegistroCPU)list_remove(paquete_recv_list, 0)->buffer;
    pid = (int)list_remove(paquete_recv_list, 0)->buffer;
    
    index_pcb = buscar_pid(lista_pcb_memoria, pid);
    if(index_pcb==-1){
        error_contexto("No se encontro el PID %d en memoria", pid);
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
    for (int i=0;i<list_size(lista);i++){
        if(list_get(lista, i).pid==pid){
            return i;
        }
    }
    return -1;
}
//retorna index de tid en la lista de threads
int buscar_tid(t_list *lista, int tid){
    for (int i=0;i<list_size(lista);i++){
        if(list_get(lista, i).tid==tid){
            return i;
        }
    }
    return -1;
}
void error_contexto(char * error){
    log_error(logger, error);
    t_paquete send = crear_paquete(ERROR_MEMORIA);
    send = agregar_a_paquete(send, error, sizeof(error));
    eliminar_paquete(send);
    return;
}