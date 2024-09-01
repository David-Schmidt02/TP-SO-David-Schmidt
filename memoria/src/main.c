#include <main.h>

t_log *logger;

int main(int argc, char* argv[]) {

    pthread_t tid_cpu;
    pthread_t tid_kernel;
    pthread_t tid_fs;

    argumentos_thread arg_cpu;
    argumentos_thread arg_kernel;
    argumentos_thread arg_fs;

    logger = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/memoria.config");

    void *ret_value;

    //conexiones
	arg_cpu.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    arg_kernel.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    arg_fs.puerto = config_get_string_value(config, "PUERTO_FILESYSTEM");
    arg_fs.ip = config_get_string_value(config, "IP_FILESYSTEM");

    //conexiones
	pthread_create(&tid_cpu, NULL, conexion_cpu, (void *)&arg_cpu);
	pthread_create(&tid_kernel, NULL, conexion_kernel, (void *)&arg_kernel);
	pthread_create(&tid_fs, NULL, cliente_conexion_filesystem, (void *)&arg_fs);
	//conexiones

    //espero fin conexiones
	pthread_join(tid_cpu, ret_value);
	pthread_join(tid_kernel, ret_value);
	pthread_join(tid_fs, ret_value);
	//espero fin conexiones

}
void *conexion_cpu(void* arg_cpu)
{
	argumentos_thread * args = arg_cpu; 
	t_paquete *handshake_send;
	t_list *handshake_recv;
	char * handshake_texto = "handshake";
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente CPU");
	
	int socket_cliente_cpu = esperar_cliente(server);
	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


	while(true){
		int cod_op = recibir_operacion(socket_cliente_cpu);
		switch (cod_op)
		{
			case HANDSHAKE:
				handshake_recv = recibir_paquete(socket_cliente_cpu);
				log_info(logger, "me llego:\n");
				list_iterate(handshake_recv, (void*) iterator);
				enviar_paquete(handshake_send, socket_cliente_cpu);
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
	close(socket_cliente_cpu);
    return (void *)EXIT_SUCCESS;
}
void *conexion_kernel(void* arg_kernel)
{
	argumentos_thread * args = arg_kernel; 
	t_paquete *handshake_send;
	t_list *handshake_recv;
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
					log_info(logger, "me llego:\n");
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
void *cliente_conexion_filesystem(void * arg_fs){

	argumentos_thread * args = arg_fs;
	t_paquete* send_handshake;
	int conexion_memoria_fs;
	protocolo_socket op;
	int flag=1;
	do
	{
		conexion_memoria_fs = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_memoria_fs == -1);
	
	send_handshake = crear_paquete(HANDSHAKE);
	
	while(flag){
		enviar_paquete(send_handshake, conexion_memoria_fs);
		sleep(1);
		op = recibir_operacion(conexion_memoria_fs);
		switch (op)
		{
		case HANDSHAKE:
			log_info(logger, "recibi handshake de fs");
			break;
		case TERMINATE:
			flag = 0;
			break;

		default:
			break;
		}
	}

	eliminar_paquete(send_handshake);
	liberar_conexion(conexion_memoria_fs);
    return (void *)EXIT_SUCCESS;
}