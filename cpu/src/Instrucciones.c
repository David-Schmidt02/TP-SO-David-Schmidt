#include <instrucciones.h>

t_log *logger;
t_paquete *send_handshake;
t_paquete *paquete_respuesta;

logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_DEBUG);
t_config *config = config_create("config/cpu.config");


// Función para obtener el contexto de ejecución de la memoria
void obtener_contexto_de_memoria (t_pcb *pcb, int tid) {
    // Crear un paquete de solicitud para obtener el contexto
    send_handshake = crear_paquete(HANDSHAKE); // le pondria handshake?, como se que recibo?
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); // Agregar TID

    // Enviar la solicitud para obtener el contexto
    enviar_paquete(send_handshake, conexion_cpu_memoria);
    eliminar_paquete(send_handshake); // elimina el paquete después de enviarlo

    // Recibir la respuesta
    paquete_respuesta = recibir_paquete(conexion_cpu_memoria); // Recibir el contexto de ejecución
    if (paquete_respuesta != NULL) 
    {
        memcpy(&(pcb->registro->base), paquete_respuesta->buffer, sizeof(pcb->registro->base));
        memcpy(&(pcb->registro->limite), paquete_respuesta->buffer + sizeof(pcb->registro->base), sizeof(pcb->registro->limite));
    
        // Log
        log_info(logger, %d "## TID: %d - Contexto inicializado - Base: %u, Límite: %u", pcb->registro->ID_instruccion, tid, pcb->registro->base, pcb->registro->limite);
        log_info(logger, %d, pcb->registro->ID_instruccion);

    } else 
        log_info(logger,"Error: No se recibió contexto de ejecución.");

    // Cerrar conexión
    liberar_conexion(conexion_cpu_memoria);

    log_info(sprintf(cpu->registers, "## TID: %d - Solicito Contexto Ejecución", tid)); 
}

// Función para obtener la próxima instrucción
void fetch(t_pcb *pcb) {
    // Verificar que el PC esté dentro del rango de instrucciones válidas
    if (pcb->registro->PC < memoria.cont) 
    {
        // Obtención de la instrucción correspondiente al Program Counter
        t_instruccion inst = memoria.instrucciones[pcb->registro->PC]; //en mi cabeza tiene sentido jajaja

        // Registro de la acción de fetch en el log
         log_info(logger,"## TID: %d - FETCH - Program Counter: %u", pcb->registro->PC, inst.ID_instruccion);
        
        // Llamar a la función decode para procesar la instrucción
        decode(pcb, &inst);
    } else 
        // Manejo de Segmentation Fault si PC está fuera de límites
        log_info(logger,"Error: Segmentation Fault - Acceso fuera de límites");    
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
void decode(RegistroCPU *cpu, t_instruccion *inst) {

    // Compara el ID de la instrucción y ejecuta la operación correspondiente
    if (strcmp(inst->ID_instruccion, "SET") == 0) 
    {
        // Instrucción SET: Asigna un valor a un registro
        if (inst->parametros_validos == 2) // Se necesitan 2 parámetros
        { 
            uint32_t *reg_destino = registro_aux(cpu, inst->parametros[0]);
            if (reg_destino != NULL) 
            {
                *reg_destino = inst->parametros[1]; // Asigna el valor al registro
                log_info(logger, "## Ejecutando: SET - Reg: %d, Valor: %d", inst->parametros[0], inst->parametros[1]);
            } else 
                log_info(logger, "Error: Registro no válido: %d", inst->parametros[0]);
        }
    } 
    else if (strcmp(inst->ID_instruccion, "SUM") == 0) 
    {
        // Instrucción SUM: Suma el valor de un registro a otro
        if (inst->parametros_validos == 2) // Se necesitan 2 parámetros
        { 
            uint32_t *reg_destino = registro_aux(cpu, inst->parametros[0]);
            uint32_t *reg_origen = registro_aux(cpu, inst->parametros[1]);
            if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino += *reg_origen; // Suma el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUM - Reg: %d, Nuevo Valor: %d", inst->parametros[0], *reg_destino);
            } else 
                log_info(logger, "Error: Registro no válido en SUM");
        }
    }
    else if (strcmp(inst->ID_instruccion, "SUB") == 0) 
    {
        // Instrucción SUB: Resta el valor de un registro a otro
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_destino = registro_aux(cpu, inst->parametros[0]);
            uint32_t *reg_origen = registro_aux(cpu, inst->parametros[1]);
            if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino -= *reg_origen; // Resta el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUB - Reg: %d, Nuevo Valor: %d", inst->parametros[0], *reg_destino);
            } else 
                log_info(logger, "Error: Registro no válido en SUB");
        }
    } 
    else if (strcmp(inst->ID_instruccion, "READ_MEM") == 0) 
    {
        // Instrucción READ_MEM: Lee de memoria a un registro
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_destino = registro_aux(cpu, inst->parametros[0]); // Registro donde se guardará el valor leído
            uint32_t *reg_direccion = registro_aux(cpu, inst->parametros[1]); // Registro que contiene la dirección lógica
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
    else if (strcmp(inst->ID_instruccion, "WRITE_MEM") == 0) 
    {
        // Instrucción WRITE_MEM: Escribe en memoria desde un registro
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_direccion = registro_aux(cpu, inst->parametros[0]); // Dirección lógica
            uint32_t *reg_valor = registro_aux(cpu, inst->parametros[1]); // Registro con el valor a escribir
            if (reg_direccion != NULL && reg_valor != NULL) {
                uint32_t direccion_fisica = 0;
                traducir_direccion(cpu, *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);

                // iría la lógica de escribir en la memoria real
            } else 
                log_info(logger,"Error: Registro no válido en WRITE_MEM");
        }
    }
    else if (strcmp(inst->ID_instruccion, "JNZ") == 0) 
    {
        // Instrucción JNZ: Salta si el valor de un registro no es cero
        if (inst->parametros_validos == 2) 
        {
            uint32_t *reg_comparacion = registro_aux(cpu, inst->parametros[0]); // Registro a comparar con 0
            if (reg_comparacion != NULL && *reg_comparacion != 0) 
            {
                cpu->PC = inst->parametros[1]; // Actualiza el PC 
                log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu->PC);
            } else if (reg_comparacion == NULL) 
                log_info(logger, "Error: Registro no válido en JNZ");
        }
    } else
        log_info(logger, "Instrucción no reconocida: %s", inst->ID_instruccion);

}



// Función que ejecuta una instrucción
void execute(RegistroCPU *cpu) {
    fetch(cpu); // Obtener la instrucción actual
    cpu->PC++; // Incrementar el contador de programa
}

// Función que chequea interrupciones (no implementado en este código)
void checkInterrupt(RegistroCPU *cpu) {
    // Interacción con el Kernel para verificar interrupciones
}

// Función que inicia el ciclo de ejecución de la CPU
void runCPU(RegistroCPU *cpu) {
    while (cpu->PC < memoria.cont) { // Mientras haya instrucciones
        execute(cpu); // Ejecuta la instrucción
        checkInterrupt(cpu); // Verifica por interrupciones
    }
}

