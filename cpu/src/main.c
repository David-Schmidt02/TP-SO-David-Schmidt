#include <main.h>

t_log *logger;

int main(int argc, char* argv[]) {

    pthread_t tid_kernelI;
	pthread_t tid_kernelD;
    pthread_t tid_memoria;

	argumentos_thread arg_kernelD;
	argumentos_thread arg_kernelI;
    argumentos_thread arg_memoria;

    logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/cpu.config");

    void *ret_value;

    //conexiones
	arg_memoria.puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	arg_memoria.ip = config_get_string_value(config, "IP_MEMORIA");
    arg_kernelD.puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	arg_kernelI.puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    

    //conexiones
	pthread_create(&tid_kernelI, NULL, conexion_kernel_interrupt, (void *)&arg_kernelI);
	pthread_create(&tid_kernelD, NULL, conexion_kernel_dispatch, (void *)&arg_kernelD);
	pthread_create(&tid_memoria, NULL, cliente_conexion_memoria, (void *)&arg_memoria);
	//conexiones

    //espero fin conexiones
	
	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_kernelI, ret_value);
	pthread_join(tid_kernelD, ret_value);

	//espero fin conexiones

}

void *conexion_kernel_dispatch(void* arg_kernelD)
{
	argumentos_thread * args = arg_kernelD; 
	t_paquete *handshake_send;
	t_paquete *handshake_recv;
	char * handshake_texto = "handshake";
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente Kernel");
	
	int socket_cliente_kernel = esperar_cliente(server);
	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


		while(true){
			int cod_op = recibir_operacion(socket_cliente_kernel);
			switch (cod_op)
			{
				case HANDSHAKE:
					handshake_recv = recibir_paquete(socket_cliente_kernel);
					log_info(logger, "me llego: kernel dispatch\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_cliente_kernel);
					break;
				case IO:	
				
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					return (void *)EXIT_FAILURE;
					break;
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					break;
			}
		}
		
	close(server);
	close(socket_cliente_kernel);
    return (void *)EXIT_SUCCESS;
}

void *conexion_kernel_interrupt(void* arg_kernelI)
{
	argumentos_thread * args = arg_kernelI; 
	t_paquete *handshake_send;
	t_paquete *handshake_recv;
	char * handshake_texto = "handshake";
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente Kernel");
	
	int socket_cliente_kernel = esperar_cliente(server);
	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


		while(true){
			int cod_op = recibir_operacion(socket_cliente_kernel);
			switch (cod_op)
			{
				case HANDSHAKE:
					handshake_recv = recibir_paquete(socket_cliente_kernel);
					log_info(logger, "me llego: kernel interrupt\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_cliente_kernel);
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
		
	close(server);
	close(socket_cliente_kernel);
    return (void *)EXIT_SUCCESS;
}

void *cliente_conexion_memoria(void * arg_memoria){

	argumentos_thread * args = arg_memoria;
	t_paquete* send_handshake;
	int conexion_cpu_memoria;
	char *valor = "conexion cpu";
	protocolo_socket op;
	int flag=1;
	do
	{
		conexion_cpu_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_cpu_memoria == -1);
	
	send_handshake = crear_paquete(HANDSHAKE);
	agregar_a_paquete (send_handshake, valor , strlen(valor)+1); 
	
	while(flag){
		enviar_paquete(send_handshake, conexion_cpu_memoria);
		sleep(1);
		op = recibir_operacion(conexion_cpu_memoria);
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
	liberar_conexion(conexion_cpu_memoria);
    return (void *)EXIT_SUCCESS;
}