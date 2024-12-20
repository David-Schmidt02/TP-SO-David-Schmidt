#include <main.h>
#include <IO.h>
#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>
#include <syscalls.h>

t_log *logger;

//Config
t_config *config;
char * algoritmo;
int quantum;
//

t_tcb* hilo_actual;  
t_pcb* proceso_actual;

//Conexion con Memoria
int conexion_kernel_memoria;

t_list *lista_t_peticiones;
sem_t * sem_lista_t_peticiones;
pthread_mutex_t *mutex_lista_t_peticiones;
sem_t * sem_estado_respuesta_desde_memoria;

pthread_mutex_t *mutex_socket_memoria;

sem_t * sem_proceso_finalizado; // utilizado para reintentar crear procesos

pthread_mutex_t * mutex_respuesta_desde_memoria;
//pthread_cond_t * cond_respuesta_desde_memoria;
//

//Conexion con CPU
int conexion_kernel_cpu_dispatch;
sem_t * sem_estado_conexion_cpu_dispatch;
int conexion_kernel_cpu_interrupt;

pthread_mutex_t *mutex_socket_cpu_dispatch;
pthread_mutex_t *mutex_socket_cpu_interrupt;

sem_t * sem_estado_cpu_dispatch;
sem_t * sem_estado_cpu_interrupt;
//

//Colas y semáforos del planificador de largo plazo
t_cola_proceso* procesos_cola_ready;
pthread_mutex_t * mutex_procesos_cola_ready;
sem_t * sem_estado_procesos_cola_ready;

t_cola_procesos_a_crear* procesos_a_crear;
pthread_mutex_t * mutex_procesos_a_crear;
sem_t * sem_estado_procesos_a_crear;
//

t_cola_procesos_a_crear* lista_procesos_a_crear_reintento;
pthread_mutex_t * mutex_lista_procesos_a_crear_reintento;
sem_t * sem_estado_lista_procesos_a_crear_reintento;

//Colas y semáforos del planificador de corto plazo
t_cola_hilo* hilos_cola_ready;
pthread_mutex_t * mutex_hilos_cola_ready;
sem_t * sem_estado_hilos_cola_ready;

sem_t * sem_hilo_actual_encolado;
sem_t *  sem_hilo_nuevo_encolado;

t_cola_hilo* hilos_cola_exit;
pthread_mutex_t * mutex_hilos_cola_exit;
sem_t * sem_estado_hilos_cola_exit;

t_cola_hilo* hilos_cola_bloqueados;
pthread_mutex_t * mutex_hilos_cola_bloqueados;
sem_t * sem_estado_hilos_cola_bloqueados;

t_colas_multinivel *colas_multinivel;
pthread_mutex_t * mutex_colas_multinivel;
sem_t * sem_estado_multinivel;
//

t_list* lista_global_tcb; // lista para manipular los hilos
t_list* lista_mutexes;

//Colas y semáforos de IO
t_cola_IO *colaIO;
pthread_mutex_t * mutex_colaIO;
sem_t * sem_estado_colaIO;

