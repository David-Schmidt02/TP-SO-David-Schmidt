#include <main.h>

t_log *logger;

int socket_conexion_memoria;
int socket_conexion_kernel_interrupt;
int socket_conexion_kernel_dispatch;


sem_t * sem_estado_hilo_ejecutar;
//Variables globales
uint32_t base_actual;
uint32_t limite_actual;
RegistroCPU *cpu_actual;
int pid_actual;
int tid_actual;
char * instruccion_actual;
bool flag_hay_interrupcion;
t_interrupcion* interrupcion_actual;
t_instruccion_partida * instruccion_actual_partida;
//

int tid_de_interrupcion_FIN_QUANTUM;

//Lista de interrupciones
t_list* lista_interrupciones;
sem_t * sem_lista_interrupciones;
pthread_mutex_t *mutex_lista_interrupciones;
//

sem_t * sem_registros_actualizados;
//No se usa
sem_t * sem_hay_contexto;
pthread_mutex_t * mutex_fetch;
//

//Mutexs para proteger las conexiones
pthread_mutex_t *mutex_kernel_interrupt;
pthread_mutex_t *mutex_kernel_dispatch;
pthread_mutex_t *mutex_conexion_memoria;
//

//Semáforos para esperar a que las conexiones se hagan
sem_t * sem_conexion_kernel_interrupt;
sem_t * sem_conexion_kernel_cpu_dispatch;
sem_t * sem_conexion_memoria;
//

int main(int argc, char* argv[]) {

    pthread_t tid_kernelI;
	pthread_t tid_kernelD;
    pthread_t tid_memoria;

	argumentos_thread arg_kernelD;
	argumentos_thread arg_kernelI;
    argumentos_thread arg_memoria;

    logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/cpu.config");

    void *ret_value = NULL;

    //conexiones
	arg_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	arg_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
    arg_kernelD.puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	arg_kernelI.puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    
	//conexiones
	inicializar_estructuras_cpu();

	sem_estado_hilo_ejecutar = malloc(sizeof(sem_t));
    sem_init(sem_estado_hilo_ejecutar, 0, 0);


    //conexiones
	pthread_create(&tid_kernelI, NULL, conexion_kernel_interrupt, (void *)&arg_kernelI);
	pthread_create(&tid_kernelD, NULL, conexion_kernel_dispatch, (void *)&arg_kernelD);
	pthread_create(&tid_memoria, NULL, cliente_conexion_memoria, (void *)&arg_memoria);

	sem_wait(sem_conexion_kernel_cpu_dispatch);
	sem_wait(sem_conexion_kernel_interrupt);
	sem_wait(sem_conexion_memoria);

	//ciclo de instruccion
	log_info(logger,"Creacion del hilo para el ciclo de instrucción\n");
    pthread_t hilo_ciclo_instruccion;
    pthread_create(&hilo_ciclo_instruccion, NULL, (void *)ciclo_instruccion, NULL);

	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_kernelI, ret_value);
	pthread_join(tid_kernelD, ret_value);
    pthread_join(hilo_ciclo_instruccion, ret_value);

	//espero fin conexiones
}
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
    tid_actual = 0; 

    list_clean(lista_interrupciones);

    log_info(logger, "Contexto de CPU inicializado");
    sem_post(sem_registros_actualizados);
}

void inicializar_semaforos_cpu(){
	flag_hay_interrupcion = false;
    mutex_lista_interrupciones = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_lista_interrupciones, NULL);

    sem_hay_contexto = malloc(sizeof(sem_t));
    sem_init(sem_hay_contexto, 0, 1);

    sem_conexion_memoria = malloc(sizeof(sem_t));
    sem_init(sem_conexion_memoria, 0, 0);

    sem_conexion_kernel_interrupt = malloc(sizeof(sem_t));
    sem_init(sem_conexion_kernel_interrupt, 0, 0);

    sem_conexion_kernel_cpu_dispatch = malloc(sizeof(sem_t));
    sem_init(sem_conexion_kernel_cpu_dispatch, 0, 0);

    sem_registros_actualizados = malloc(sizeof(sem_t));
    sem_init(sem_registros_actualizados, 0, 0);
    //sem_se_detuvo_ejecucion = malloc(sizeof(sem_t));
    //sem_init(sem_se_detuvo_ejecucion, 0, 0);

    sem_lista_interrupciones = malloc(sizeof(sem_t));
    sem_init(sem_lista_interrupciones, 0, 0);

    log_info(logger,"Mutex y semáforo de estado para la lista de interrupciones creados\n");

    mutex_kernel_interrupt = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_kernel_interrupt, NULL);

    mutex_kernel_dispatch = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_kernel_dispatch, NULL);

    mutex_conexion_memoria = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_conexion_memoria, NULL);

    mutex_fetch = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_fetch, NULL);
}

