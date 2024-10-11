#include "planificador_corto_plazo.h"
extern int conexion_kernel_cpu;

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
void *obtener_planificador_corto_plazo(void * arg)
{   //debo ver como incluir el ALGORITMO_PLANIFICACION
    if (strcmp(ALGORITMO_PLANIFICACION, "FIFO") == 0)
        corto_plazo_fifo();
    else if (strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0)
        corto_plazo_prioridades();
    else if (strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0)
        corto_plazo_colas_multinivel();
    else
    {
        // mensaje de error
    }
}

void corto_plazo_fifo(t_cola_hilo *cola_ready)
{
    while (1)
    {
        // Debo esperar a tener un elemento en la lista
        sem_wait(cola_ready->sem_estado);
        // Utilizo el mutex
        sem_wait(cola_ready->mutex_estado);
        // Desencolo
        t_tcb *hilo = list_remove(cola->lista_hilos, 0);
        // Debería encolarlo en una cola de EXEC
        sem_wait(cola_ready->mutex_estado);
        sem_post(cola_ready->sem_estado);
        // Transiciona a EXEC
        enviar_a_cpu_dispatch(hilo);
        // Espera el motivo de la devolución
    }
}



void enviar_a_cpu_dispatch(int tid, int pid)
{
    // Crear un paquete con el tid y el pib del hilo seleccionado para ejecutar
    send_handshake = crear_paquete(INFO_HILO);
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); // Agregar TID
    agregar_a_paquete(send_handshake, &pid, sizeof(tid)); // Agregar PID
    // Enviamos el paquete al cpu a través de la conexion cpu dispatch
    enviar_paquete(send_handshake, conexion_kernel_cpu);
    eliminar_paquete(send_handshake); // Eliminamos el paquete 
    //Se espera la respuesta, primero el tid y luego el motivo
    recibir_motivo_devolucion()
}

void recibir_motivo_devolucion()
{
    // Recibir un paquete de respuesta desde la CPU
    t_paquete *paquete_respuesta = recibir_paquete(conexion_kernel_cpu);

    // Extraer el TID y el motivo de la devolución
    int tid, motivo;
    obtener_de_paquete(paquete_respuesta, &tid, sizeof(int));
    obtener_de_paquete(paquete_respuesta, &motivo, sizeof(int));

    // Tratamos el motivo:
    switch (motivo)
    {
    case FINALIZACION:
        printf("El hilo %d ha finalizado correctamente\n", tid);
        break;
    case FIN_QUANTUM:
        printf("El hilo %d fue desalojado por fin de quantum\n", tid);
        // se debe replanificar y llamar nuevamente al planificador (supongo que funciona de manera recursiva)
        //replanificar(tid); -> debo obtener nuevamente el tipo de algoritmo y volver a la funcion
        break;
    case SEGMENTATION_FAULT:
        printf("El hilo %d tuvo un segmentation fault\n", tid);
        break;
    default:
        // ? 
        break;
    }
    eliminar_paquete(paquete_respuesta);
}