int main(int argc, char* argv[]) {
	
	if (argc < 4) {
    fprintf(stderr, "Uso: %s <ruta_archivo_pseudocodigo> <tamanio_proceso> <prioridad_proceso>\n", argv[0]);
    return EXIT_FAILURE;
	}

    lista_mutexes = list_create(); //esta lista de mutex es una lista a parte de la que tenemos en el tcb
	inicializar_estructuras();

	lista_global_tcb = list_create();
    lista_procesos_a_crear_reintento = malloc(sizeof(lista_procesos_a_crear_reintento));
    lista_procesos_a_crear_reintento->lista_procesos = list_create();

	char *ruta_archivo_pseudocodigo_proceso_inicial = argv[1];

	FILE * archivo_pseudocodigo_proceso_inicial = fopen(ruta_archivo_pseudocodigo_proceso_inicial, "r");
	
	if (!archivo_pseudocodigo_proceso_inicial) {
		perror("Error al abrir el archivo");
		return EXIT_FAILURE;
	}

    int tamanio_proceso_proceso_inicial = atoi(argv[2]);
	int prioridad_proceso_inicial = atoi(argv[3]);

	PROCESS_CREATE(archivo_pseudocodigo_proceso_inicial, tamanio_proceso_proceso_inicial, prioridad_proceso_inicial);
    
	
    pthread_t tid_memoria;
    pthread_t tid_cpu_dispatch;
    pthread_t tid_cpu_interrupt;
	pthread_t tid_entradaSalida;
    argumentos_thread arg_memoria;
    argumentos_thread arg_cpu_dispatch;
    argumentos_thread arg_cpu_interrupt;
	argumentos_thread arg_planificador;

    void *ret_value;

    //conexiones
	arg_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    arg_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
    
    arg_cpu_dispatch.puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    arg_cpu_dispatch.ip = config_get_string_value(config, "IP_CPU");

    arg_cpu_interrupt.puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    arg_cpu_interrupt.ip = config_get_string_value(config, "IP_CPU");

    //conexiones
	pthread_create(&tid_memoria, NULL, administrador_peticiones_memoria, (void *)&arg_memoria);
    pthread_create(&tid_cpu_dispatch, NULL, conexion_cpu_dispatch, (void *)&arg_cpu_dispatch);
    pthread_create(&tid_cpu_interrupt, NULL, conexion_cpu_interrupt, (void *)&arg_cpu_interrupt);

    //planificador
    log_info(logger,"Creacion del hilo para el hilo de IO\n");
	pthread_create(&tid_entradaSalida, NULL, acceder_Entrada_Salida, (void *)&arg_planificador);
	
	log_info(logger,"Creacion del hilo para el planificador de largo plazo\n");
    pthread_t hilo_largo_plazo;
    pthread_create(&hilo_largo_plazo, NULL, (void *)largo_plazo_fifo, NULL);

    log_info(logger,"Creacion del hilo para el planificador de corto plazo\n");
    pthread_t hilo_corto_plazo;
    pthread_create(&hilo_corto_plazo, NULL, (void *)planificador_corto_plazo_hilo, NULL);

    log_info(logger,"Creacion del hilo para el atender los procesos que no se pudieron crear\n");
    pthread_t hilo_reintentar_creacion;
    pthread_create(&hilo_reintentar_creacion, NULL, (void *)reintentar_creacion_proceso, NULL);

    //espero fin conexiones
	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_cpu_dispatch, ret_value);
	pthread_join(tid_cpu_interrupt, ret_value);
	//espero fin conexiones
}

void *conexion_cpu_dispatch(void * arg_cpu){

	argumentos_thread * args = arg_cpu;
	do
	{
		conexion_kernel_cpu_dispatch = crear_conexion(args->ip, args->puerto);
		sleep(1);
        

	}while(conexion_kernel_cpu_dispatch == -1);
    log_info(logger, "Se realizó la conexion con CPU DISPATCH");
	sem_post(sem_estado_conexion_cpu_dispatch);
    return (void *)EXIT_SUCCESS;
}

void *conexion_cpu_interrupt(void * arg_cpu){

	argumentos_thread * args = arg_cpu;
	do
	{
		conexion_kernel_cpu_interrupt = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu_interrupt == -1);
	log_info(logger, "Se realizó la conexion con CPU INTERRUPT");
    return (void *)EXIT_SUCCESS;
}

void *administrador_peticiones_memoria(void* arg_server){
	t_peticion *peticion;
	t_paquete_peticion args_peticion; 
	argumentos_thread * args = arg_server;
	pthread_t aux_thread;
	
	while(1){
		sem_wait(sem_lista_t_peticiones);
		pthread_mutex_lock(mutex_lista_t_peticiones);
		peticion = list_remove(lista_t_peticiones, 0);
		pthread_mutex_unlock(mutex_lista_t_peticiones);
		do{
			conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
			sleep(1);

		}while(conexion_kernel_memoria == -1);
		args_peticion.peticion = peticion;
		args_peticion.socket = conexion_kernel_memoria; 
		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)&args_peticion);
		pthread_detach(aux_thread);
	}
    pthread_exit(EXIT_SUCCESS);
}

