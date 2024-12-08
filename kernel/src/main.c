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

sem_t * sem_proceso_finalizado; // utilizado para reintentar crear procesos

pthread_mutex_t * mutex_respuesta_desde_memoria;
//pthread_cond_t * cond_respuesta_desde_memoria;
//

//Conexion con CPU
int conexion_kernel_cpu_dispatch;
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

//Colas y semáforos del planificador de corto plazo
t_cola_hilo* hilos_cola_ready;
pthread_mutex_t * mutex_hilos_cola_ready;
sem_t * sem_estado_hilos_cola_ready;

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

struct timeval tiempo_inicio_quantum; 
pthread_mutex_t mutex_tiempo_inicio = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char* argv[]) {
	
	if (argc < 4) {
    fprintf(stderr, "Uso: %s <ruta_archivo_pseudocodigo> <tamanio_proceso> <prioridad_proceso>\n", argv[0]);
    return EXIT_FAILURE;
	}

    lista_mutexes = list_create(); //esta lista de mutex es una lista a parte de la que tenemos en el tcb
	inicializar_estructuras();

	lista_global_tcb = list_create();

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

    //planificador
	pthread_create(&tid_entradaSalida, NULL, acceder_Entrada_Salida, (void *)&arg_planificador);
	
	log_info(logger,"Creacion del hilo para el planificador de largo plazo\n");
    pthread_t hilo_largo_plazo;
    pthread_create(&hilo_largo_plazo, NULL, (void *)largo_plazo_fifo, NULL);
    sleep(1);

    log_info(logger,"Creacion del hilo para el planificador de corto plazo\n");
    pthread_t hilo_corto_plazo;
    pthread_create(&hilo_corto_plazo, NULL, (void *)planificador_corto_plazo_hilo, NULL);

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
	
    return (void *)EXIT_SUCCESS;
}

void *conexion_cpu_interrupt(void * arg_cpu){

	argumentos_thread * args = arg_cpu;
	do
	{
		conexion_kernel_cpu_interrupt = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu_interrupt == -1);
		
    return (void *)EXIT_SUCCESS;
}

void *administrador_peticiones_memoria(void* arg_server){
	t_peticion *peticion;
	t_paquete_peticion args_peticion; 
	argumentos_thread * args = arg_server;
	pthread_t aux_thread;
	
	while(1){
		log_info(logger, "Entraste al While del administrador de peticiones");
		sem_wait(sem_lista_t_peticiones);
		log_info(logger, "Entró una peticion a la lista de peticiones");
		pthread_mutex_lock(mutex_lista_t_peticiones);
		peticion = list_remove(lista_t_peticiones, 0);
		pthread_mutex_unlock(mutex_lista_t_peticiones);
		do{
			conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
			sleep(1);

		}while(conexion_kernel_memoria == -1);
		args_peticion.peticion = peticion;
		args_peticion.socket = conexion_kernel_memoria; 
		log_info(logger, "Se crea un hilo individual para la peticion");
		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)&args_peticion);
		pthread_detach(aux_thread);
	}
    pthread_exit(EXIT_SUCCESS);
}

void *peticion_kernel(void *args) {
    t_paquete_peticion *args_peticion = args;
    t_peticion *peticion = args_peticion->peticion;
    t_pcb *proceso = peticion->proceso;
    t_tcb *hilo = peticion->hilo;
    t_paquete *send_protocolo;
    protocolo_socket op;
	log_info(logger, "Se envía la peticion a memoria");
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
			sem_post(sem_proceso_finalizado);
            break;

        case THREAD_CREATE_OP:
            send_protocolo = crear_paquete(THREAD_CREATE_OP);
            agregar_a_paquete(send_protocolo, hilo, sizeof(t_tcb));
            
            t_list_iterator * iterator = list_iterator_create(hilo->instrucciones);
            char *aux_instruccion;
            while(list_iterator_has_next(iterator)){
                aux_instruccion = list_iterator_next(iterator);
                agregar_a_paquete(send_protocolo, &aux_instruccion, strlen(aux_instruccion)+1);
            }
			log_info(logger, "Se crea la peticion de THREAD CREATE");
            break;

        case THREAD_EXIT_OP:
            send_protocolo = crear_paquete(THREAD_EXIT_OP);
            agregar_a_paquete(send_protocolo, hilo, sizeof(t_tcb));
			log_info(logger, "Se crea la peticion de THREAD EXIT");
            break;

		case THREAD_CANCEL_OP:
            send_protocolo = crear_paquete(THREAD_CANCEL_OP);
            agregar_a_paquete(send_protocolo, hilo, sizeof(t_tcb));
			log_info(logger, "Se crea la peticion de THREAD EXIT");
            break;

        case DUMP_MEMORY_OP:
            send_protocolo = crear_paquete(DUMP_MEMORY_OP);
			agregar_a_paquete(send_protocolo, &proceso_actual->pid, sizeof(int));
            agregar_a_paquete(send_protocolo, &hilo_actual->tid, sizeof(int));
			log_info(logger, "Se crea la peticion de DUMP MEMORY");
            break;

        default:
            log_error(logger, "Tipo de operación desconocido: %d", peticion->tipo);
            return NULL;
    }

    enviar_paquete(send_protocolo, args_peticion->socket);
    log_info(logger, "Petición enviada a memoria, esperando respuesta...");

    // Esperar respuesta bloqueante -> esta es la respuesta esperada desde memoria
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
	log_info(logger, "Actualizo el valor de respuesta recibida a true");
    eliminar_paquete(send_protocolo);
    liberar_conexion(args_peticion->socket);
    return NULL;
}

