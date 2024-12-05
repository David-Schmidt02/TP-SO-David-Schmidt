#include <instrucciones.h>

extern t_log *logger;
extern int conexion_cpu_memoria;
extern int conexion_cpu_dispatch;
extern int conexion_cpu_interrupt;
extern int pid;
extern int tid;
RegistroCPU cpu;
t_paquete *send_handshake;
t_paquete *paquete_respuesta;
t_list* lista_interrupciones;
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
    cpu->limit = 0;
    memset(cpu->instruccion_actual, 0, sizeof(cpu->instruccion_actual));
    log_info(logger, "Contexto de CPU inicializado");
}

// Función para obtener el contexto de ejecución de la memoria
void obtener_contexto_de_memoria (RegistroCPU *registro, int pid) {
    t_paquete *msjerror;
    t_list *paquete_respuesta;
    // Crear un paquete de solicitud para obtener el contexto
    send_handshake = crear_paquete(CONTEXTO_RECEIVE); // le pondria handshake?, como se que recibo?
    agregar_a_paquete(send_handshake, &pid, sizeof(tid)); // Agregar TID

    // Enviar la solicitud para obtener el contexto
    enviar_paquete(send_handshake, conexion_cpu_memoria);
    eliminar_paquete(send_handshake); // elimina el paquete después de enviarlo

    // Recibir la respuesta
    if (recibir_operacion(conexion_cpu_memoria)==ERROR_MEMORIA){
        paquete_respuesta = recibir_paquete(conexion_cpu_memoria);
        msjerror = list_remove(paquete_respuesta, 0);
        char *error;
        memcpy(error, msjerror->buffer, sizeof(msjerror->buffer));
        log_error(logger, error);
        agregar_interrupcion(SEGMENTATION_FAULT,1);
    }
    paquete_respuesta = recibir_paquete(conexion_cpu_memoria); // Recibir el contexto de ejecución
    if (paquete_respuesta != NULL) 
    {
        memcpy(&(registro->registro), paquete_respuesta->buffer, sizeof(paquete_respuesta->buffer));
    
        // Log
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
        paquete_respuesta = recibir_paquete(conexion_cpu_memoria)
        msjerror = list_remove(paquete_respuesta, 0);
        char *error;
        memcpy(error, msjerror->buffer, sizeof(msjerror->buffer));
        log_error(logger, error);
        agregar_interrupcion(SEGMENTATION_FAULT,1);
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
        memcpy(instruccion, paquete_request->buffer, sizeof(paquete_request->buffer));
        if (strcmp(instruccion, "SEGMENTATION FAULT"))
            // se genera interrupcion
        // Registro de la acción de fetch en el log
        log_info(logger,"## TID: %d - FETCH ", cpu->PC);
        
        // Llamar a la función decode para procesar la instrucción
        decode(cpu, instruccion);
   
}

// Función para traducir direcciones lógicas a físicas
void traducir_direccion(RegistroCPU *cpu, uint32_t dir_logica, uint32_t *dir_fisica) 
{
    *dir_fisica = cpu->base + dir_logica; // Traducción a dirección física
    if (*dir_fisica >= cpu->base + cpu->limit) { // Validación de segmento
        log_info(logger,"Error: Segmentation Fault (Acceso fuera de límites)\n");
        exit(SEGMENTATION_FAULT); // Sale por error
    }
}

// Función para decodificar una instrucción y ejecutarla en los registros de la CPU
void decode(RegistroCPU *cpu, char *inst) {
    char *texto[5] = string_split(inst," "); //5 porque hay instrucciiones que poseen 4 valores: PROCESS_CREATE(0) PROCESO1(1) 256(2) 1(3), la 5ta es pos NULL. ejemplo: string_split("hola, mundo", ",") => ["hola", " mundo", NULL]
    // Compara el ID de la instrucción y ejecuta la operación correspondiente
    if (strcmp(texto[0], "SET") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción SET: Asigna un valor a un registro
        if (inst->parametros_validos == 2) // Se necesitan 2 parámetros 
            execute(cpu,1,texto); // 1 = SET
    } 
    else if (strcmp(texto[0], "SUM") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción SUM: Suma el valor de un registro a otro   
        if (inst->parametros_validos == 2 ) // Se necesitan 2 parámetros
            execute(cpu,2,texto); // 2 = SUM
    }
    else if (strcmp(texto[0], "SUB") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción SUB: Resta el valor de un registro a otro
        if (inst->parametros_validos == 2) 
            execute(cpu,3,texto);//3 = SUB
    } 
    else if (strcmp(texto[0], "READ_MEM") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción READ_MEM: Lee de memoria a un registro
        if (inst->parametros_validos == 2) 
            execute(cpu,4,texto); // 4 = READ_MEM

    }
    else if (strcmp(texto[0], "WRITE_MEM") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción WRITE_MEM: Escribe en memoria desde un registro
        if (inst->parametros_validos == 2) 
            execute(cpu,5,texto); // 5 = WRITE_MEM
    }
    else if (strcmp(texto[0], "JNZ") == 0 && texto[1] && texto[2]) 
    {
        // Instrucción JNZ: Salta si el valor de un registro no es cero
        if (inst->parametros_validos == 2) 
            execute(cpu,6,texto); // 6 = JNZ
    } else
        log_info(logger, "Instrucción no reconocida: %s", texto[0];
        // interrupcion
}

// Función que ejecuta una instrucción
void execute(RegistroCPU *cpu, int instruccion, char *texto) {
    switch (instruccion)
    {
        case 1: // SET
            uint32_t *reg_destino = registro_aux(cpu, texto[1]);
            if (reg_destino != NULL) 
            {
                *reg_destino = texto[2]; // Asigna el valor al registro
                log_info(logger, "## Ejecutando: SET - Reg: %d, Valor: %d", inst->parametros[0], inst->parametros[1]);
            } else 
                log_info(logger, "Error: Registro no válido: %d", inst->parametros[0]);
        break;
        case 2: // SUM
            uint32_t *reg_destino = registro_aux(cpu,texto[1]);    
            uint32_t *reg_origen = registro_aux(cpu, texto[2]);

            if (reg_destino != NULL && reg_origen == NULL)
            {
                reg_destino += *reg_origen; // Suma el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUM - Reg: %d, Nuevo Valor: %d", texto[1], *reg_destino);
            }      
            else if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino += *reg_origen; // Suma el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUM - Reg: %d, Nuevo Valor: %d", texto[1], *reg_destino);
            } else
                log_info(logger, "Error: Registro no válido en SUM");
        break;
        case 3: // SUB
            uint32_t *reg_destino = registro_aux(cpu,texto[1]);    
            uint32_t *reg_origen = registro_aux(cpu, texto[2]);
            if (reg_destino != NULL && reg_origen == NULL)
            {
                *reg_destino -= *reg_origen; // Resta el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUB - Reg: %d, Nuevo Valor: %d", texto[1], *reg_destino);
            }
            if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino -= *reg_origen; // Resta el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUB - Reg: %d, Nuevo Valor: %d", texto[1], *reg_destino);
            } else 
                log_info(logger, "Error: Registro no válido en SUB");
        break;
        case 4: // READ MEM
            uint32_t *reg_destino = registro_aux(cpu, texto[1]); // Registro donde se guardará el valor leído
            uint32_t *reg_direccion = registro_aux(cpu, texto[2]); // Registro que contiene la dirección lógica
            if (reg_destino != NULL && reg_direccion != NULL) 
            {
                uint32_t direccion_fisica = 0;
                traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## LEER MEMORIA - Dirección Física: %u", direccion_fisica);

                // iría la lógica de leer de la memoria real y asignar al registro destino
                *reg_destino = 1234; // Simulación de lectura de memoria, asignamos un valor ficticio
            } else 
                log_info(logger, "Error: Registro no válido en READ_MEM");
        break;
        case 5: // WRITE MEM
            uint32_t *reg_direccion = registro_aux(cpu, texto[1]); // Dirección lógica
            uint32_t *reg_valor = registro_aux(cpu, texto[2]); // Registro con el valor a escribir
            if (reg_direccion != NULL && reg_valor != NULL) {
                uint32_t direccion_fisica = 0;
                traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);

                // iría la lógica de escribir en la memoria real
            } else 
                log_info(logger,"Error: Registro no válido en WRITE_MEM");
        break;
        case 6: // JNZ
            uint32_t *reg_comparacion = registro_aux(cpu, texto[1]); // Registro a comparar con 0
            if (reg_comparacion != NULL && *reg_comparacion != 0) 
            {
                cpu->PC = atoi(texto[2]); // Actualiza el PC, que se supone que es entero el valor del PC
                log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu->PC);
            } else if (reg_comparacion == NULL) 
                log_info(logger, "Error: Registro no válido en JNZ");
        break;
    default:
            log_info(logger, "Error: Instrucción no reconocida");
        break;
    }
    cpu->PC++; // Incrementar el contador de programa
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
void checkInterrupt(RegistroCPU *cpu) { 
    int tipo_interrupcion = recibir_interrupcion();
    switch (tipo_interrupcion) {
        case FIN_QUANTUM:
            log_info(logger, "## Interrupción FIN_QUANTUM recibida desde Kernel");
            agregar_interrupcion("FIN_QUANTUM", 3);  // Prioridad baja
            break;

        case THREAD_JOIN_OP:
            log_info(logger, "## Interrupción THREAD_JOIN_OP recibida desde Kernel");
            agregar_interrupcion("THREAD_JOIN_OP", 2);  // Prioridad media
            break;

        case INTERRUPCION:
            log_info(logger, "## Interrupción genérica desde Kernel");
            agregar_interrupcion("INTERRUPCION", 4);  // Prioridad baja-media
            break;
    }

    // Verificar si la CPU generó alguna interrupción (ej: Segmentation Fault)
    if (cpu_genero_interrupcion()) {  // esta función determina si la CPU generó una interrupción crítica
        log_info(logger, "## Llega interrupción de la CPU");
        agregar_interrupcion("SEGMENTATION_FAULT", 1);  // Interrupción crítica de la CPU, prioridad alta
    }

    // Obtener la interrupción de mayor prioridad
    t_interrupcion* interrupcion_actual = obtener_interrupcion_mayor_prioridad();
    
    if (interrupcion_actual != NULL) {
        if (strcmp(interrupcion_actual->tipo, "SEGMENTATION_FAULT") == 0) {
            manejar_segmentation_fault(cpu);
        } else if (strcmp(interrupcion_actual->tipo, "THREAD_JOIN_OP") == 0) {
            manejar_thread_join(cpu);
        } else if (strcmp(interrupcion_actual->tipo, "FIN_QUANTUM") == 0) {
            manejar_fin_quantum(cpu);
        } else if (strcmp(interrupcion_actual->tipo, "IO_SYSCALL") == 0) {
            manejar_io_syscall(cpu);
        }
        liberar_interrupcion(interrupcion_actual);
    }
}