void *peticion_kernel(void *args) {
    t_paquete_peticion *args_peticion = args;
    int socket = args_peticion->socket;
    t_peticion *peticion = args_peticion->peticion;
    t_pcb *proceso = peticion->proceso;
    t_tcb *hilo = peticion->hilo;
    t_paquete *send_protocolo;
    protocolo_socket op;
    switch (peticion->tipo) {
        case PROCESS_CREATE_OP:
            send_protocolo = crear_paquete(PROCESS_CREATE_OP);
            agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(proceso->pid));
            agregar_a_paquete(send_protocolo, &proceso->memoria_necesaria, sizeof(proceso->memoria_necesaria));
            agregar_a_paquete(send_protocolo, &proceso->estado, sizeof(proceso->estado));
			log_info(logger, "Se envió la peticion de PROCESS CREATE del PID: %d Tamaño: %d", proceso->pid, proceso->memoria_necesaria);
            break;

        case PROCESS_EXIT_OP:
            send_protocolo = crear_paquete(PROCESS_EXIT_OP);
            agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(int));
			log_info(logger, "Se envió la peticion de PROCESS EXIT del PID: %d", proceso->pid);
			//sem_post(sem_proceso_finalizado);
            break;

        case THREAD_CREATE_OP:
            send_protocolo = crear_paquete(THREAD_CREATE_OP);
            agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
            agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->pid));
            agregar_a_paquete(send_protocolo, &hilo->prioridad, sizeof(hilo->prioridad));
            agregar_a_paquete(send_protocolo, &hilo->estado, sizeof(hilo->estado));
            agregar_a_paquete(send_protocolo, &hilo->quantum_restante, sizeof(hilo->quantum_restante));
            t_list_iterator * iterator = list_iterator_create(hilo->instrucciones);
            char *aux_instruccion;
            while(list_iterator_has_next(iterator)){
                aux_instruccion = list_iterator_next(iterator);
                int tamanio2 = strlen(aux_instruccion)+1;
                agregar_a_paquete(send_protocolo, aux_instruccion, tamanio2);
            }
			log_info(logger, "Se envió la peticion de THREAD CREATE");
            break;

        case THREAD_EXIT_OP:
            send_protocolo = crear_paquete(THREAD_EXIT_OP);
            agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
            agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->tid));
			log_info(logger, "Se envió la peticion de THREAD EXIT");
            break;

		case THREAD_CANCEL_OP:
            send_protocolo = crear_paquete(THREAD_CANCEL_OP);
            agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
            agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->tid));
			log_info(logger, "Se crea la peticion de THREAD CANCEL");
            break;

        case DUMP_MEMORY_OP:
            send_protocolo = crear_paquete(DUMP_MEMORY_OP);
            agregar_a_paquete(send_protocolo, &hilo_actual->tid, sizeof(int));
			agregar_a_paquete(send_protocolo, &proceso_actual->pid, sizeof(int));
			log_info(logger, "Se envió la peticion de DUMP MEMORY");
            break;

        default:
            log_error(logger, "Tipo de operación desconocido: %d", peticion->tipo);
            return NULL;
    }

    enviar_paquete(send_protocolo, socket);

    // Esperar respuesta bloqueante -> esta es la respuesta esperada desde memoria
    op = recibir_operacion(socket);
    switch (op) {
        case SUCCESS:
            log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
            peticion->respuesta_exitosa = true;
            break;

        case ERROR:
            log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
            peticion->respuesta_exitosa = false;
            break;

		case OK:
            log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
            peticion->respuesta_exitosa = true;
            break;	

        default:
            log_warning(logger, "Código de operación desconocido recibido: %d", op);
            peticion->respuesta_exitosa = false;
            break;
    }
	sem_post(sem_estado_respuesta_desde_memoria);
    eliminar_paquete(send_protocolo);
    liberar_conexion(socket);

    return NULL;
}