void *ciclo_instruccion(void* args)
{
	int i =0;
	while ((1))
	{
		sem_wait(sem_estado_hilo_ejecutar);
		obtener_contexto_de_memoria();
		while (!flag_hay_interrupcion)
		{
			log_warning(logger, "Nro de vueltas: %d",i);
			fetch();
			instruccion_actual_partida = decode(instruccion_actual);
			execute(instruccion_actual_partida);
            if (instruccion_actual_partida->operacion != JNZ)
			    cpu_actual->PC++;
		}
		checkInterrupt();
		enviar_contexto_de_memoria();
		devolver_motivo_a_kernel(interrupcion_actual->tipo, interrupcion_actual->parametro);
		inicializar_registros_cpu();
	}
}

void *conexion_kernel_dispatch(void* arg_kernelD)
{
	argumentos_thread * args = arg_kernelD; 
	t_list *paquete;
	int tid;

	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor de Dispatch listo para recibir al cliente Kernel");
	
	pthread_mutex_lock(mutex_kernel_dispatch);
	socket_conexion_kernel_dispatch = esperar_cliente(server);
	pthread_mutex_unlock(mutex_kernel_dispatch);

	log_info(logger, "Se realizó la conexion Kernel - Cpu Dispatch");
	sem_post(sem_conexion_kernel_cpu_dispatch);	
	while(true){
		pthread_mutex_lock(mutex_kernel_dispatch);
		int cod_op = recibir_operacion(socket_conexion_kernel_dispatch);
		switch (cod_op){
			case INFO_HILO:
                sem_wait(sem_registros_actualizados);
				log_info(logger, "Recibí un hilo para ejecutar de parte de Kernel");
				t_list *paquete = recibir_paquete(socket_conexion_kernel_dispatch);
				tid_actual = *(int *)list_remove(paquete, 0);
				pid_actual = *(int *)list_remove(paquete, 0);
				log_error(logger, "El hilo a ejecutar es PID: %d TID:%d", pid_actual, tid_actual);
				list_destroy(paquete);
				flag_hay_interrupcion = false;
				sem_post(sem_estado_hilo_ejecutar);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				pthread_mutex_unlock(mutex_kernel_dispatch);
				return (void *)EXIT_FAILURE;
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		pthread_mutex_unlock(mutex_kernel_dispatch);
		}
		
	close(socket_conexion_kernel_dispatch);
    return (void *)EXIT_SUCCESS;
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

        log_info(logger, "Se recibio contexto de memoria para el TID %d", tid_actual);
        list_destroy(paquete_respuesta);
    }
	else{
		log_error(logger, "Se recibio contexto de memoria para el TID %d", tid_actual);
	}
    pthread_mutex_unlock(mutex_conexion_memoria);
}