void encolar_peticion_memoria(t_peticion * peticion){
        // Encolar la petición
        pthread_mutex_lock(mutex_lista_t_peticiones);
		if (peticion->proceso){
			log_info(logger,"Se encola la peticion del proceso %d a la lista de peticiones", peticion->proceso->pid);
		}
		if (peticion->hilo){
			log_info(logger,"Se encola la peticion del hilo %d del proceso %d a la lista de peticiones", peticion->hilo->tid, obtener_pcb_por_pid(peticion->hilo->pid)->pid );
		}
        list_add(lista_t_peticiones, peticion);
        pthread_mutex_unlock(mutex_lista_t_peticiones);
        sem_post(sem_lista_t_peticiones);
}

void inicializar_estructuras(){
	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    config = config_create("config/kernel.config");
	if (config == NULL) {
        perror("Error al cargar el archivo de configuración");
    }
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");
	log_info(logger, "Se inicializa la lista de peticiones con list_create");
	lista_t_peticiones = list_create();
	log_info(logger, "Lista de peticiones inicializada");
	inicializar_semaforos();
	inicializar_colas_largo_plazo();
	inicializar_colas_corto_plazo();

	colaIO = malloc(sizeof(t_cola_IO));
    if (procesos_cola_ready == NULL) {
        perror("Error al asignar memoria para cola de IO");
        exit(EXIT_FAILURE);
    }
	colaIO->lista_io = list_create();
}

void inicializar_semaforos(){
	mutex_colaIO = malloc(sizeof(pthread_mutex_t));
    if (mutex_colaIO == NULL) {
        perror("Error al asignar memoria para mutex de cola IO");
        exit(EXIT_FAILURE);
    }
	pthread_mutex_init(mutex_colaIO, NULL);

	sem_estado_colaIO = malloc(sizeof(sem_t));
    if (sem_estado_colaIO == NULL) {
        perror("Error al asignar memoria para semáforo de cola IO");
        exit(EXIT_FAILURE);
    }
	sem_init(sem_estado_colaIO, 0, 0);
	log_info(logger,"Mutex y semáforo de estado para la lista de peticiones creados\n");

	inicializar_semaforos_conexion_cpu();
	inicializar_semaforos_corto_plazo(); 
	inicializar_semaforos_largo_plazo(); 
	inicializar_semaforos_peticiones(); 
}

void inicializar_semaforos_conexion_cpu(){
	mutex_socket_cpu_dispatch = malloc(sizeof(pthread_mutex_t));
    if (mutex_socket_cpu_dispatch == NULL) {
        perror("Error al asignar memoria para mutex de CPU dispatch");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_socket_cpu_dispatch, NULL);

	mutex_socket_cpu_interrupt = malloc(sizeof(pthread_mutex_t));
    if (mutex_socket_cpu_interrupt == NULL) {
        perror("Error al asignar memoria para mutex de CPU interrupt");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_socket_cpu_interrupt, NULL);
	log_info(logger,"Mutex para la conexion con cpu interrupt y cpu dispatch creados\n");
}

void inicializar_semaforos_peticiones(){
	
    mutex_lista_t_peticiones = malloc(sizeof(pthread_mutex_t));
    if (mutex_lista_t_peticiones == NULL) {
        perror("Error al asignar memoria para mutex de cola");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_lista_t_peticiones, NULL);

    sem_lista_t_peticiones = malloc(sizeof(sem_t));
    if (sem_lista_t_peticiones == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
	log_info(logger,"Mutex y semáforo de estado para la lista de peticiones creados\n");
	sem_init(sem_lista_t_peticiones, 0, 0);

	sem_estado_respuesta_desde_memoria = malloc(sizeof(sem_t));
    if (sem_estado_respuesta_desde_memoria == NULL) {
        perror("Error al asignar memoria para semáforo de cola");
        exit(EXIT_FAILURE);
    }
	log_info(logger,"Mutex y semáforo de estado para la lista de peticiones creados\n");
	sem_init(sem_estado_respuesta_desde_memoria, 0, 0);
}

void inicializar_colas_largo_plazo(){
	procesos_cola_ready = inicializar_cola_procesos_ready();
	procesos_a_crear = inicializar_cola_procesos_a_crear();
}

void inicializar_colas_corto_plazo(){
	hilos_cola_ready = inicializar_cola_hilo(READY);
	hilos_cola_bloqueados = inicializar_cola_hilo(BLOCK);
	hilos_cola_exit = inicializar_cola_hilo(EXIT);
	log_info(logger,"Cola de hilos en Ready/Block/Exit inicializadas \n");
	colas_multinivel = inicializar_colas_multinivel();

}

