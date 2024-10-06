#include "planificador_corto_plazo.h"
/*
Planificador Corto Plazo
    -> Leer el archivo .config para obtener el algoritmo de planificación
        -> FIFO
        -> Prioridades
            -> Se elige el hilo a ejecutar según el que tenga la prioridad más baja
        -> Multicolas
            -> Una cola por cada nivel de prioridad 
            -> Entre colas prioridades sin desalojo
    -> Seleccion de hilos a ejecutar
        -> Transicionarlo al estado EXEC 
        -> Mandarlo a CPU por el dispatch
        -> Esperar recibirlo con un motivo (si es necesario replanificarlo)
        -> En caso de que se necesite replanificar (por fin de q x ej) se manda a CPU por interrupt
*/
void obtener_planificador_corto_plazo(){    
    if( strcmp(ALGORITMO_PLANIFICACION, "FIFO") == 0 ){
        corto_plazo_fifo();
    }
    else if( strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0 ){
        corto_plazo_prioridades();
    }
    else if( strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0 ){
        corto_plazo_colas_multinivel();
    }
    else{
        //mensaje de error 
    }

void corto_plazo_fifo(t_list* lista_global_tcb;){
    while(1){
        // la lista no puede estar vacia para desencolar -> aplicar un wait 
        t_tcb *tcb = 
        proceso_a_exec(pcb);
        sem_post(&sem_estado_planificacion_ready_to_exec);

        enviar_contexto_de_ejecucion(pcb);
        
        t_codigo_operacion motivo_desalojo;
        t_buffer *buffer = crear_buffer();
        recibir_paquete(fd_cpu_dispatch, &motivo_desalojo, buffer); // Espera por el Dispatch la llegada del contexto actualizado tras la ejecucion del proceso (pid y registros). Junto con el contexto debe llegar el motivo por el cual finalizo la ejecucion (motivo de desalojo)
        buffer_desempaquetar_contexto_ejecucion(buffer, pcb); // Modifica al pcb con lo que recibe
        
        manejar_motivo_desalojo(pcb, motivo_desalojo, buffer, NULL, NULL);
        
        eliminar_buffer(buffer);        
    }
}