#include <main.h>
#include <pcb.h>
#include <IO.h>
#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>

t_log *logger;
t_list *lista_t_peticiones;
t_list* lista_global_tcb; // lista para manipular los hilos

int conexion_kernel_cpu;

t_list* lista_mutexes;
t_list* lista_procesos;
t_cola_IO *colaIO;

// Listas para planificar los hilos (corto plazo)
t_config *config;

// Mutex para las conexiones con CPU
pthread_mutex_t mutex_socket_cpu_dispatch;
pthread_mutex_t mutex_socket_cpu_interrupt;
pthread_mutex_t *mutex_lista_t_peticiones;

//semaforos
sem_t *sem_lista_t_peticiones;

/*
Anotaciones de lo que entiendo que falta hacer en Kernel
main.c
    -> Crear una funcion que a partir de cada petición necesaria a memoria se cree una conexión efímera
    -> Un proceso inicial al inicializar el módulo kernel
*/

int main(int argc, char* argv[]) {
    
	int quantum = config_get_int_value(config, "QUANTUM");
	printf(quantum);
	
    lista_mutexes = list_create(); //esta lista de mutex es una lista a parte de la que tenemos en el tcb
	lista_procesos = list_create();
	
	// Inicializo las variables globales -> despues puedo englobar en otra funcion donde inicialice:
	//variables globales, listas de hilos, listas de procesos, listas de mutexes, etc...
	
	lista_global_tcb = list_create();
	//hilo_actual=NULL;
	pthread_mutex_init(&mutex_socket_cpu_dispatch, NULL);
    pthread_mutex_init(&mutex_socket_cpu_interrupt, NULL);

    pthread_t tid_memoria;
    pthread_t tid_cpu_dispatch;
    pthread_t tid_cpu_interrupt;
	pthread_t tid_entradaSalida;
    argumentos_thread arg_memoria;
    argumentos_thread arg_cpu_dispatch;
    argumentos_thread arg_cpu_interrupt;
	argumentos_thread arg_planificador;

    void *ret_value;

    logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    config = config_create("config/kernel.config");

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
	t_peticion *peticion; //FALTA CREAR STRUCT
	argumentos_thread * args = arg_server;
	pthread_t aux_thread;
	int conexion_kernel_memoria;
	
	while(flag){
		sem_wait(sem_lista_t_peticiones);
		pthread_mutex_lock(mutex_lista_t_peticiones);
		peticion = list_remove(lista_t_peticiones, 0);
		pthread_mutex_unlock(mutex_lista_t_peticiones);

		do
		{
			conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
			sleep(1);

		}while(conexion_kernel_memoria == -1);

		//dependiendo de la peticion -> switch case
		//case x:
		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)&conexion_kernel_memoria);
		log_info(logger, "nueva peticion iniciada");
		//case terminate:
		//flag = 0;
	}

	close(server);
    pthread_exit(EXIT_SUCCESS);
}
void *peticion_kernel(void * args){
	int conexion_kernel_memoria = *args;
	//manejo de peticion
	//recibir info de memoria
	//guarda la info en var global
	
	//termina
}

/*Peticiones que se deben enviar a memoria
- Creacion de Proceso
- Finalizacion de Proceso
- Creacion de Hilo
- Finalizacion de Hilo
- Dump Memory
*/