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

// Función para decodificar una instrucción
void decode(RegistroCPU *cpu, t_instrucciones *inst) 
{
    if (strcmp(inst->instruction, "SET") == 0) 
    {
        cpu->registers[inst->params[0]] = inst->params[1]; // Asignar valor al registro
        log_info(logger, "## TID: %d - Ejecutando: SET - Reg: %d, Valor: %d", 0, inst->params[0], inst->params[1]);
        
    } 
    else if (strcmp(inst->instruction, "SUM") == 0) 
    {
        cpu->registers[inst->params[0]] += cpu->registers[inst->params[1]]; // Suma registros
        log_info(logger, "## TID: %d - Ejecutando: SUM - Reg: %d, Nueva Valor: %d", 0, inst->params[0], cpu->registers[inst->params[0]]);
        
        
    } 
    else if (strcmp(inst->instruction, "SUB") == 0) 
    {
        cpu->registers[inst->params[0]] -= cpu->registers[inst->params[1]]; // Resta registros
        log_info(logger, "## TID: %d - Ejecutando: SUB - Reg: %d, Nueva Valor: %d", 0, inst->params[0], cpu->registers[inst->params[0]]);
    } 
    else if (strcmp(inst->instruction, "READ_MEM") == 0) 
    {
        uint32_t dir_logica = cpu->registers[inst->params[1]]; // Dirección lógica
        uint32_t dir_fisica; // Dirección física a llenar
        memoryTranslation(cpu, dir_logica, &dir_fisica); // Traducción
        cpu->registers[inst->params[0]] = 0; // Simulando la lectura, asignar un valor
        log_info(logger, "## TID: %d - Acción: LEER - Dirección Física: %d", 0, dir_fisica); // Log de acción
    } 
    else if (strcmp(inst->instruction, "WRITE_MEM") == 0) 
    {
        uint32_t dir_logica = cpu->registers[inst->params[0]]; // Dirección lógica
        uint32_t dir_fisica; 
        memoryTranslation(cpu, dir_logica, &dir_fisica); 
        log_info(logger, "## TID: %d - Acción: ESCRIBIR - Dirección Física: %d, Valor: %d", 0, physicalAddress, cpu->registers[inst->params[1]]);
    } 
    else if (strcmp(inst->instruction, "JNZ") == 0) 
    {
        if (cpu->registers[inst->params[0]] != 0) // Verifica si el registro no es cero
        { 
            cpu->PC = inst->params[1]; // Cambiar el PC
            log_info(logger, "## TID: %d - JNZ - Nuevo PC: %d", 0, cpu->PC); 
        }
    } 

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

