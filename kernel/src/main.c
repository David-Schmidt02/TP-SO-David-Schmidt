#include <main.h>

t_log *logger;

int main(int argc, char* argv[]) {
    
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
	pthread_create(&tid_memoria, NULL, conexion_memoria, (void *)&arg_memoria);
    pthread_create(&tid_cpu_dispatch, NULL, conexion_cpu_dispatch, (void *)&arg_cpu_dispatch);
    pthread_create(&tid_cpu_interrupt, NULL, conexion_cpu_interrupt, (void *)&arg_cpu_interrupt);

    //espero fin conexiones
	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_cpu_dispatch, ret_value);
	pthread_join(tid_cpu_interrupt, ret_value);
	//espero fin conexiones
}
void *conexion_memoria(void * arg_memoria){

	argumentos_thread * args = arg_memoria;
	t_paquete* send_handshake;
	int conexion_kernel_memoria;
	protocolo_socket op;
	int flag=1;
	do
	{
		conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_memoria == -1);
	
	
	send_handshake = crear_paquete(HANDSHAKE);
	
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
	int conexion_kernel_cpu;
	protocolo_socket op;
	int flag=1;
	do
	{
		conexion_kernel_cpu = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu == -1);
	
	
	send_handshake = crear_paquete(HANDSHAKE);
	
	while(flag){
		enviar_paquete(send_handshake, conexion_kernel_cpu);
		sleep(1);
		op = recibir_operacion(conexion_kernel_cpu);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de memoria");
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
	int flag=1;
	do
	{
		conexion_kernel_cpu = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_kernel_cpu == -1);
	
	
	send_handshake = crear_paquete(HANDSHAKE);
	
	while(flag){
		enviar_paquete(send_handshake, conexion_kernel_cpu);
		sleep(1);
		op = recibir_operacion(conexion_kernel_cpu);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de memoria");
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