void fetch() {

        t_list * paquete_lista = NULL;
        t_paquete * paquete;
        protocolo_socket op;

        log_info(logger,"## PID: %d TID: %d PC: %d- FETCH ",pid_actual, tid_actual, cpu_actual->PC);
        pthread_mutex_lock(mutex_conexion_memoria);
        paquete = crear_paquete(OBTENER_INSTRUCCION); 

        agregar_a_paquete(paquete, &cpu_actual->PC, sizeof(uint32_t));
        agregar_a_paquete(paquete, &tid_actual, sizeof(tid_actual)); 
        agregar_a_paquete(paquete, &pid_actual, sizeof(pid_actual)); 

        enviar_paquete(paquete, socket_conexion_memoria);
        log_error(logger, "Se solicitó la siguiente instruccion (PC:%d) a memoria", cpu_actual->PC);
        eliminar_paquete(paquete);

        op = recibir_operacion(socket_conexion_memoria);
        paquete_lista = recibir_paquete(socket_conexion_memoria);
        
        if (paquete_lista == NULL || list_is_empty(paquete_lista)) {
            log_error(logger, "No se recibió ningún paquete o la lista está vacía");
            return;
        }

        instruccion_actual = list_remove(paquete_lista,0);

        list_destroy(paquete_lista);
        log_error(logger, "La instruccion obtenida es: %s", instruccion_actual);
              
        // Llamar a la función decode para procesar la instrucción
        pthread_mutex_unlock(mutex_conexion_memoria);
        log_error(logger, "Se recibió la siguiente instruccion (PC:%d) a memoria", cpu_actual->PC);
}

t_instruccion_partida * decode(char *inst) {
    // Divide la instrucción en palabras
    char **texto = string_split(inst, " "); // Agrega un NULL al final del array
    free(inst);

    t_instruccion_partida *instruccion_actual_partida = malloc(sizeof(t_instruccion_partida));
    instruccion_actual_partida->texto_partido = texto;

    // Mapeo de instrucciones
    if (strcmp(texto[0], "SET") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = SET;
    } 
    else if (strcmp(texto[0], "SUM") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = SUM;
    }
    else if (strcmp(texto[0], "SUB") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = SUB;
    }
    else if (strcmp(texto[0], "READ_MEM") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = READ_MEM;
    }
    else if (strcmp(texto[0], "WRITE_MEM") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = WRITE_MEM; 
    }
    else if (strcmp(texto[0], "JNZ") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = JNZ;
    }
		//SYSCALLS
    else if (strcmp(texto[0], "MUTEX_CREATE") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = MUTEX_CREATE_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "MUTEX_LOCK") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = MUTEX_LOCK_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "MUTEX_UNLOCK") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = MUTEX_UNLOCK_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "DUMP_MEMORY") == 0) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = DUMP_MEMORY_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "PROCESS_CREATE") == 0 && texto[1] && texto[2] && texto[3]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = PROCESS_CREATE_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "THREAD_CREATE") == 0 && texto[1] && texto[2]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = THREAD_CREATE_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "THREAD_CANCEL") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = THREAD_CANCEL_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "THREAD_JOIN") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = THREAD_JOIN_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "THREAD_EXIT") == 0) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = THREAD_EXIT_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "PROCESS_EXIT") == 0) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = PROCESS_EXIT_OP;
		instruccion_actual_partida->prioridad = 2;
    }
    else if (strcmp(texto[0], "LOG") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = INSTRUCCION_NORMAL;
        instruccion_actual_partida->operacion = LOG_OP;
    }
    else if (strcmp(texto[0], "IO") == 0 && texto[1]) {
        instruccion_actual_partida->tipo = SYSCALL;
        instruccion_actual_partida->syscall = IO_SYSCALL;
		instruccion_actual_partida->prioridad = 2;
    }
    else {
        log_info(logger, "Instrucción no reconocida: %s", *texto);
        exit(EXIT_FAILURE);
    }

    return instruccion_actual_partida;
}

