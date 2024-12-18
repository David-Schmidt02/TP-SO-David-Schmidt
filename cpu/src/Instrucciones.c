#include <instrucciones.h>
extern pthread_mutex_t * mutex_fetch;

extern t_log *logger;

//Para manejar los valores actuales del hilo/proceso ejecutandose actualmente
extern int pid_actual;
extern int tid_actual;
extern int tid_de_interrupcion_FIN_QUANTUM;

extern uint32_t base_actual;
extern uint32_t limite_actual;
extern RegistroCPU * cpu_actual;
//

extern int flag_hay_contexto;

//Para manejar la lista de interrupciones
extern t_list* lista_interrupciones;
extern sem_t * sem_lista_interrupciones;
extern sem_t * sem_hay_contexto;
extern pthread_mutex_t *mutex_lista_interrupciones;
//

//Para manejar que las conexiones se hayan realizado
extern sem_t * sem_conexion_kernel_interrupt;
extern sem_t * sem_conexion_kernel_cpu_dispatch;
extern sem_t * sem_conexion_memoria;
extern sem_t * sem_se_detuvo_ejecucion;
//

//Para manejar las conexiones con mutexs
extern pthread_mutex_t *mutex_kernel_interrupt;
extern pthread_mutex_t *mutex_kernel_dispatch;
extern pthread_mutex_t *mutex_conexion_memoria;
//

//Sockets de las conexiones
extern int socket_conexion_kernel_interrupt;
extern int socket_conexion_kernel_dispatch;
extern int socket_conexion_memoria;
 //

void inicializar_registros_cpu() {
    cpu_actual = malloc(sizeof(RegistroCPU));
    cpu_actual->AX = 0;
    cpu_actual->BX = 0;
    cpu_actual->CX = 0;
    cpu_actual->DX = 0;
    cpu_actual->EX = 0;
    cpu_actual->FX = 0;
    cpu_actual->GX = 0;
    cpu_actual->HX = 0;
    cpu_actual->PC = 0;

    

    pid_actual = 0;
    tid_actual = -1; 

    log_info(logger, "Contexto de CPU inicializado");
    sem_post(sem_se_detuvo_ejecucion);
}

