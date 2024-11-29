#include <main.h>
#include <pcb.h>
#include <IO.h>
#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>

t_log *logger;

//Config
t_config *config;
char * algoritmo;
int quantum;
//

//Conexion con Memoria
int conexion_kernel_memoria;

t_list *lista_t_peticiones;
sem_t * sem_lista_t_peticiones;
pthread_mutex_t *mutex_lista_t_peticiones;

sem_t * sem_proceso_finalizado; // falta inicializar

pthread_mutex_t mutex_respuesta_desde_memoria = PTHREAD_MUTEX_INITIALIZER; // falta inicializar
pthread_cond_t cond_respuesta_desde_memoria = PTHREAD_COND_INITIALIZER; // falta inicializar
//

//Conexion con CPU
int conexion_kernel_cpu;

pthread_mutex_t mutex_socket_cpu_dispatch;
pthread_mutex_t mutex_socket_cpu_interrupt;
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

t_cola_IO *colaIO;

int main(int argc, char* argv[]) {
	
    lista_mutexes = list_create(); //esta lista de mutex es una lista a parte de la que tenemos en el tcb
	inicializar_estructuras();

	lista_global_tcb = list_create();
	//hilo_actual=NULL;
	//pthread_mutex_init(&mutex_socket_cpu_dispatch, NULL);
    //pthread_mutex_init(&mutex_socket_cpu_interrupt, NULL);

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
	pthread_create(&tid_entradaSalida, NULL, acceder_Entrada_Salida, (void *)&arg_planificador); //PREGUNTAR TEMA PUERTO

    //conexiones
	arg_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    arg_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
    
    arg_cpu_dispatch.puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    arg_cpu_dispatch.ip = config_get_string_value(config, "IP_CPU");

    arg_cpu_interrupt.puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    arg_cpu_interrupt.ip = config_get_string_value(config, "IP_CPU");

    //conexiones
	lista_t_peticiones = list_create();
	pthread_create(&tid_memoria, NULL, administrador_peticiones_memoria, (void *)&arg_memoria);
    pthread_create(&tid_cpu_dispatch, NULL, conexion_cpu_dispatch, (void *)&arg_cpu_dispatch);
    pthread_create(&tid_cpu_interrupt, NULL, conexion_cpu_interrupt, (void *)&arg_cpu_interrupt);
	
    //espero fin conexiones
	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_cpu_dispatch, ret_value);
	pthread_join(tid_cpu_interrupt, ret_value);
	//espero fin conexiones
}
void *conexion_memoria(void * arg_memoria){

	argumentos_thread *args = arg_memoria;
	t_paquete *send_handshake;
	int conexion_kernel_memoria;
	protocolo_socket op;
	char* valor = "conexion kernel";
	int flag=1;
	do
	{
		conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_memoria == -1);

	send_handshake = crear_paquete(HANDSHAKE);
	agregar_a_paquete (send_handshake, valor , strlen(valor)+1); 
	
	while(flag){
		enviar_paquete(send_handshake, conexion_kernel_memoria);
		sleep(1);
		op = recibir_operacion(conexion_kernel_memoria);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de memoria");
			break;
		case TERMINATE:
			flag = 0;
			break;

		default:
			break;
		}
	}

	eliminar_paquete(send_handshake);
	liberar_conexion(conexion_kernel_memoria);
    return (void *)EXIT_SUCCESS;
}

void *conexion_cpu_dispatch(void * arg_cpu){

	argumentos_thread * args = arg_cpu;
	t_paquete* send_handshake;
	protocolo_socket op;
	int flag=1;
	char* valor = "conexion kernel->cpu dispatch";
	do
	{
		conexion_kernel_cpu = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu == -1);
	
	
	send_handshake = crear_paquete(HANDSHAKE);
	agregar_a_paquete (send_handshake, valor , strlen(valor)+1); 

	while(flag){
		enviar_paquete(send_handshake, conexion_kernel_cpu);
		sleep(1);
		op = recibir_operacion(conexion_kernel_cpu);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de cpu_dispatch");
			break;
		case INSTRUCCIONES:
			log_info(logger, "Recibi el archivo de instruccciones de memoria");
			break;
		case TERMINATE:
			flag = 0;
			break;

		default:
			break;
		}
	}

	eliminar_paquete(send_handshake);
	liberar_conexion(conexion_kernel_cpu);
    return (void *)EXIT_SUCCESS;
}

void *conexion_cpu_interrupt(void * arg_cpu){

	argumentos_thread * args = arg_cpu;
	t_paquete* send_handshake;
	int conexion_kernel_cpu;
	protocolo_socket op;
	char* valor = "conexion kernel->cpu interrupt";
	int flag=1;
	do
	{
		conexion_kernel_cpu = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu == -1);
	
	
	send_handshake = crear_paquete(HANDSHAKE);
	agregar_a_paquete (send_handshake, valor , strlen(valor)+1); 
	
	while(flag){
		enviar_paquete(send_handshake, conexion_kernel_cpu);
		sleep(1);
		op = recibir_operacion(conexion_kernel_cpu);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de cpu_interrupt");
			break;
		case INSTRUCCIONES:
			log_info(logger, "Recibi el archivo de instruccciones de memoria");
			break;
		case TERMINATE:
			flag = 0;
			break;

		default:
			break;
		}
	}

	eliminar_paquete(send_handshake);
	liberar_conexion(conexion_kernel_cpu);
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

		do
		{
			conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
			sleep(1);

		}while(conexion_kernel_memoria == -1);
		args_peticion.peticion = peticion;
		args_peticion.socket = conexion_kernel_memoria; 
		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)&args_peticion);
		log_info(logger, "Nueva peticion iniciada");
	}
    pthread_exit(EXIT_SUCCESS);
}