void execute(t_instruccion_partida *instruccion_partida) {
    uint32_t *reg_destino = NULL;
    uint32_t *reg_origen = NULL;
    uint32_t *reg_direccion = NULL;
    uint32_t *reg_valor = NULL;
    uint32_t *reg_comparacion = NULL;
    t_paquete *paquete;
    char *ok_respuesta;
    t_list *paquete_recv;

    protocolo_socket tipo = instruccion_partida->tipo;
    int i= 0;
    switch (tipo) {
        case INSTRUCCION_NORMAL: {
            protocolo_socket operacion = instruccion_partida->operacion;
            for (i; i< string_array_size(instruccion_partida->texto_partido); i++){
                log_warning(logger, "Valores de las instrucciones: %s", instruccion_partida->texto_partido[i]);
            }
            char **parametros = instruccion_partida->texto_partido;
            switch (operacion) {
                case SET: {
                    reg_destino = registro_aux(parametros[1]);
                    if (reg_destino != NULL && parametros[2]) {
                        *reg_destino = (uint32_t)atoi(parametros[2]);
                        log_info(logger, "## Ejecutando: SET - Reg: %s, Valor: %u", parametros[0], *reg_destino);
                    } else {
                        log_error(logger, "Error en SET: Registro no válido o valor no especificado");
                    }
                    break;
                }
                case SUM: {
                    reg_destino = registro_aux(parametros[1]);
                    reg_origen = registro_aux(parametros[2]);

                    if (reg_destino != NULL && reg_origen != NULL) {
                        *reg_destino += *reg_origen;
                        log_info(logger, "## Ejecutando: SUM - Reg: %s, Nuevo Valor: %u", parametros[1], *reg_destino);
                    } else {
                        log_info(logger, "Error: Registro no válido en SUM");
                    }
                    break;
                }
                case SUB: {
                    reg_destino = registro_aux(parametros[1]);
                    reg_origen = registro_aux(parametros[2]);

                    if (reg_destino != NULL && reg_origen != NULL) {
                        *reg_destino -= *reg_origen;
                        log_info(logger, "## Ejecutando: SUB - Reg: %s, Nuevo Valor: %u", parametros[1], *reg_destino);
                    } else {
                        log_info(logger, "Error: Registro no válido en SUB");
                    }
                    break;
                }
                case READ_MEM: {
                    reg_destino = registro_aux(parametros[1]);
                    reg_direccion = registro_aux(parametros[2]);

                    if (reg_destino != NULL && reg_direccion != NULL) {
                        uint32_t direccion_fisica = 0;
                        traducir_direccion(*reg_direccion, &direccion_fisica);
                        log_info(logger, "## LEER MEMORIA - Dirección Física: %u", direccion_fisica);

                        paquete = crear_paquete(READ_MEM);
                        agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
                        agregar_a_paquete(paquete, &pid_actual, sizeof(int));
                        agregar_a_paquete(paquete, &tid_actual, sizeof(int));

                        pthread_mutex_lock(mutex_conexion_memoria);
                        enviar_paquete(paquete, socket_conexion_memoria);
                        eliminar_paquete(paquete);

                        int cod_op = recibir_operacion(socket_conexion_memoria);
                        if (cod_op) {
                            paquete_recv = recibir_paquete(socket_conexion_memoria);
                        }
                        pthread_mutex_unlock(mutex_conexion_memoria);

                        *reg_destino = *(uint32_t *)list_remove(paquete_recv, 0);
                        log_info(logger, "Valor leído: %d", *reg_destino);
                        list_destroy(paquete_recv);
                    } else {
                        log_info(logger, "Error: Registro no válido en READ_MEM");
						encolar_interrupcion(SEGMENTATION_FAULT, 1 ,parametros);
                    }
                    break;
                }
                case WRITE_MEM: {
                    reg_direccion = registro_aux(parametros[1]);
                    reg_valor = registro_aux(parametros[2]);

                    if (reg_direccion != NULL && reg_valor != NULL) {
                        uint32_t direccion_fisica = 0;
                        traducir_direccion(*reg_direccion, &direccion_fisica);

                        paquete = crear_paquete(WRITE_MEM);
                        agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
                        agregar_a_paquete(paquete, reg_valor, sizeof(uint32_t));
                        agregar_a_paquete(paquete, &pid_actual, sizeof(int));
                        agregar_a_paquete(paquete, &tid_actual, sizeof(int));

                        pthread_mutex_lock(mutex_conexion_memoria);
                        enviar_paquete(paquete, socket_conexion_memoria);
                        eliminar_paquete(paquete);

                        log_info(logger, "## ESCRIBIR MEMORIA - Dirección Física: %u, Valor: %u", direccion_fisica, *reg_valor);

                        protocolo_socket cod_op = recibir_operacion(socket_conexion_memoria);
                        pthread_mutex_unlock(mutex_conexion_memoria);

                        switch (cod_op) {
                            case OK:
                                log_info(logger, "## Operación recibida: OK!");
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
								encolar_interrupcion(SEGMENTATION_FAULT, 1 ,parametros);
	                            break;
                            default:
                                log_info(logger, "Error: Registro no válido en WRITE_MEM");
                                break;
                        }
                    }
                    break;
                }
                case JNZ: {
                    reg_comparacion = registro_aux(parametros[1]); 
                    if (reg_comparacion != NULL && *reg_comparacion != 0) {
                        cpu_actual->PC = (uint32_t) atoi(parametros[2]);
                        log_info(logger, "## JNZ - Salto a la Instrucción: %u", cpu_actual->PC);
                    } else if (reg_comparacion == NULL) 
                        log_info(logger, "Error: Registro no válido en JNZ");
                    else 
                    {
                        log_info(logger, "## JNZ - No se realiza salto porque el valor es 0");
                        cpu_actual->PC++;
                    }
                    break;
                }
                case LOG:  { 
                    uint32_t * registro_a_leer = registro_aux(parametros[2]);
                    log_info(logger, "Valor del registro %i: %d", parametros[1], *registro_a_leer);
                    break;
                }
                default: {
                    log_info(logger, "Error: Operación no reconocida");
                    break;
                }
            }
            break;
        }
        case SYSCALL: {
			char **parametros = instruccion_partida->texto_partido;
            protocolo_socket syscall = instruccion_partida->syscall;
            manejar_syscall(syscall, instruccion_partida->prioridad, parametros);
            break;
        }
        default: {
            log_info(logger, "Error: Tipo de instrucción no reconocido");
            break;
        }
    }
}