void inicializar_semaforos_cpu(){
    mutex_lista_interrupciones = malloc(sizeof(pthread_mutex_t));
    if (mutex_lista_interrupciones == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_lista_interrupciones, NULL);

    sem_hay_contexto = malloc(sizeof(sem_t));
    if (sem_hay_contexto == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_hay_contexto, 0, 1);

    sem_conexion_memoria = malloc(sizeof(sem_t));
    if (sem_conexion_memoria == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_conexion_memoria, 0, 0);

    sem_conexion_kernel_interrupt = malloc(sizeof(sem_t));
    if (sem_conexion_kernel_interrupt == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_conexion_kernel_interrupt, 0, 0);

    sem_conexion_kernel_cpu_dispatch = malloc(sizeof(sem_t));
    if (sem_conexion_kernel_cpu_dispatch == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_conexion_kernel_cpu_dispatch, 0, 0);

    sem_se_detuvo_ejecucion = malloc(sizeof(sem_t));
    if (sem_se_detuvo_ejecucion == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_se_detuvo_ejecucion, 0, 0);

    sem_lista_interrupciones = malloc(sizeof(sem_t));
    if (sem_lista_interrupciones == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
    sem_init(sem_lista_interrupciones, 0, 0);

    log_info(logger,"Mutex y semáforo de estado para la lista de interrupciones creados\n");

    mutex_kernel_interrupt = malloc(sizeof(pthread_mutex_t));
    if (mutex_kernel_interrupt == NULL) {
        perror("Error al asignar memoria para la conexion kernel-interrupt");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_kernel_interrupt, NULL);

    mutex_kernel_dispatch = malloc(sizeof(pthread_mutex_t));
    if (mutex_kernel_dispatch == NULL) {
        perror("Error al asignar memoria para la conexion kernel-dispatch");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_kernel_dispatch, NULL);

    mutex_conexion_memoria = malloc(sizeof(pthread_mutex_t));
    if (mutex_conexion_memoria == NULL) {
        perror("Error al asignar memoria para la conexion con memoria");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_conexion_memoria, NULL);

    mutex_fetch = malloc(sizeof(pthread_mutex_t));
    if (mutex_fetch == NULL) {
        perror("Error al asignar memoria para la conexion con memoria");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_fetch, NULL);
}

void obtener_contexto_de_memoria() {
    char *texto[1];
    
    t_list *paquete_respuesta;
    pthread_mutex_lock(mutex_conexion_memoria);

    t_paquete *paquete = crear_paquete(CONTEXTO_RECEIVE); 
    agregar_a_paquete(paquete, &pid_actual, sizeof(pid_actual));
    agregar_a_paquete(paquete, &tid_actual, sizeof(tid_actual));

    log_info(logger, "Se le solicita el contexto a memoria del PID: %d TID: %d con el cod op %d", pid_actual, tid_actual, CONTEXTO_RECEIVE);
    enviar_paquete(paquete, socket_conexion_memoria);
    eliminar_paquete(paquete);

    int cod_op = recibir_operacion(socket_conexion_memoria);

    if(cod_op == CONTEXTO_RECEIVE){
        paquete_respuesta = recibir_paquete(socket_conexion_memoria);

        cpu_actual = list_remove(paquete_respuesta, 0);
        base_actual = *(int *)list_remove(paquete_respuesta, 0);
        limite_actual = *(int *)list_remove(paquete_respuesta, 0);

        sem_post(sem_hay_contexto);

        log_info(logger, "Se recibio contexto de memoria para el TID %d", tid_actual);
        list_destroy(paquete_respuesta);
    }
    pthread_mutex_unlock(mutex_conexion_memoria);
    //log_info(logger, "## PID: %d - Se solicitó el Contexto Ejecución", pid); 
}

void fetch() {

        t_list * paquete_lista = NULL;
        t_paquete * paquete;
        protocolo_socket op;

        sem_wait(sem_hay_contexto);
        log_info(logger,"## TID: %d PC: %d- FETCH ",tid_actual, cpu_actual->PC);
        pthread_mutex_lock(mutex_conexion_memoria);
        paquete = crear_paquete(OBTENER_INSTRUCCION); 
        agregar_a_paquete(paquete, &cpu_actual->PC , sizeof(uint32_t));
        agregar_a_paquete(paquete, &tid_actual, sizeof(tid_actual)); 
        agregar_a_paquete(paquete, &pid_actual, sizeof(pid_actual)); 

        enviar_paquete(paquete, socket_conexion_memoria);
        log_error(logger, "Se solicitó la siguiente instruccion (PC:%d) a memoria", cpu_actual->PC);
        eliminar_paquete(paquete); // elimina el paquete después de enviarlo

        op = recibir_operacion(socket_conexion_memoria);
        paquete_lista = recibir_paquete(socket_conexion_memoria);
        
        if (paquete_lista == NULL || list_is_empty(paquete_lista)) {
            log_error(logger, "No se recibió ningún paquete o la lista está vacía");
            return;
        }

        char *instruccion;
        instruccion = list_remove(paquete_lista,0);
        list_destroy(paquete_lista);
        sem_post(sem_hay_contexto);
        
        if (instruccion == NULL) {
            log_error(logger, "No se pudo asignar memoria para la instrucción");
            return;
        }
              
        // Llamar a la función decode para procesar la instrucción
        pthread_mutex_unlock(mutex_conexion_memoria);
        log_error(logger, "Se recibió la siguiente instruccion (PC:%d) a memoria", cpu_actual->PC);
        decode(instruccion);
        
}

// Función para traducir direcciones lógicas a físicas
void traducir_direccion( uint32_t dir_logica, uint32_t *dir_fisica) {
    *dir_fisica = base_actual + dir_logica; 
    if (*dir_fisica >= base_actual + limite_actual) { // Validación de segmento
        log_info(logger,"Error: Segmentation Fault (Acceso fuera de límites)\n");
        char* texto[1];
        encolar_interrupcion(SEGMENTATION_FAULT,1,texto); // Sale por error
    }
}

void decode( char *inst) {
    char **texto = string_split(inst," "); //Se agrega al vector en la última posicion un NULL
    free(inst);
    //PROCESS_CREATE(0) PROCESO1(1) 256(2) 1(3), la 5ta es pos NULL. ejemplo: string_split("hola, mundo", ",") => ["hola", " mundo", NULL]
    //Compara el ID de la instrucción y ejecuta la operación correspondiente

    if (strcmp(texto[0], "SET") == 0 && texto[1] && texto[2]) 
        // Instrucción SET: Asigna un valor a un registro
            execute(1,texto); // 1 = SET
    
    else if (strcmp(texto[0], "SUM") == 0 && texto[1] && texto[2]) 
        // Instrucción SUM: Suma el valor de un registro a otro   
            execute(2,texto); // 2 = SUM
    
    else if (strcmp(texto[0], "SUB") == 0 && texto[1] && texto[2]) 
        // Instrucción SUB: Resta el valor de un registro a otro
            execute(3,texto);//3 = SUB

    else if (strcmp(texto[0], "READ_MEM") == 0 && texto[1] && texto[2]) 
        // Instrucción READ_MEM: Lee de memoria a un registro
            execute(4,texto); // 4 = READ_MEM

    else if (strcmp(texto[0], "WRITE_MEM") == 0 && texto[1] && texto[2]) 
        // Instrucción WRITE_MEM: Escribe en memoria desde un registro
            execute(5,texto); // 5 = WRITE_MEM

    else if (strcmp(texto[0], "JNZ") == 0 && texto[1] && texto[2]) 
        // Instrucción JNZ: Salta si el valor de un registro no es cero 

            execute(6,texto); // 6 = JNZ
    else if (strcmp(texto[0], "MUTEX_CREATE") == 0 && texto[1])
            execute(7,texto); // 7 = MUTEX_CREATE}
            

    else if (strcmp(texto[0], "MUTEX_LOCK") == 0 && texto[1]) 
            execute(8,texto); // 8 = MUTEX_LOCK

    else if (strcmp(texto[0], "DUMP_MEMORY") == 0) 
            execute(9,texto); // 9 = DUMP_MEMORY

    else if (strcmp(texto[0], "PROCESS_CREATE") == 0 && texto[1] && texto[2] && texto[3]) 
            execute(10,texto);// 10 = PROCESS_CREATE

    else if (strcmp(texto[0], "THREAD_CREATE") == 0 && texto[1] && texto[2]) 
            execute(11,texto);// 11 = THREAD_CREATE

    else if (strcmp(texto[0], "THREAD_CANCEL") == 0 && texto[1]) 
            execute(12,texto);// 12 = THREAD_CANCEL

    else if (strcmp(texto[0], "THREAD_JOIN") == 0 && texto[1]) 
            execute(13,texto);// 13 = THREAD_JOIN

    else if (strcmp(texto[0], "THREAD_EXIT") == 0) 
            execute(14,texto);// 14 = THREAD_EXIT

    else if (strcmp(texto[0], "PROCESS_EXIT") == 0) 
            execute(15,texto);// 15 = PROCESS_EXIT

    else if (strcmp(texto[0], "MUTEX_LOCK") == 0 && texto[1]) 
            execute(16,texto); // LOG
    else if ((strcmp(texto[0], "LOG") == 0 && texto[1]))
            execute(17, texto);
    else if ((strcmp(texto[0], "IO") == 0 && texto[1]))
            execute(18, texto);
    else{
        log_info(logger, "Instrucción no reconocida: %s", texto[0]);
        exit(EXIT_FAILURE);
    }
}

// Función que ejecuta una instrucción
void execute(int instruccion, char **texto) {
    uint32_t *reg_destino = NULL;
    uint32_t *reg_origen = NULL;
    uint32_t *reg_direccion = NULL;
    uint32_t *reg_valor = NULL;
    uint32_t *reg_comparacion = NULL;
    t_paquete * paquete;
    char * ok_respuesta;
    t_list *paquete_recv;

    switch (instruccion)
    {
        case 1: // SET
            reg_destino = registro_aux(texto[1]);
            if (reg_destino != NULL && texto[2]) 
            {
                *reg_destino = (uint32_t) atoi(texto[2]); // Convierte el valor a entero y lo asigna al registro
                log_info(logger, "## Ejecutando: SET - Reg: %s, Valor: %u", texto[1], *reg_destino);
            } else 
                log_error(logger, "Error en SET: Registro no válido o valor no especificado");
        break;
        case 2: // SUM
            reg_destino = registro_aux( texto[1]);    
            reg_origen = registro_aux(texto[2]);
    
            if (reg_destino != NULL && reg_origen != NULL) 
            {
                *reg_destino += *reg_origen; // Suma el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUM - Reg: %s, Nuevo Valor: %u", texto[1], *reg_destino);
            } else
                log_info(logger, "Error: Registro no válido en SUM");
        break;
        case 3: // SUB
            reg_destino = registro_aux(texto[1]);    
            reg_origen = registro_aux( texto[2]);

            if (reg_destino != NULL && reg_origen != NULL)
            {
                *reg_destino -= *reg_origen; // Resta el valor del registro origen al destino
                log_info(logger, "## Ejecutando: SUB - Reg: %s, Nuevo Valor: %u", texto[1], *reg_destino);
            }
            else 
                log_info(logger, "Error: Registro no válido en SUB");
        break;
        case 4: // READ MEM
            reg_destino = registro_aux(texto[1]); // Registro donde se guardará el valor leído
            reg_direccion = registro_aux(texto[2]); // Registro que contiene la dirección lógica
            if (reg_destino != NULL && reg_direccion != NULL) 
            {
                uint32_t direccion_fisica = 0;
                traducir_direccion( *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                log_info(logger, "## LEER MEMORIA - Dirección Física: %u", direccion_fisica);

                // iría la lógica de leer de la memoria real y asignar al registro destino
                paquete = crear_paquete(READ_MEM);
                agregar_a_paquete(paquete,reg_direccion,sizeof(uint32_t));
                agregar_a_paquete(paquete,&pid_actual,sizeof(int));
                agregar_a_paquete(paquete,&pid_actual,sizeof(int));
                pthread_mutex_lock(mutex_conexion_memoria);
                enviar_paquete(paquete,socket_conexion_memoria);
                eliminar_paquete(paquete);
                int cod_op = recibir_operacion(socket_conexion_memoria);
                if(cod_op){
                    paquete_recv = recibir_paquete(socket_conexion_memoria);
                }
                pthread_mutex_unlock(mutex_conexion_memoria);

                *reg_destino = *(uint32_t *)list_remove(paquete_recv,0);
                log_info(logger, "Valor leido: %d", *reg_destino);
                list_destroy(paquete_recv);
            }else{ 
                log_info(logger, "Error: Registro no válido en READ_MEM");
                manejar_motivo(SEGMENTATION_FAULT,texto);
            }
        break;
        case 5: // WRITE MEM
            reg_direccion = registro_aux(texto[1]); // Dirección lógica
            reg_valor = registro_aux( texto[2]); // Registro con el valor a escribir
            if (reg_direccion != NULL && reg_valor != NULL) {
                    uint32_t direccion_fisica = 0;
                    traducir_direccion( *reg_direccion, &direccion_fisica); // Traduce la dirección lógica a física
                    paquete = crear_paquete(WRITE_MEM);
                    agregar_a_paquete(paquete,reg_direccion,sizeof(uint32_t));
                    agregar_a_paquete(paquete,reg_valor,sizeof(uint32_t));
                    agregar_a_paquete(paquete,&pid_actual,sizeof(int));
                    agregar_a_paquete(paquete,&pid_actual,sizeof(int));
                    pthread_mutex_lock(mutex_conexion_memoria);
                    enviar_paquete(paquete,socket_conexion_memoria);
                    eliminar_paquete(paquete);
                    log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);
                    protocolo_socket cod_op;
                    cod_op = recibir_operacion(socket_conexion_memoria);
                    pthread_mutex_unlock(mutex_conexion_memoria);
                    switch (cod_op)
                    {
                        case OK:
                            log_info(logger, "## Operación recibida:OK!");
                            paquete_recv = recibir_paquete(socket_conexion_memoria);
                            ok_respuesta = list_remove(paquete_recv, 0);
                            free(ok_respuesta);
                            list_destroy(paquete_recv);
                        break;
                        case SEGMENTATION_FAULT:
                            paquete_recv = recibir_paquete(socket_conexion_memoria);
                            ok_respuesta = list_remove(paquete_recv, 0);
                            free(ok_respuesta);
                            list_destroy(paquete_recv);
                            manejar_motivo(SEGMENTATION_FAULT,texto);
                        break;
                        default:
                            log_info(logger,"Error: Registro no válido en WRITE_MEM");
                        break;
                    }
                }
        break;
        case 6: // JNZ
            reg_comparacion = registro_aux( texto[1]); // Registro a comparar con 0
            if (reg_comparacion != NULL && *reg_comparacion != 0) {
                cpu_actual->PC = (uint32_t) atoi(texto[2])-1; // Actualiza el PC, que se supone que es entero el valor del PC
                log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu_actual->PC+1);
            } else if (reg_comparacion == NULL) 
                log_info(logger, "Error: Registro no válido en JNZ");
            else 
                log_info(logger, "## JNZ - No se realiza salto porque el valor es 0");
        break;

        //Syscalls
        case 7: // MUTEX_CREATE
            encolar_interrupcion(MUTEX_CREATE_OP,2,texto);
        break;
        case 8: // MUTEX_LOCK
            encolar_interrupcion(MUTEX_LOCK_OP,2,texto);
        break;
        case 9: // DUMP_MEMORY
            encolar_interrupcion(DUMP_MEMORY_OP,2,texto);
        break;
        case 10: // PROCESS_CREATE
            encolar_interrupcion(PROCESS_CREATE_OP,2,texto);
        break;
        case 11: // THREAD_CREATE
            encolar_interrupcion(THREAD_CREATE_OP,2,texto);     
        break;
        case 12: // THREAD_CANCEL
            encolar_interrupcion(THREAD_CANCEL_OP,2,texto);
        break;
        case 13: // THREAD_JOIN
            encolar_interrupcion(THREAD_JOIN_OP,2,texto);
        break;
        case 14: // THREAD_EXIT
            encolar_interrupcion(THREAD_EXIT_OP,2,texto);
        break;
        case 15: // PROCESS_EXIT
            encolar_interrupcion(PROCESS_EXIT_OP,2,texto);
        break;
        case 16: // MUTEX_UNLOCK
            encolar_interrupcion(MUTEX_UNLOCK_OP,2,texto);
        break;
        case 17: // LOG_OP
            uint32_t * registro_a_leer = registro_aux(texto[1]);
            log_info(logger, "Valor del registro %s: %d", texto[1], *registro_a_leer);
        break;
        case 18: // IO_SYSCALL
            encolar_interrupcion(IO_SYSCALL,2,texto);
        break;
        default:
            log_info(logger, "Error: Instrucción no reconocida");
        break;
    }
    cpu_actual->PC++; // Incrementar el contador de programa
    checkInterrupt();
  
}

