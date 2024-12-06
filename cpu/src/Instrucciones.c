#include <instrucciones.h>

extern t_log *logger;
extern int conexion_cpu_memoria;
extern int conexion_cpu_dispatch;
extern int conexion_cpu_interrupt;
extern int pid;
extern int tid;
extern int tid_actual;
RegistroCPU *cpu;
t_paquete *send_handshake;
t_paquete *paquete_respuesta;

//hay que inicializar
extern t_list* lista_interrupciones;
extern sem_t * sem_lista_interrupciones;
extern pthread_mutex_t *mutex_lista_interrupciones;
//

void inicializar_cpu_contexto(RegistroCPU *cpu) {
    cpu->AX = 0;
    cpu->BX = 0;
    cpu->CX = 0;
    cpu->DX = 0;
    cpu->EX = 0;
    cpu->FX = 0;
    cpu->GX = 0;
    cpu->HX = 0;
    cpu->PC = 0;  
    cpu->base = 0;
    cpu->limite = 0;
    log_info(logger, "Contexto de CPU inicializado");
}

void inicializar_lista_interrupciones(){
    lista_interrupciones = list_create();
    mutex_lista_interrupciones = malloc(sizeof(pthread_mutex_t));
    if (mutex_lista_interrupciones == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_lista_interrupciones, NULL);

    sem_lista_interrupciones = malloc(sizeof(sem_t));
    if (sem_lista_interrupciones == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_lista_interrupciones, 0, 0);

    log_info(logger,"Mutex y semáforo de estado para la lista de interrupciones creados\n");
}

// Función para obtener el contexto de ejecución de la memoria
void obtener_contexto_de_memoria (RegistroCPU *registro, int pid) {
    t_paquete *msjerror;
    t_list *paquete_respuesta;
    // Crear un paquete de solicitud para obtener el contexto
    t_paquete *send_handshake = crear_paquete(CONTEXTO_RECEIVE); // le pondria handshake?, como se que recibo?
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); // Agregar TID

    // Enviar la solicitud para obtener el contexto
    enviar_paquete(send_handshake, conexion_cpu_memoria);
    eliminar_paquete(send_handshake); // elimina el paquete después de enviarlo

    // Recibir la respuesta
    if (recibir_operacion(conexion_cpu_memoria) == ERROR_MEMORIA){
        paquete_respuesta = recibir_paquete(conexion_cpu_memoria);
        msjerror = list_remove(paquete_respuesta, 0);
        char *error;
        memcpy(error, msjerror->buffer, sizeof(msjerror->buffer->size));
        log_error(logger, error);
        free(error);
        agregar_interrupcion(SEGMENTATION_FAULT,1, tid);
    }
    paquete_respuesta = recibir_paquete(conexion_cpu_memoria); // Recibir el contexto de ejecución
    if (paquete_respuesta != NULL) 
    {
        registro->AX = *(uint32_t *) list_get(paquete_respuesta, 0);
        registro->BX = *(uint32_t *) list_get(paquete_respuesta, 1);
        registro->CX = *(uint32_t *) list_get(paquete_respuesta, 2);
        registro->DX = *(uint32_t *) list_get(paquete_respuesta, 3);
        registro->EX = *(uint32_t *) list_get(paquete_respuesta, 4);
        registro->FX = *(uint32_t *) list_get(paquete_respuesta, 5);
        registro->GX = *(uint32_t *) list_get(paquete_respuesta, 6);
        registro->HX = *(uint32_t *) list_get(paquete_respuesta, 7);

        log_info(logger, "## PID: %d - Contexto inicializado", pid);

    } else 
        log_info(logger,"Error: No se recibió contexto de ejecución.");

    log_info(logger, "## PID: %d - Solicito Contexto Ejecución", pid); 
}
void enviar_contexto_de_memoria (RegistroCPU *registro, int pid){
    t_paquete *msjerror;
    t_list *paquete_respuesta;
    t_paquete *paquete_send = crear_paquete(CONTEXTO_SEND);
    agregar_a_paquete(paquete_send, registro, sizeof(registro));
    agregar_a_paquete(paquete_send, pid, sizeof(pid));
    enviar_paquete(paquete_send, conexion_cpu_memoria);
    eliminar_paquete(paquete_send);

    if (recibir_operacion(conexion_cpu_memoria)==ERROR_MEMORIA){
        paquete_respuesta = recibir_paquete(conexion_cpu_memoria);
        msjerror = list_remove(paquete_respuesta, 0);
        char *error;
        memcpy(error, msjerror->buffer, sizeof(msjerror->buffer));
        log_error(logger, error);
        agregar_interrupcion(SEGMENTATION_FAULT,1,tid);
    }
    else log_info(logger, "contexto de PID %d enviado correctamente", pid);
    return;

}


