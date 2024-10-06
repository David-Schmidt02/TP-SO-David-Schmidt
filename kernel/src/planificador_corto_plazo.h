#include "planificador_corto_plazo.c"
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
        planificador_corto_plazo_fifo();
    }
    else if( strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0 ){
        planificador_corto_plazo_prioridades();
    }
    else if( strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0 ){
        planificador_corto_plazo_colas_multinivel();
    }
    else{
        //mensaje de error 
    }