void *peticion_kernel(void * args){
	t_paquete_peticion args_peticion = *args;
	t_pcb* proceso = args_peticion.peticion->proceso;
	t_tcb* hilo = args_peticion.peticion->hilo;
	t_paquete* send_protocolo;
	char * op;

	switch (args_peticion.peticion.tipo)//
	{
	case CREATE_PROCESS:
		send_protocolo = crear_paquete(CREATE_PROCESS);
		agregar_a_paquete(send_protocolo, proceso , strlen(proceso)+1); 
		enviar_paquete(send_protocolo, args_peticion.socket);
		sleep(1);
		op = recibir_operacion(args_peticion.socket);
		switch (op)
		{
			case SUCCESS:
				log_info(logger, "'SUCCESS' recibido desde Memoria, inicializando proceso");
				args_peticion.peticion->respuesta_exitosa = true;
			case ERROR:
				log_info(logger, "'ERROR' recibido desde Memoria, no se inicializa el proceso");
				args_peticion.peticion->respuesta_exitosa = false;
			default:
				break;
			}
		pthread_mutex_lock(&mutex_respuesta_desde_memoria);
        args_peticion.peticion->respuesta_recibida = true;
        pthread_cond_signal(&cond_respuesta_desde_memoria);
        pthread_mutex_unlock(&mutex_respuesta_desde_memoria);
        break;
	case EXIT_PROCESS:
		break;
	case CREATE_THREAD:
		break;
	case EXIT_THREAD:
		break;
	case DUMP_MEMORY:
		break;
	default:
		break;
	}

eliminar_paquete(send_protocolo);
liberar_conexion(args_peticion.socket);
return (void *)EXIT_SUCCESS;
}

void encolar_peticion_memoria(t_peticion * peticion){
        // Encolar la petición
        pthread_mutex_lock(mutex_lista_t_peticiones);
        list_add(lista_t_peticiones, peticion);
        pthread_mutex_unlock(mutex_lista_t_peticiones);
        sem_post(sem_lista_t_peticiones);
        pthread_mutex_unlock(mutex_procesos_a_crear);
    // Esperar respuesta de memoria
        pthread_mutex_lock(&mutex_respuesta_desde_memoria);
        while (!peticion->respuesta_recibida) {
            pthread_cond_wait(&cond_respuesta_desde_memoria, &mutex_respuesta_desde_memoria);
        }
        pthread_mutex_unlock(&mutex_respuesta_desde_memoria);
}

void inicializar_estructuras(){
	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    config = config_create("config/kernel.config");
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");
	inicializar_semaforos();
	inicializar_colas_largo_plazo();
	inicializar_colas_corto_plazo();

}

void inicializar_semaforos(){
	inicializar_semaforos_conexiones();
	inicializar_semaforos_corto_plazo(); 
	inicializar_semaforos_largo_plazo(); 
	inicializar_lista_semaforos_peticiones(); 
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
}

void inicializar_semaforos_peticiones(){
 	lista_t_peticiones = NULL;
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
    sem_init(sem_lista_t_peticiones, 0, 0);
	
	// Semáforos para manejar la respuesta desde memoria
	pthread_mutex_t mutex_respuesta_desde_memoria = malloc(sizeof(pthread_mutex_t));
	if (mutex_respuesta_desde_memoria == NULL) {
        perror("Error al asignar memoria para mutex de respuesta desde memoria");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(mutex_respuesta_desde_memoria, NULL);

	cond_respuesta_desde_memoria = malloc(sizeof(pthread_cond_t));
    if (cond_respuesta_desde_memoria == NULL) {
        perror("Error al asignar memoria para variable de condición de respuesta desde memoria");
        exit(EXIT_FAILURE);
    }
    pthread_cond_init(cond_respuesta_desde_memoria, NULL);
}

void inicializar_colas_largo_plazo(){
	t_cola_proceso* procesos_cola_ready = inicializar_cola_procesos_ready();
	t_cola_procesos_a_crear* procesos_a_crear = inicializar_cola_procesos_a_crear();
}

void inicializar_colas_corto_plazo(){
	hilos_cola_ready = inicializar_cola_hilo(READY);
	hilos_cola_bloqueados = inicializar_cola_hilo(BLOCK);
	hilos_cola_exit = inicializar_cola_hilo(EXIT);
	colas_multinivel = inicializar_colas_multinivel();
}

/*Peticiones que se deben enviar a memoria
- Creacion de Proceso
- Finalizacion de Proceso
- Creacion de Hilo
- Finalizacion de Hilo
- Dump Memory
*/