// Función para obtener la próxima instrucción
void fetch(t_pcb *pcb) {
        // Obtención de la instrucción correspondiente al Program Counter
        send_handshake = crear_paquete(OBTENER_INSTRUCCION); 
        agregar_a_paquete(send_handshake, &pid, sizeof(pid));
        agregar_a_paquete(send_handshake, &tid, sizeof(tid)); 
        agregar_a_paquete(send_handshake, &cpu->PC , sizeof(cpu->PC)); 

        enviar_paquete(send_handshake, conexion_cpu_memoria);
        eliminar_paquete(send_handshake); // elimina el paquete después de enviarlo
        t_list *paquete_lista = recibir_paquete(conexion_cpu_memoria);
        t_paquete* paquete_request = list_remove(paquete_lista,0);
        char* instruccion;
        memcpy(instruccion, paquete_request->buffer, paquete_request->buffer);
        
        log_info(logger,"## TID: %d - FETCH ", cpu->PC);
        
        // Llamar a la función decode para procesar la instrucción
        decode(cpu, instruccion);
   
}

// Función para traducir direcciones lógicas a físicas
void traducir_direccion(RegistroCPU *cpu, uint32_t dir_logica, uint32_t *dir_fisica) {
    *dir_fisica = cpu->base + dir_logica; // Traducción a dirección física
    if (*dir_fisica >= cpu->base + cpu->limite) { // Validación de segmento
        log_info(logger,"Error: Segmentation Fault (Acceso fuera de límites)\n");
        exit(SEGMENTATION_FAULT); // Sale por error
    }
}