int cpu_genero_interrupcion(RegistroCPU *cpu) {
    // Verificar si se produjo un segmentation fault
    if (cpu->PC >= cpu->limit) {
        log_error(logger, "Segmentation Fault: PC (%d) fuera de los límites permitidos (%d)", cpu->PC, cpu->limit);
        return 1; // Retorna 1 indicando que se generó una interrupción crítica
    }

    // Verificar si se ejecutó una instrucción no válida
    if (cpu->base == 0 || cpu->limit == 0) { // CPU sin inicializar correctamente
        log_error(logger, "Error crítico: Contexto de CPU no inicializado correctamente (base: %d, límite: %d)", cpu->base, cpu->limit);
        return 1; // Indica interrupción crítica
    }

    // Si no se detectaron interrupciones críticas, retorna 0
    return 0;
}


int recibir_interrupcion() {
    t_paquete *paquete_interrupcion = recibir_paquete(conexion_cpu_interrupt);

    if (paquete_interrupcion == NULL) {
        return 0;  // No se recibió ninguna interrupción
    }

    char *tipo_interrupcion = paquete_interrupcion->buffer;

    if (strcmp(tipo_interrupcion, "FIN_QUANTUM") == 0) {
        log_info(logger, "Interrupción recibida: FIN_QUANTUM");

        agregar_interrupcion("FIN_QUANTUM", 4);  
        eliminar_paquete(paquete_interrupcion);
        return 1;
    } 
    else if (strcmp(tipo_interrupcion, "THREAD_JOIN_OP") == 0) {
        log_info(logger, "Interrupción recibida: THREAD_JOIN_OP");
        agregar_interrupcion("THREAD_JOIN_OP", 2);  
        eliminar_paquete(paquete_interrupcion);
        return 1;
    }
    else if (strcmp(tipo_interrupcion, "SYS_IO") == 0) {
        log_info(logger, "Interrupción recibida: SYS_IO");
        agregar_interrupcion("SYS_IO", 2);  
        eliminar_paquete(paquete_interrupcion);
        return 1;
    } 
    /*else if (strcmp(tipo_interrupcion, "INTERRUPCION") == 0) {
        log_info(logger, "Interrupción recibida: INTERRUPCION_KERNEL");
        agregar_interrupcion("INTERRUPCION", 4);  
        eliminar_paquete(paquete_interrupcion);
        return 1;
    }*/ 
    else {
        log_warning(logger, "Interrupción desconocida: %s", tipo_interrupcion);
        eliminar_paquete(paquete_interrupcion);
        return 0;
    }
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

void agregar_interrupcion(char* tipo, int prioridad) { // Las interrupciones se pueden generar tanto por la CPU como por el Kernel. Las añadimos a la lista de interrupciones con su respectiva prioridad.
    t_interrupcion* nueva_interrupcion = malloc(sizeof(t_interrupcion));
    nueva_interrupcion->tipo = strdup(tipo); // si no uso el strdup, tanto "nueva_interrupcion->tipo" como "tipo" apuntarian al mismo lugar, por lo que si se modifica uno se modifica el otro; 
    nueva_interrupcion->prioridad = prioridad; // aca no importa porque es entero xd
    
    // Insertamos la interrupción en la lista
    list_add(lista_interrupciones, nueva_interrupcion);
    log_info(logger, "Interrupción agregada: %s con prioridad %d", tipo, prioridad);
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
        list_remove_and_destroy_element(lista_interrupciones, indice_mayor_prioridad, (void*) liberar_interrupcion());


    return interrupcion_mayor_prioridad;
}

void liberar_interrupcion(t_interrupcion* interrupcion) {
    if (interrupcion != NULL) {
        free(interrupcion->tipo);  // Liberamos la cadena duplicada con strdup
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
}

void manejar_thread_join(RegistroCPU *cpu) {
    log_info(logger, "## Manejo de Thread Join");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel que se ejecutó la Syscall Join
    notificar_kernel_interrupcion(pid, tid,THREAD_JOIN_OP);
}

manejar_io_syscall(RegistroCPU *cpu){
    log_info(logger, "## Manejo de syscall io");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel que se ejecutó la io syscall
    notificar_kernel_interrupcion(pid, tid,IO_SYSCALL);
}
/*void manejar_interrupcion_kernel(RegistroCPU *cpu) {
    log_info(logger, "## Manejo de interrupción genérica enviada por el Kernel");

    // Guardar contexto en Memoria
    enviar_contexto_de_memoria(cpu, pid);

    // Notificar al Kernel que la interrupción fue manejada
    notificar_kernel_interrupcion(pid, tid,INTERRUPCION);
}
*/