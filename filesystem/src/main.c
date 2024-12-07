#include <main.h>

t_log *logger;
int retardo_acceso;
t_config *config;
uint32_t block_count;
int block_size;
char* mount_dir;

int main() {

    pthread_t tid_memoria;

	argumentos_thread arg_memoria;

    logger = log_create("filesystem.log", "filesystem", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/filesystem.config");

    void *ret_value; // no es mejor un NULL?

    //conexiones

   	arg_memoria.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	
	// Leer parámetros del archivo de configuración
	block_count = config_get_int_value(config, "BLOCK_COUNT");
	block_size = config_get_int_value(config, "BLOCK_SIZE");
	retardo_acceso = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
	mount_dir = config_get_string_value(config, "MOUNT_DIR");
	// Inicializar estructuras
	
	mount_dir = crear_directorio(mount_dir,"/mount_dir");
	inicializar_bitmap(mount_dir,block_count);
	inicializar_bloques(block_count,block_size,mount_dir);

	//esto es lo que tiene que hacer cuando recibe la peticion
	//
	
    //conexiones
	pthread_create(&tid_memoria, NULL, conexion_memoria, (void *)&arg_memoria);
	//conexiones

    //espero fin conexiones
	
	pthread_join(tid_memoria, ret_value);

	//espero fin conexiones
	return 0;
}

void *conexion_memoria(void* arg_memoria)
{
	argumentos_thread * args = arg_memoria; 
	t_paquete *recv;
	t_list *recv_list;
	char * handshake_texto = "conexion filesystem";
	t_config *config = config_create("config/filesystem.config");

	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente memoria");
	int socket_cliente_memoria = esperar_cliente(server);

	//HANDSHAKE_end
		while(true){
			int cod_op = recibir_operacion(socket_cliente_memoria);
			switch (cod_op)
			{
				case INIT_BITMAP:
					log_info(logger, "Solicitud para inicializar el Bitmap recibida");

					char* mount_dir = config_get_string_value(config, "MOUNT_DIR");
					if (mount_dir == NULL) {
						log_error(logger, "Error: No se encontró 'MOUNT_DIR' en la configuración.");
						return (void*)EXIT_FAILURE;
					}
					uint32_t block_count = (uint32_t) config_get_int_value(config, "BLOCK_COUNT");
					if (block_count == 0) {
						log_error(logger, "Error: No se encontró 'BLOCK_COUNT' en la configuración.");
						return (void*)EXIT_FAILURE;
					}
					inicializar_bitmap(mount_dir, block_count);
					break;
				case DUMP_MEMORY_OP:
					recv_list = recibir_paquete(socket_cliente_memoria);

					char* nombre_archivo;
					uint32_t tamanio;
					uint32_t *datos;

					nombre_archivo = list_remove(recv_list,0);
					tamanio = list_remove(recv_list,0);
					datos = list_remove(recv_list,0);

					char* dir_files = mount_dir;
					dir_files = crear_directorio(dir_files,"/files");
					espacio_disponible(tamanio);
					crear_archivo_metadata(block_count,block_size,dir_files,nombre_archivo,tamanio);
					log_info(logger, "Fin de solicitud - Archivo: <%s>", nombre_archivo);
					
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
	config_destroy(config);
    return (void *)EXIT_SUCCESS;
}

char* crear_directorio(char* base_path, char* ruta_a_agregar) {
    if (!base_path) {
        fprintf(stderr, "Error: La ruta base es NULL.\n");
        exit(EXIT_FAILURE);
    }

    size_t path_length = strlen(base_path) + strlen(ruta_a_agregar) + 1;
    char* mount_dir = malloc(path_length);
    if (!mount_dir) {
        fprintf(stderr, "Error: No se pudo asignar memoria para la ruta del directorio.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(mount_dir, path_length, "%s%s", base_path,ruta_a_agregar);

    if (mkdir(mount_dir, 0700) == 0) {
        printf("Directorio '%s' creado correctamente.\n", mount_dir);
    } else if (errno == EEXIST) {
        printf("El directorio '%s' ya existe.\n", mount_dir);
    } else {
        perror("Error al crear el directorio");
        free(mount_dir); // Liberar memoria en caso de error
        exit(EXIT_FAILURE);
    }

    return mount_dir;
}