// Función para decodificar una instrucción y ejecutarla en los registros de la CPU
void decode(RegistroCPU *cpu, char *inst) {
    char **texto = string_split(inst," "); //5 porque hay instrucciiones que poseen 4 valores: PROCESS_CREATE(0) PROCESO1(1) 256(2) 1(3), la 5ta es pos NULL. ejemplo: string_split("hola, mundo", ",") => ["hola", " mundo", NULL]
    // Compara el ID de la instrucción y ejecuta la operación correspondiente

    if (strcmp(texto[0], "SET") == 0 && texto[1] && texto[2]) 
        // Instrucción SET: Asigna un valor a un registro
            execute(cpu,1,texto); // 1 = SET
    
    else if (strcmp(texto[0], "SUM") == 0 && texto[1] && texto[2]) 
        // Instrucción SUM: Suma el valor de un registro a otro   
            execute(cpu,2,texto); // 2 = SUM
    
    else if (strcmp(texto[0], "SUB") == 0 && texto[1] && texto[2]) 
        // Instrucción SUB: Resta el valor de un registro a otro
            execute(cpu,3,texto);//3 = SUB

    else if (strcmp(texto[0], "READ_MEM") == 0 && texto[1] && texto[2]) 
        // Instrucción READ_MEM: Lee de memoria a un registro
            execute(cpu,4,texto); // 4 = READ_MEM

    else if (strcmp(texto[0], "WRITE_MEM") == 0 && texto[1] && texto[2]) 
        // Instrucción WRITE_MEM: Escribe en memoria desde un registro
            execute(cpu,5,texto); // 5 = WRITE_MEM

    else if (strcmp(texto[0], "JNZ") == 0 && texto[1] && texto[2]) 
        // Instrucción JNZ: Salta si el valor de un registro no es cero 

            execute(cpu,6,texto); // 6 = JNZ
    else if (strcmp(texto[0], "MUTEX_CREATE") == 0 && texto[1] && texto[2]) 
            execute(cpu,7,texto); // 7 = MUTEX_CREATE

    else if (strcmp(texto[0], "MUTEX_LOCK") == 0 && texto[1] && texto[2]) 
            execute(cpu,8,texto); // 8 = MUTEX_LOCK

    else if (strcmp(texto[0], "DUMP_MEMORY") == 0 && texto[1] && texto[2]) 
            execute(cpu,9,texto); // 9 = DUMP_MEMORY

    else if (strcmp(texto[0], "PROCESS_CREATE") == 0 && texto[1] && texto[2]) 
            execute(cpu,10,texto);// 10 = PROCESS_CREATE

    else if (strcmp(texto[0], "THREAD_CREATE") == 0 && texto[1] && texto[2]) 
            execute(cpu,11,texto);// 11 = THREAD_CREATE

    else if (strcmp(texto[0], "THREAD_CANCEL") == 0 && texto[1] && texto[2]) 
            execute(cpu,12,texto);// 12 = THREAD_CANCEL

    else if (strcmp(texto[0], "THREAD_JOIN") == 0 && texto[1] && texto[2]) 
            execute(cpu,13,texto);// 13 = THREAD_JOIN

    else if (strcmp(texto[0], "THREAD_EXIT") == 0 && texto[1] && texto[2]) 
            execute(cpu,14,texto);// 14 = THREAD_EXIT

    else if (strcmp(texto[0], "PROCESS_EXIT") == 0 && texto[1] && texto[2]) 
            execute(cpu,15,texto);// 15 = PROCESS_EXIT
     else{
        log_info(logger, "Instrucción no reconocida: %s", texto[0]);
        exit(EXIT_FAILURE);
     }
    


}