void manejar_syscall(protocolo_socket syscall, int prioridad, char ** texto){
	encolar_interrupcion(syscall, prioridad, texto);
}

void encolar_interrupcion(protocolo_socket tipo, int prioridad, char** texto) { 
    t_interrupcion* nueva_interrupcion = malloc(sizeof(t_interrupcion));
    nueva_interrupcion->tipo = tipo;
    nueva_interrupcion->parametro = texto;
    nueva_interrupcion->prioridad = prioridad;

    list_add(lista_interrupciones, nueva_interrupcion);
    log_info(logger, "Interrupción agregada: %i con prioridad %d", tipo, prioridad);
	flag_hay_interrupcion = true;
    sem_post(sem_lista_interrupciones);
}

void checkInterrupt() { //el checkInterrupt se corre siempre -> interrupcion -> tipo 
    
	interrupcion_actual = obtener_interrupcion();

	list_clean(lista_interrupciones);

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
					//encolar_interrupcion(FIN_QUANTUM, 2 ,interrupcion_actual->parametro);
                    break;
                }
            log_info(logger, "## Interrupción FIN_QUANTUM cancelada, PID: %d TID:%d", pid_actual, tid_actual);
            break;
        case PROCESS_CREATE_OP:
            log_info(logger, "## syscall Interrupción PROCESS_CREATE_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case PROCESS_EXIT_OP:
            log_info(logger, "## syscall Interrupción PROCESS_EXIT_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case THREAD_CREATE_OP:
            log_info(logger, "## syscall THREAD_CREATE_OP recibida");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case THREAD_CANCEL_OP:
            log_info(logger, "## syscall THREAD_CANCEL_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case THREAD_EXIT_OP:
            log_info(logger, "## syscall THREAD_EXIT_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case THREAD_JOIN_OP:
            log_info(logger, "## Interrupción THREAD_JOIN_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);  
            break; 
        case MUTEX_CREATE_OP:
            log_info(logger, "## syscall Interrupción MUTEX_CREATE_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case MUTEX_LOCK_OP:
            log_info(logger, "## syscall Interrupción MUTEX_LOCK_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case MUTEX_UNLOCK_OP:
            log_info(logger, "## syscall Interrupción MUTEX_UNLOCK_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case DUMP_MEMORY_OP:
            log_info(logger, "##syscall  Interrupción DUMP_MEMORY_OP recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;
        case SEGMENTATION_FAULT:
            log_info(logger, "## Interrupción SEGMENTATION_FAULT recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2, interrupcion_actual->parametro);
            break;    
        case IO_SYSCALL:
            log_info(logger, "##syscall  Interrupción IO_SYSCALL recibida ");
            //encolar_interrupcion(interrupcion_actual->tipo, 2,interrupcion_actual->parametro);
            break;
        default:
            log_error(logger, "Instruccion invalida %d", interrupcion_actual->tipo);
            //encolar_interrupcion(ERROR, 2, interrupcion_actual->parametro); // se envia un "ERROR"
            break;
    }
    //free(interrupcion_actual);
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
        log_warning(logger, "Se tiene esta cantidad de interrupciones encoladas: %d", list_size(lista_interrupciones));
        t_interrupcion* interrupcion_actual2 = list_get(lista_interrupciones, i);
        if (interrupcion_actual2->prioridad < interrupcion_mayor_prioridad->prioridad) { 
    // Si encontramos una interrupción con menor prioridad numérica (prioridad más alta), actualizamos nuestro candidato.
            interrupcion_mayor_prioridad = interrupcion_actual2;
            indice_mayor_prioridad = i;
        }
        log_info(logger, "La interrupcion actual es: %d", interrupcion_actual2->tipo);
    }
    
    // Una vez que encontramos la interrupción con mayor prioridad, la eliminamos de la lista
    pthread_mutex_lock(mutex_lista_interrupciones);
    t_interrupcion* interrupcion_removida = list_remove(lista_interrupciones, indice_mayor_prioridad);
    list_clean(lista_interrupciones);
    pthread_mutex_unlock(mutex_lista_interrupciones);

    return interrupcion_mayor_prioridad;
}

uint32_t* registro_aux(char* reg) {
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

void traducir_direccion( uint32_t dir_logica, uint32_t *dir_fisica) {
    *dir_fisica = base_actual + dir_logica; 
    if (*dir_fisica >= base_actual + limite_actual) { // Validación de segmento
        log_info(logger,"Error: Segmentation Fault (Acceso fuera de límites)\n");
        char* texto[1];
        encolar_interrupcion(SEGMENTATION_FAULT,1,texto); // Sale por error
    }
}

void *conexion_kernel_interrupt(void* arg_kernelI)
{
	argumentos_thread * args = arg_kernelI; 
	t_list *paquete;
	int tid;

	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor de Interrupt listo para recibir al cliente Kernel");
	
	pthread_mutex_lock(mutex_kernel_interrupt);
	socket_conexion_kernel_interrupt = esperar_cliente(server);
	pthread_mutex_unlock(mutex_kernel_interrupt);

	log_info(logger, "Se realizó la conexion Kernel - Cpu Interrupt");
	sem_post(sem_conexion_kernel_interrupt);
	while(true){
		int cod_op = recibir_operacion(socket_conexion_kernel_interrupt);
		switch (cod_op)
		{
			case FIN_QUANTUM:
				paquete = recibir_paquete(socket_conexion_kernel_interrupt);
				tid_de_interrupcion_FIN_QUANTUM = *(int *)list_remove(paquete, 0);
				list_destroy(paquete);
				char* texto[1];
                if(list_size(lista_interrupciones)>1)
                    log_info(logger, "Se descartó interrupción FIN_QUANTUM");
                else{
				if(tid_actual == tid_de_interrupcion_FIN_QUANTUM){
					pthread_mutex_lock(mutex_lista_interrupciones);
					encolar_interrupcion(FIN_QUANTUM, 3, texto);
					pthread_mutex_unlock(mutex_lista_interrupciones);
					log_info(logger, "Se recibió interrupción FIN_QUANTUM");
				}
                }
				break;
			
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				return (void *)EXIT_FAILURE;
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}
	}
	close(socket_conexion_kernel_interrupt);
    return (void *)EXIT_SUCCESS;
}

void *cliente_conexion_memoria(void * arg_memoria){

	argumentos_thread * args = arg_memoria;
	do
	{
		socket_conexion_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(socket_conexion_memoria == -1);
	sem_post(sem_conexion_memoria);
}

void inicializar_estructuras_cpu(){
	lista_interrupciones = list_create();
	inicializar_semaforos_cpu();
	inicializar_registros_cpu();
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