uint32_t* registro_aux( char* reg) {
    if (strcmp(reg, "AX") == 0)
        return &(cpu_actual->AX);
    else if (strcmp(reg, "BX") == 0)
        return &(cpu_actual->BX);
    else if (strcmp(reg, "CX") == 0)
        return &(cpu_actual->CX);
    else if (strcmp(reg, "DX") == 0)
        return &(cpu_actual->DX);
    else if (strcmp(reg, "EX") == 0)
        return &(cpu_actual->EX);
    else if (strcmp(reg, "FX") == 0)
        return &(cpu_actual->FX);
    else if (strcmp(reg, "GX") == 0)
        return &(cpu_actual->GX);
    else if (strcmp(reg, "HX") == 0)
        return &(cpu_actual->HX);
    return NULL; // En caso de que el registro no sea válido
    
}

void checkInterrupt() { //el checkInterrupt se corre siempre -> interrupcion -> tipo 
    t_interrupcion* interrupcion_actual = obtener_interrupcion();

    if (interrupcion_actual == NULL)
    {
        log_info(logger, "No hay interrupciones para tratar");
        return;
    }

    log_info(logger, "Se recibio una interrupcion");
    switch (interrupcion_actual->tipo) {
        case FIN_QUANTUM:
            log_info(logger, "## Interrupción FIN_QUANTUM recibida para el TID: %d", tid_de_interrupcion_FIN_QUANTUM);
            if(pid_actual != 0)
                {
                    manejar_motivo(interrupcion_actual->tipo,interrupcion_actual->parametro);
                    break;
                }
            log_info(logger, "## Interrupción FIN_QUANTUM cancelada, PID: %d TID:%d", pid_actual, tid_actual);
            break;
        case PROCESS_CREATE_OP:
            log_info(logger, "## syscall Interrupción PROCESS_CREATE_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case PROCESS_EXIT_OP:
            log_info(logger, "## syscall Interrupción PROCESS_EXIT_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case THREAD_CREATE_OP:
            log_info(logger, "## syscall THREAD_CREATE_OP recibida");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case THREAD_CANCEL_OP:
            log_info(logger, "## syscall THREAD_CANCEL_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case THREAD_EXIT_OP:
            log_info(logger, "## syscall THREAD_EXIT_OP recibida ");
            manejar_finalizacion(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case THREAD_JOIN_OP:
            log_info(logger, "## Interrupción THREAD_JOIN_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo,interrupcion_actual->parametro);  
            break; 
        case MUTEX_CREATE_OP:
            log_info(logger, "## syscall Interrupción MUTEX_CREATE_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case MUTEX_LOCK_OP:
            log_info(logger, "## syscall Interrupción MUTEX_LOCK_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case MUTEX_UNLOCK_OP:
            log_info(logger, "## syscall Interrupción MUTEX_UNLOCK_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case DUMP_MEMORY_OP:
            log_info(logger, "##syscall  Interrupción DUMP_MEMORY_OP recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        case SEGMENTATION_FAULT:
            log_info(logger, "## Interrupción SEGMENTATION_FAULT recibida ");
            manejar_motivo(interrupcion_actual->tipo,interrupcion_actual->parametro);
            break;    
        case INFO_HILO:
            log_info(logger, "Se recibio la info de un nuevo hilo");
            log_info(logger, "EL PID ACTUAL A PARTIR DEL HILO RECIBIDO ES IGUAL A: %d", pid_actual);
            obtener_contexto_de_memoria();
            break;
        case IO_SYSCALL:
            log_info(logger, "##syscall  Interrupción IO_SYSCALL recibida ");
            manejar_motivo(interrupcion_actual->tipo, interrupcion_actual->parametro);
            break;
        default:
            log_error(logger, "Instruccion invalida %d", interrupcion_actual->tipo);
            manejar_motivo(ERROR, interrupcion_actual->parametro); // se envia un "ERROR"
            break;
    }
    // Verificar si la CPU generó alguna interrupción (ej: Segmentation Fault)
    free(interrupcion_actual);
}

void enviar_contexto_de_memoria() {
    // Crear el paquete a enviar
    char *texto[1];
    t_paquete *paquete_send = crear_paquete(CONTEXTO_SEND);
    t_list * paquete_respuesta;

    // Agregar los registros de la CPU al paquete
    agregar_a_paquete(paquete_send, &pid_actual, sizeof(int));
    agregar_a_paquete(paquete_send, &tid_actual, sizeof(int));
    agregar_a_paquete(paquete_send, cpu_actual, sizeof(RegistroCPU));

    // Enviar el paquete a la memoria
    enviar_paquete(paquete_send, socket_conexion_memoria);
    eliminar_paquete(paquete_send); // Liberar el paquete enviado

    // Recibir la respuesta de memoria (paquete del tipo t_list)
    protocolo_socket respuesta = recibir_operacion(socket_conexion_memoria);

    // Validar que el paquete contenga datos y extraerlos
    switch (respuesta){
        case OK:
            log_info(logger, "Contexto de PID %d enviado correctamente a Memoria", pid_actual);
            paquete_respuesta = recibir_paquete(socket_conexion_memoria);
            list_destroy(paquete_respuesta);
            break;
        case SEGMENTATION_FAULT:
            log_error(logger, "Segmentation Fault recibido desde Memoria.");
            pthread_mutex_lock(mutex_lista_interrupciones);
            encolar_interrupcion(SEGMENTATION_FAULT, 1,texto);
            pthread_mutex_unlock(mutex_lista_interrupciones);
            paquete_respuesta = recibir_paquete(socket_conexion_memoria);
            list_destroy(paquete_respuesta);
            break;      
        case ERROR_MEMORIA:
            log_error(logger, "Error crítico: Memoria respondió con un error.");
            pthread_mutex_lock(mutex_lista_interrupciones);
            encolar_interrupcion(SEGMENTATION_FAULT, 1,texto);
            pthread_mutex_unlock(mutex_lista_interrupciones);
            paquete_respuesta = recibir_paquete(socket_conexion_memoria);
            list_destroy(paquete_respuesta);
            break;
        default:  // Caso de respuesta inesperada
            log_warning(logger, "Respuesta inesperada de Memoria!");
            paquete_respuesta = recibir_paquete(socket_conexion_memoria);
            list_destroy(paquete_respuesta);
            break;
    }
}

void devolver_motivo_a_kernel(protocolo_socket cod_op, char** texto) {
    t_paquete *paquete_notify = crear_paquete(cod_op);
    log_info(logger, "Entra a la primera parte de devolver motivo a kernel");
    // Enviamos el PID y TID para notificar al Kernel que el proceso fue interrumpido
    char * ok_send = "OK";
    int texto1;
    int texto2;
    int texto3;
    switch (cod_op)
    {
        //no mandan nada
        case DUMP_MEMORY_OP:
        case PROCESS_EXIT_OP:
        case THREAD_EXIT_OP:
        case FIN_QUANTUM:
        case SEGMENTATION_FAULT:
            agregar_a_paquete(paquete_notify, ok_send, strlen(ok_send)+1);
            break;

        //solo mandan el primer elemento
        case MUTEX_CREATE_OP:
        case LOG_OP:
        case MUTEX_LOCK_OP: 
        case MUTEX_UNLOCK_OP:
            agregar_a_paquete(paquete_notify,texto[1], strlen(texto[1]) + 1);
            break;

        case THREAD_JOIN_OP: 
        case THREAD_CANCEL_OP:
        case IO_SYSCALL:
            //agregar_a_paquete(paquete_notify,texto[1], strlen(texto[1]) + 1); // tid texto[1]
            texto1 = atoi(texto[1]);
            agregar_a_paquete(paquete_notify, &texto1, sizeof(int)); // tid texto[1]
            break;

        //mandan 2 elementos
        case THREAD_CREATE_OP:
            agregar_a_paquete(paquete_notify, texto[1], strlen(texto[1]) + 1);// nombre archivo texto[1]
            texto2 = atoi(texto[2]);
            agregar_a_paquete(paquete_notify, &texto2, sizeof(int));// prioridad texto[2]
            log_info(logger, "Agrega correctamente al paquete a enviar a kernel estos textos %s, %s", texto[1], texto[2]);
            break;
        
        case PROCESS_CREATE_OP:
            agregar_a_paquete(paquete_notify,texto[1], strlen(texto[1]) + 1);// nombre archivo texto[1]
            texto2 = atoi(texto[2]);
            texto3 = atoi(texto[3]);
            agregar_a_paquete(paquete_notify, &texto2, sizeof(int));// prioridad texto[3]
            agregar_a_paquete(paquete_notify, &texto3, sizeof(int));// tamanio texto[2]
            log_info(logger, "Agrega correctamente al paquete a enviar a kernel estos textos %s, %s, %s", texto[1], texto[2], texto[3]);
            break;
        
        default: 
            log_error(logger, "Interrupcion invalida %d", cod_op);
            break;
    }
    log_info(logger, "Notificación enviada al Kernel por la interrupción del PID %d, TID %d", pid_actual, tid_actual);
    enviar_paquete(paquete_notify, socket_conexion_kernel_interrupt);
    eliminar_paquete(paquete_notify);  // Limpiar el paquete de respuesta
}

void encolar_interrupcion(protocolo_socket tipo, int prioridad, char** texto) { 
    // Las interrupciones se pueden generar tanto por la CPU como por el Kernel. 
    // Las añadimos a la lista de interrupciones con su respectiva prioridad.
    t_interrupcion* nueva_interrupcion = malloc(sizeof(t_interrupcion));
    nueva_interrupcion->tipo = tipo;
    nueva_interrupcion->prioridad = prioridad;
    nueva_interrupcion->parametro = texto;
    
    // Insertamos la interrupción en la lista
    list_add(lista_interrupciones, nueva_interrupcion);
    log_info(logger, "Interrupción agregada: %i con prioridad %d", tipo, prioridad);
    sem_post(sem_lista_interrupciones);
}

t_interrupcion* obtener_interrupcion() {
    if (list_is_empty(lista_interrupciones))
    {
        return NULL;
    }

    t_interrupcion* interrupcion_mayor_prioridad = list_get(lista_interrupciones, 0);  // Suponemos que hay al menos una interrupción
    int indice_mayor_prioridad = 0;

    
    //Recorrer la lista para encontrar la interrupción con mayor prioridad (menor valor numérico)
    for (int i = 0; i < list_size(lista_interrupciones); i++) {
        t_interrupcion* interrupcion_actual = list_get(lista_interrupciones, i);
        if (interrupcion_actual->prioridad < interrupcion_mayor_prioridad->prioridad) { 
    // Si encontramos una interrupción con menor prioridad numérica (prioridad más alta), actualizamos nuestro candidato.
            interrupcion_mayor_prioridad = interrupcion_actual;
            indice_mayor_prioridad = i;
        }
        log_info(logger, "La interrupcion actual es: %d", interrupcion_actual->tipo);
    }
    
    // Una vez que encontramos la interrupción con mayor prioridad, la eliminamos de la lista
    pthread_mutex_lock(mutex_lista_interrupciones);

    list_remove(lista_interrupciones, indice_mayor_prioridad);
    pthread_mutex_unlock(mutex_lista_interrupciones);

    return interrupcion_mayor_prioridad;
}

void liberar_interrupcion(t_interrupcion* interrupcion) {
    if (interrupcion != NULL) {
        free(interrupcion);  // Liberamos la estructura   
        return;
    }
    return;
}

void manejar_motivo(protocolo_socket tipo, char** texto) {

    log_info(logger, "Se detiene la ejecucion del hilo actual para manejar el motivo: %d", tipo);

    pthread_mutex_lock(mutex_conexion_memoria);
    enviar_contexto_de_memoria();
    pthread_mutex_unlock(mutex_conexion_memoria);
    
    pthread_mutex_lock(mutex_kernel_interrupt);
    devolver_motivo_a_kernel(tipo,texto);
    pthread_mutex_unlock(mutex_kernel_interrupt);
    detener_ejecucion();
}

void manejar_finalizacion(protocolo_socket tipo, char** texto){
    log_info(logger, "## Manejo de Finalización");

    // Notificar al Kernel que el hilo finalizó

    pthread_mutex_lock(mutex_kernel_interrupt);
    devolver_motivo_a_kernel(tipo,texto);
    pthread_mutex_unlock(mutex_kernel_interrupt);
    
    detener_ejecucion();
}

void detener_ejecucion(){
    inicializar_registros_cpu();
}