// Función que ejecuta una instrucción
void execute(RegistroCPU *cpu, int instruccion, char **texto) {
    uint32_t *reg_destino = NULL;
    uint32_t *reg_origen = NULL;
    uint32_t *reg_direccion = NULL;
    uint32_t *reg_valor = NULL;
    uint32_t *reg_comparacion = NULL;

    switch (instruccion)
    {
        case 1: // SET
            reg_destino = registro_aux(cpu, texto[1]);
            if (reg_destino != NULL && texto[2]) 
            {
                reg_destino = (uint32_t) atoi(texto[2]); // Convierte el valor a entero y lo asigna al registro
                log_info(logger, "## Ejecutando: SET - Reg: %s, Valor: %u", texto[1], *reg_destino);
            } else 
                log_error(logger, "Error en SET: Registro no válido o valor no especificado");
        break;
        case 2: // SUM
            reg_destino = registro_aux(cpu, texto[1]);    
            reg_origen = registro_aux(cpu, texto[2]);
    
            if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino += *reg_origen; // Suma el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUM - Reg: %s, Nuevo Valor: %u", texto[1], *reg_destino);
            } else
                log_info(logger, "Error: Registro no válido en SUM");
        break;
        case 3: // SUB
            reg_destino = registro_aux(cpu,texto[1]);    
            reg_origen = registro_aux(cpu, texto[2]);

            if (reg_destino != NULL && reg_origen != NULL)
            {
                *reg_destino -= *reg_origen; // Resta el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUB - Reg: %s, Nuevo Valor: %u", texto[1], *reg_destino);
            }
            else 
                log_info(logger, "Error: Registro no válido en SUB");
        break;
        case 4: // READ MEM
            reg_destino = registro_aux(cpu, texto[1]); // Registro donde se guardará el valor leído
            reg_direccion = registro_aux(cpu, texto[2]); // Registro que contiene la dirección lógica
            if (reg_destino != NULL && reg_direccion != NULL) 
            {
                uint32_t direccion_fisica = 0;
                traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## LEER MEMORIA - Dirección Física: %u", direccion_fisica);

                // iría la lógica de leer de la memoria real y asignar al registro destino
                send_handshake = crear_paquete(READ_MEM);
                agregar_a_paquete(send_handshake,reg_direccion,sizeof(uint32_t));
                enviar_paquete(send_handshake,conexion_cpu_memoria);
                eliminar_paquete(send_handshake);
                t_list* paquete_recv = recibir_paquete(conexion_cpu_memoria);
                t_paquete* recv = list_remove(paquete_recv,0);
                *reg_destino = (uint32_t) recv->buffer->stream;
                list_destroy(paquete_recv);
                eliminar_paquete(recv);
            }    
                else 
                    log_info(logger, "Error: Registro no válido en READ_MEM");
        break;
        case 5: // WRITE MEM
            reg_direccion = registro_aux(cpu, texto[1]); // Dirección lógica
            reg_valor = registro_aux(cpu, texto[2]); // Registro con el valor a escribir
            if (reg_direccion != NULL && reg_valor != NULL) {
                    uint32_t direccion_fisica = 0;
                    traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                    send_handshake = crear_paquete(WRITE_MEM);
                    agregar_a_paquete(send_handshake,reg_direccion,sizeof(uint32_t));
                    agregar_a_paquete(send_handshake,reg_valor,sizeof(uint32_t));
                    enviar_paquete(send_handshake,conexion_cpu_memoria);
                    eliminar_paquete(send_handshake);
                    log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);
                    protocolo_socket cod_op;
                    cod_op = recibir_operacion(conexion_cpu_memoria);
                    switch (cod_op)
                    {
                        case OK:
                            log_info(logger, "## Operación recibida: %s", cod_op_a_string(cod_op));
                        break;
                        case SEGMENTATION_FAULT:
                            manejar_segmentation_fault(cpu);
                        break;
                        default:
                            log_info(logger,"Error: Registro no válido en WRITE_MEM");
                        break;
                    }
                }
        break;
        case 6: // JNZ
            reg_comparacion = registro_aux(cpu, texto[1]); // Registro a comparar con 0
            if (reg_comparacion != NULL && *reg_comparacion != 0) {
                cpu->PC = (uint32_t) atoi(texto[2]); // Actualiza el PC, que se supone que es entero el valor del PC
                log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu->PC);
            } else if (reg_comparacion == NULL) 
                log_info(logger, "Error: Registro no válido en JNZ");
            else 
                log_info(logger, "## JNZ - No se realiza salto porque el valor es 0");
        break;

        case 7: // MUTEX_CREATE
            send_handshake = crear_paquete(MUTEX_CREATE);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            nombre archivo texto[1]
            cod_op
            */
        break;

        case 8: // MUTEX_LOCK
            send_handshake = crear_paquete(MUTEX_LOCK);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            nombre archivo texto[1]
            cod_op
            */
        break;

        case 9: // DUMP_MEMORY
            send_handshake = crear_paquete(DUMP_MEMORY);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
        break;

        case 10: // PROCESS_CREATE
            send_handshake = crear_paquete(PROCESS_CREATE);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[2],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            nombre archivo texto[1]
            prioridad texto[2]
            */
        break;

        case 11: // THREAD_CREATE
            send_handshake = crear_paquete(THREAD_CREATE);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[2],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            nombre archivo texto[1]
            prioridad texto[2]
            */        
        break;
        case 12: // THREAD_CANCEL
            send_handshake = crear_paquete(THREAD_CANCEL);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            tid texto[1]
            */  
            
        break;

        case 13: // THREAD_JOIN
            send_handshake = crear_paquete(THREAD_JOIN);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            agregar_a_paquete(send_handshake,texto[1],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            tid texto[1]
            */ 
        break;

        case 14: // THREAD_EXIT
            send_handshake = crear_paquete(THREAD_EXIT);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            */ 
            
        break;

        case 15: // PROCESS_EXIT
            send_handshake = crear_paquete(PROCESS_EXIT);
            agregar_a_paquete(send_handshake,texto[0],sizeof(char**));
            enviar_paquete(send_handshake,conexion_kernel_interrupt);
            eliminar_paquete(send_handshake);
            /*
            cod_op
            */
        break;
        default:
            log_info(logger, "Error: Instrucción no reconocida");
        break;
    }
    cpu->PC++; // Incrementar el contador de programa
    checkInterrupt(cpu);
}