void encolar_peticion_memoria(t_peticion * peticion){
        pthread_mutex_lock(mutex_lista_t_peticiones);
        if(peticion->tipo == DUMP_MEMORY_OP || peticion->tipo == PROCESS_EXIT_OP){
            list_add_in_index(lista_t_peticiones, 0, peticion);
        }else list_add(lista_t_peticiones, peticion);
        pthread_mutex_unlock(mutex_lista_t_peticiones);
        sem_post(sem_lista_t_peticiones);
}

void *reintentar_creacion_proceso(void * args){
    int i = 0;
    while (1)
    {
        sem_wait(sem_estado_lista_procesos_a_crear_reintento);
        pthread_mutex_lock(mutex_lista_procesos_a_crear_reintento);
        t_pcb * proceso = list_remove(lista_procesos_a_crear_reintento->lista_procesos, 0);
        pthread_mutex_unlock(mutex_lista_procesos_a_crear_reintento);
        t_peticion *peticion = malloc(sizeof(t_peticion));
        peticion->tipo = PROCESS_CREATE_OP;
        peticion->proceso = proceso;
        peticion->hilo = NULL;
        pthread_mutex_lock(mutex_socket_memoria);
        encolar_peticion_memoria(peticion);
        sem_wait(sem_estado_respuesta_desde_memoria);
        if (peticion->respuesta_exitosa) {
            proceso->estado = READY;
            encolar_proceso_en_ready(proceso);
            encolar_hilo_principal_corto_plazo(proceso);
            sem_post(sem_hilo_actual_encolado);
            
        } else 
            {
                log_error(logger, "No se pudo crear el proceso PID: %d Tamaño: %d, reitentando cuando otro proceso finalice...", proceso->pid, proceso->memoria_necesaria);
                list_add(lista_procesos_a_crear_reintento->lista_procesos, proceso);
                i++;
                if (i < list_size(lista_procesos_a_crear_reintento->lista_procesos))
                    sem_post(sem_estado_lista_procesos_a_crear_reintento);
                else
                    i = 0;
            }
        pthread_mutex_unlock(mutex_socket_memoria);
        free(peticion);
    }
}

void inicializar_estructuras(){
	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    config = config_create("config/kernel.config");
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");

	lista_t_peticiones = list_create();


	inicializar_semaforos();
	inicializar_colas_largo_plazo();
	inicializar_colas_corto_plazo();

	colaIO = malloc(sizeof(t_cola_IO));
	colaIO->lista_io = list_create();
}

void inicializar_semaforos(){
	mutex_colaIO = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_colaIO, NULL);

	sem_estado_colaIO = malloc(sizeof(sem_t));
	sem_init(sem_estado_colaIO, 0, 0);

    mutex_socket_memoria = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_socket_memoria, NULL);


	inicializar_semaforos_conexion_cpu();
	inicializar_semaforos_corto_plazo(); 
	inicializar_semaforos_largo_plazo(); 
	inicializar_semaforos_peticiones(); 
}

void inicializar_semaforos_conexion_cpu(){
	mutex_socket_cpu_dispatch = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_socket_cpu_dispatch, NULL);

	mutex_socket_cpu_interrupt = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_socket_cpu_interrupt, NULL);

    sem_estado_conexion_cpu_dispatch = malloc(sizeof(sem_t));
	sem_init(sem_estado_conexion_cpu_dispatch, 0, 0);

}

void inicializar_semaforos_peticiones(){
	
    mutex_lista_t_peticiones = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_lista_t_peticiones, NULL);

    sem_lista_t_peticiones = malloc(sizeof(sem_t));
	sem_init(sem_lista_t_peticiones, 0, 0);

	sem_estado_respuesta_desde_memoria = malloc(sizeof(sem_t));
	sem_init(sem_estado_respuesta_desde_memoria, 0, 0);
}

void inicializar_colas_largo_plazo(){
	procesos_cola_ready = inicializar_cola_procesos_ready();
	inicializar_cola_procesos_a_crear();
}

void inicializar_colas_corto_plazo(){
	hilos_cola_ready = inicializar_cola_hilo(READY);
	hilos_cola_bloqueados = inicializar_cola_hilo(BLOCK);
	hilos_cola_exit = inicializar_cola_hilo(EXIT);
	colas_multinivel = inicializar_colas_multinivel();

}

