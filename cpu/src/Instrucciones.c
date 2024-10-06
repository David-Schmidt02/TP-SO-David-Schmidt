#include <instrucciones.h>

extern t_log *logger;
extern int conexion_cpu_memoria;
extern int pid;
extern int tid;
RegistroCPU cpu;
t_paquete *send_handshake;
t_paquete *paquete_respuesta;

uint32_t* registro_aux(RegistroCPU *cpu, char* reg) {
    if  (strcmp(reg,'AX'))
        return &(cpu->AX);
    else if  (strcmp(reg,'BX'))
        return &(cpu->BX);
    else if  (strcmp(reg,'CX'))
        return &(cpu->CX);
    else if  (strcmp(reg,'DX'))
        return &(cpu->DX);
    else if  (strcmp(reg,'EX'))
        return &(cpu->EX);
    else if  (strcmp(reg,'FX'))
        return &(cpu->FX);
    else if  (strcmp(reg,'GX'))
        return &(cpu->GX);
    else if  (strcmp(reg,'HX'))
        return &(cpu->HX);
    return NULL; // En caso de que el registro no sea válido
    
}



// Función para obtener el contexto de ejecución de la memoria
void obtener_contexto_de_memoria (t_pcb *pcb, int tid) {
    // Crear un paquete de solicitud para obtener el contexto
    send_handshake = crear_paquete(CONTEXTO); // le pondria handshake?, como se que recibo?
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); // Agregar TID

    // Enviar la solicitud para obtener el contexto
    enviar_paquete(send_handshake, conexion_cpu_memoria);
    eliminar_paquete(send_handshake); // elimina el paquete después de enviarlo

    // Recibir la respuesta
    paquete_respuesta = recibir_paquete(conexion_cpu_memoria); // Recibir el contexto de ejecución
    if (paquete_respuesta != NULL) 
    {
        memcpy(&(pcb->registro), paquete_respuesta->buffer, sizeof(pcb->registro));
    
        // Log
        log_info(logger, "## TID: %d - Contexto inicializado" , pcb->pid);
        log_info(logger, "%d", pcb->pid);

    } else 
        log_info(logger,"Error: No se recibió contexto de ejecución.");

    // Cerrar conexión
    liberar_conexion(conexion_cpu_memoria);

    log_info(logger, "## TID: %d - Solicito Contexto Ejecución", tid); 
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
    if (strcmp(texto[0], "SET") == 0) 
    {
        // Instrucción SET: Asigna un valor a un registro
        if (inst->parametros_validos == 2) // Se necesitan 2 parámetros
        { 
            uint32_t *reg_destino = registro_aux(cpu, texto[1]);
            if (reg_destino != NULL) 
            {
                *reg_destino = texto[2]; // Asigna el valor al registro
                log_info(logger, "## Ejecutando: SET - Reg: %d, Valor: %d", inst->parametros[0], inst->parametros[1]);
            } else 
                log_info(logger, "Error: Registro no válido: %d", inst->parametros[0]);
        }
    } 
    else if (strcmp(texto[0], "SUM") == 0) 
    {
        // Instrucción SUM: Suma el valor de un registro a otro   
        if (inst->parametros_validos == 2) // Se necesitan 2 parámetros
        { 
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
        }
    }
    else if (strcmp(texto[0], "SUB") == 0) 
    {
        // Instrucción SUB: Resta el valor de un registro a otro
        if (inst->parametros_validos == 2) 
        {
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
        }
    } 
    else if (strcmp(texto[0], "READ_MEM") == 0) 
    {
        // Instrucción READ_MEM: Lee de memoria a un registro
        if (inst->parametros_validos == 2) 
        {
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
        }
    }
    else if (strcmp(texto[0], "WRITE_MEM") == 0) 
    {
        // Instrucción WRITE_MEM: Escribe en memoria desde un registro
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_direccion = registro_aux(cpu, texto[1]); // Dirección lógica
            uint32_t *reg_valor = registro_aux(cpu, texto[2]); // Registro con el valor a escribir
            if (reg_direccion != NULL && reg_valor != NULL) {
                uint32_t direccion_fisica = 0;
                traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);

                // iría la lógica de escribir en la memoria real
            } else 
                log_info(logger,"Error: Registro no válido en WRITE_MEM");
        }
    }
    else if (strcmp(texto[0], "JNZ") == 0) 
    {
        // Instrucción JNZ: Salta si el valor de un registro no es cero
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_comparacion = registro_aux(cpu, texto[1]); // Registro a comparar con 0
            if (reg_comparacion != NULL && *reg_comparacion != 0) 
            {
                cpu->PC = texto[2]; // Actualiza el PC 
                log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu->PC);
            } else if (reg_comparacion == NULL) 
                log_info(logger, "Error: Registro no válido en JNZ");
        }
    } else
        log_info(logger, "Instrucción no reconocida: %s", strcmp(texto[0]);

}



// Función que ejecuta una instrucción
void execute(RegistroCPU *cpu) {
    fetch(cpu); // Obtener la instrucción actual
    cpu->PC++; // Incrementar el contador de programa
}

void checkInterrupt(RegistroCPU *cpu) {

}

// Función que inicia el ciclo de ejecución de la CPU
void runCPU(RegistroCPU *cpu) {
    while (cpu->PC < memoria.cont) { // Mientras haya instrucciones
        execute(cpu); // Ejecuta la instrucción
        checkInterrupt(cpu); // Verifica por interrupciones
    }
}