uint32_t* registro_aux(RegistroCPU *cpu, char* reg) {
    if (strcmp(reg, "AX") == 0)
        return &(cpu->AX);
    else if (strcmp(reg, "BX") == 0)
        return &(cpu->BX);
    else if (strcmp(reg, "CX") == 0)
        return &(cpu->CX);
    else if (strcmp(reg, "DX") == 0)
        return &(cpu->DX);
    else if (strcmp(reg, "EX") == 0)
        return &(cpu->EX);
    else if (strcmp(reg, "FX") == 0)
        return &(cpu->FX);
    else if (strcmp(reg, "GX") == 0)
        return &(cpu->GX);
    else if (strcmp(reg, "HX") == 0)
        return &(cpu->HX);
    return NULL; // En caso de que el registro no sea válido
    
}
void checkInterrupt(RegistroCPU *cpu) { //el checkInterrupt se corre siempre -> interrupcion -> tipo 


    t_interrupcion* interrupcion_actual = obtener_interrupcion_mayor_prioridad();
    if (interrupcion_actual == NULL)
        return;
    switch (interrupcion_actual->tipo) {
        case FIN_QUANTUM:
            log_info(logger, "## Interrupción FIN_QUANTUM recibida desde Kernel");
            manejar_fin_quantum(cpu);        
            break;
        case THREAD_JOIN_OP:
            log_info(logger, "## Interrupción THREAD_JOIN_OP recibida desde Kernel");
            manejar_thread_join(cpu);  
            break;
        case SEGMENTATION_FAULT:
            log_info(logger, "## Interrupción SEGMENTATION_FAULT recibida desde Kernel");
            manejar_segmentation_fault(cpu);    
        default:
                exit(EXIT_FAILURE);
           break;
        }
        // Verificar si la CPU generó alguna interrupción (ej: Segmentation Fault)
        
}


int cpu_genero_interrupcion(RegistroCPU *cpu) {
    // Verificar si se produjo un segmentation fault
    if (cpu->PC >= cpu->limite) {
        log_error(logger, "Segmentation Fault: PC (%d) fuera de los límites permitidos (%d)", cpu->PC, cpu->limite);
        return 1; // Retorna 1 indicando que se generó una interrupción crítica
    }

    // Verificar si se ejecutó una instrucción no válida
    if (cpu->base == 0 || cpu->limite == 0) { // CPU sin inicializar correctamente
        log_error(logger, "Error crítico: Contexto de CPU no inicializado correctamente (base: %d, límite: %d)", cpu->base, cpu->limite);
        return 1; // Indica interrupción crítica
    }

    // Si no se detectaron interrupciones críticas, retorna 0
    return 0;
}

void enviar_contexto_de_memoria(RegistroCPU *registro, int pid) {
    t_paquete *paquete_send = crear_paquete(CONTEXTO_SEND);
    
    // Agregar los registros de la CPU al paquete para enviarlos a la memoria
    agregar_a_paquete(paquete_send, registro, sizeof(RegistroCPU));  
    agregar_a_paquete(paquete_send, &pid, sizeof(int));  // Agregamos el PID del proceso

    // Enviar el paquete a la memoria
    enviar_paquete(paquete_send, conexion_cpu_memoria);
    eliminar_paquete(paquete_send);  // Eliminar el paquete después de enviarlo

    // Recibir confirmación de la memoria
    t_paquete *paquete_respuesta = recibir_paquete(conexion_cpu_memoria);
    if (paquete_respuesta != NULL && strcmp(paquete_respuesta->buffer, "OK") == 0) {
        log_info(logger, "Contexto de PID %d enviado correctamente a Memoria", pid);
    } else 
        log_error(logger, "Error al enviar contexto de PID %d a Memoria", pid);

    eliminar_paquete(paquete_respuesta);  // Limpiar paquete de respuesta
}

