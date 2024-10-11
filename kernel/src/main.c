#include <main.h>
#include <pcb.h>

t_log *logger;
t_list* lista_global_tcb;
t_list* procesos_a_crear_NEW;
t_tcb* hilo_actual;
int conexion_kernel_cpu;

/*
Anotaciones de lo que entiendo que falta hacer en Kernel
main.c
    -> Crear dos sockets para mantener una conexión constante con CPU
    -> Crear una funcion que a partir de cada petición necesaria a memoria se cree una conexión efímera
    -> Un proceso inicial al inicializar el módulo kernel
*/

int main(int argc, char* argv[]) {
    
	// Inicializo las variables globales
	lista_global_tcb = list_create();
    hilo_actual = NULL;

    pthread_t tid_memoria;
    pthread_t tid_cpu_dispatch;
    pthread_t tid_cpu_interrupt;
    argumentos_thread arg_memoria;
    argumentos_thread arg_cpu_dispatch;
    argumentos_thread arg_cpu_interrupt;

    void *ret_value;

    logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/kernel.config");

    //planificador
    //

    //conexiones
	arg_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    arg_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
    
    arg_cpu_dispatch.puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    arg_cpu_dispatch.ip = config_get_string_value(config, "IP_CPU");

    arg_cpu_interrupt.puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    arg_cpu_interrupt.ip = config_get_string_value(config, "IP_CPU");

    //conexiones

	/*Es correcto inicializar la conexion a memoria de esta forma si se debe realizar para cada peticion
	una conexion efímera?*/
	
	pthread_create(&tid_memoria, NULL, conexion_memoria, (void *)&arg_memoria.puerto);
    pthread_create(&tid_cpu_dispatch, NULL, conexion_cpu_dispatch, (void *)&arg_cpu_dispatch.puerto);
    pthread_create(&tid_cpu_interrupt, NULL, conexion_cpu_interrupt, (void *)&arg_cpu_interrupt.puerto);

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
	conexion_kernel_cpu;
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
