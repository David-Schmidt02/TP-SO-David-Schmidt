#include <main.h>

t_log *logger;

int main(int argc, char* argv[]) {

    pthread_t tid_memoria;

	argumentos_thread arg_memoria;

    logger = log_create("filesystem.log", "filesystem", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/filesystem.config");

    void *ret_value;

    //conexiones

   	arg_memoria.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

    //conexiones
	pthread_create(&tid_memoria, NULL, conexion_memoria, (void *)&arg_memoria);
	//conexiones

    //espero fin conexiones
	
	pthread_join(tid_memoria, ret_value);

	//espero fin conexiones

}


void *conexion_memoria(void* arg_memoria)
{
	argumentos_thread * args = arg_memoria; 
	t_paquete *handshake_send;
	t_list *handshake_recv;
	char * handshake_texto = "handshake";
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente memoria");
	
	int socket_cliente_memoria = esperar_cliente(server);
	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


		while(true){
			int cod_op = recibir_operacion(socket_cliente_memoria);
			switch (cod_op)
			{
				case HANDSHAKE:
					handshake_recv = recibir_paquete(socket_cliente_memoria);
					log_info(logger, "me llego:\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_cliente_memoria);
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
	close(socket_cliente_memoria);
    return (void *)EXIT_SUCCESS;
}