void notificar_kernel_interrupcion(int pid, int tid, protocolo_socket cod_op) {
    t_paquete *paquete_notify = crear_paquete(cod_op);

    // Enviamos el PID y TID para notificar al Kernel que el proceso fue interrumpido
    agregar_a_paquete(paquete_notify, &pid, sizeof(int));  
    agregar_a_paquete(paquete_notify, &tid, sizeof(int));  

    // Enviamos la notificación al Kernel
    enviar_paquete(paquete_notify, conexion_cpu_interrupt);  
    eliminar_paquete(paquete_notify);  // Eliminamos el paquete tras enviarlo

    log_info(logger, "Notificación enviada al Kernel por la interrupción del PID %d, TID %d", pid, tid);

    // Esperamos una confirmación de que el Kernel recibió la interrupción
    t_paquete *paquete_respuesta = recibir_paquete(conexion_cpu_interrupt);
    if (paquete_respuesta != NULL && strcmp(paquete_respuesta->buffer, "OK") == 0) {
        log_info(logger, "Kernel reconoció la interrupción para PID %d, TID %d", pid, tid);
    } else 
        log_error(logger, "Error al notificar al Kernel sobre la interrupción de PID %d, TID %d", pid, tid);
    
    eliminar_paquete(paquete_respuesta);  // Limpiar el paquete de respuesta
}

/*

void *peticion_kernel(void *args) {
    t_paquete_peticion *args_peticion = args;
    t_peticion *peticion = args_peticion->peticion;
    t_pcb *proceso = peticion->proceso;
    t_tcb *hilo = peticion->hilo;
    t_paquete *send_protocolo;
    protocolo_socket op;

    switch (peticion->tipo) {
        case PROCESS_CREATE_OP:
            send_protocolo = crear_paquete(PROCESS_CREATE_OP);
            agregar_a_paquete(send_protocolo, proceso, sizeof(t_pcb));
			log_info(logger, "Se crea la peticion de PROCESS CREATE");
            break;

        case PROCESS_EXIT_OP:
            send_protocolo = crear_paquete(PROCESS_EXIT_OP);
            agregar_a_paquete(send_protocolo, proceso, sizeof(t_pcb));
			log_info(logger, "Se crea la peticion de PROCESS EXIT");
            break;

        case THREAD_CREATE_OP:
            send_protocolo = crear_paquete(THREAD_CREATE_OP);
            agregar_a_paquete(send_protocolo, hilo, sizeof(t_tcb));
			log_info(logger, "Se crea la peticion de THREAD CREATE");
            break;

        case THREAD_EXIT_OP:
            send_protocolo = crear_paquete(THREAD_EXIT_OP);
            agregar_a_paquete(send_protocolo, hilo, sizeof(t_tcb));
			log_info(logger, "Se crea la peticion de THREAD EXIT");
            break;

        case DUMP_MEMORY_OP:
            send_protocolo = crear_paquete(DUMP_MEMORY_OP);
            agregar_a_paquete(send_protocolo, proceso, sizeof(t_pcb));
			log_info(logger, "Se crea la peticion de DUMP MEMORY");
            break;

        default:
            log_error(logger, "Tipo de operación desconocido: %d", peticion->tipo);
            return NULL;
    }

    enviar_paquete(send_protocolo, args_peticion->socket);
    log_info(logger, "Petición enviada a memoria, esperando respuesta...");

    // Esperar respuesta bloqueante
    op = recibir_operacion(args_peticion->socket);
    switch (op) {
        case SUCCESS:
            log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
            peticion->respuesta_exitosa = true;
            break;

        case ERROR:
            log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
            peticion->respuesta_exitosa = false;
            break;

        default:
            log_warning(logger, "Código de operación desconocido recibido: %d", op);
            peticion->respuesta_exitosa = false;
            break;
    }
	log_info(logger, "Actualizo el valor de respuesta recibida a true");
    peticion->respuesta_recibida = true; // Actualizar el estado directamente
    eliminar_paquete(send_protocolo);
    liberar_conexion(args_peticion->socket);
    return NULL;
}
*/

void agregar_interrupcion(protocolo_socket tipo, int prioridad, int tid) { // Las interrupciones se pueden generar tanto por la CPU como por el Kernel. Las añadimos a la lista de interrupciones con su respectiva prioridad.
    if (tid_actual != tid){
        log_info(logger, "El tid de la interrupcion no es el mismo del hilo actual");
    }
    else{
        t_interrupcion* nueva_interrupcion = malloc(sizeof(t_interrupcion));
        nueva_interrupcion->tipo = tipo;
        nueva_interrupcion->prioridad = prioridad; // aca no importa porque es entero xd
        
        // Insertamos la interrupción en la lista
        list_add(lista_interrupciones, nueva_interrupcion);
        sem_post(sem_lista_interrupciones);
        log_info(logger, "Interrupción agregada: %s con prioridad %d", tipo, prioridad);
        free(nueva_interrupcion);
    }

}

t_interrupcion* obtener_interrupcion_mayor_prioridad() {
    if (list_is_empty(lista_interrupciones)) 
        return NULL;

    t_interrupcion* interrupcion_mayor_prioridad = list_get(lista_interrupciones, 0);  // Suponemos que hay al menos una interrupción
    int indice_mayor_prioridad = 0;

    // Recorrer la lista para encontrar la interrupción con mayor prioridad (menor valor numérico)
    for (int i = 1; i < list_size(lista_interrupciones); i++) {
        t_interrupcion* interrupcion_actual = list_get(lista_interrupciones, i);
        if (interrupcion_actual->prioridad < interrupcion_mayor_prioridad->prioridad) { //Si encontramos una interrupción con menor prioridad numérica (prioridad más alta), actualizamos nuestro candidato.
            interrupcion_mayor_prioridad = interrupcion_actual;
            indice_mayor_prioridad = i;
        }
    }
    
    // Una vez que encontramos la interrupción con mayor prioridad, la eliminamos de la lista
        list_remove_and_destroy_element(lista_interrupciones, indice_mayor_prioridad, (void) liberar_interrupcion());


    return interrupcion_mayor_prioridad;
}

void liberar_interrupcion(t_interrupcion* interrupcion) {
    if (interrupcion != NULL) {
        free(interrupcion);  // Liberamos la estructura
    }
}

void manejar_segmentation_fault(RegistroCPU *cpu) {
    log_info(logger, "## Manejo de Segmentation Fault");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel del error crítico
    notificar_kernel_interrupcion(pid, tid,SEGMENTATION_FAULT);

    // Detener ejecución del hilo actual
    detener_ejecucion();
}

void manejar_fin_quantum(RegistroCPU *cpu) {
    log_info(logger, "## Manejo de Fin de Quantum");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel del fin del Quantum
    notificar_kernel_interrupcion(pid, tid,FIN_QUANTUM);
    
    detener_ejecucion();
}

void manejar_thread_join(RegistroCPU *cpu) {
    log_info(logger, "## Manejo de Thread Join");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel que se ejecutó la Syscall Join
    notificar_kernel_interrupcion(pid, tid,THREAD_JOIN_OP);

    detener_ejecucion();
}

void manejar_io_syscall(RegistroCPU *cpu){
    log_info(logger, "## Manejo de syscall io");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel que se ejecutó la io syscall
    notificar_kernel_interrupcion(pid, tid,IO_SYSCALL);

    detener_ejecucion();
}
void manejar_finalizacion(cpu){
    log_info(logger, "## Manejo de Finalización");

    // Notificar al Kernel que el hilo finalizó
    notificar_kernel_interrupcion(pid, tid, FINALIZACION);

}
void manejar_segmentation_fault(RegistroCPU *cpu){
    enviar_contexto_de_memoria(cpu, pid);

 // Notificar al Kernel del error crítico
    notificar_kernel_interrupcion(pid, tid, SEGMENTATION_FAULT);
    
    detener_ejecucion();
}
void detener_ejecucion(){
    pid = 0;
}