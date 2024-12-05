#include <main.h>

t_log *logger;
RegistroCPU *cpu;
int conexion_cpu_memoria;
int pid;
int tid;
t_pcb *pcb;

int tid_actual;

//hay que inicializar
t_list* lista_interrupciones;
sem_t * sem_lista_interrupciones;
pthread_mutex_t *mutex_lista_interrupciones;
//hay que inicializar

int main(int argc, char* argv[]) {

    pthread_t tid_kernelI;
	pthread_t tid_kernelD;
    pthread_t tid_memoria;

	argumentos_thread arg_kernelD;
	argumentos_thread arg_kernelI;
    argumentos_thread arg_memoria;

    logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_DEBUG);
    t_config *config = config_create("config/cpu.config");

    void *ret_value = NULL;

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
	inicializar_cpu_contexto(cpu);
	inicializar_lista_interrupciones();
	fetch(pcb);
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
			switch (cod_op){
				case HANDSHAKE:
					handshake_recv = recibir_paquete(socket_cliente_kernel);
					log_info(logger, "me llego: kernel dispatch\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_cliente_kernel);
					break;
				// hilo_actual = hilo;
				case INFO_HILO:
					t_list *paquete = recibir_paquete(socket_cliente_kernel);
					int tid = *(int *)list_remove(paquete, 0);
					list_destroy_and_destroy_elements(paquete, free);
					tid_actual = tid;
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
					log_info(logger, "me llego: kernel interrupt\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_cliente_kernel);
					break;
				case FIN_QUANTUM:
					t_list *paquete = recibir_paquete(conexion_cpu_memoria);
					int tid = *(int *)list_remove(paquete, 0);
					list_destroy_and_destroy_elements(paquete, free);
					agregar_interrupcion(FIN_QUANTUM, 3, tid)
					log_info(logger, "Se recibió interrupción FIN_QUANTUM");
					//manejar_fin_quantum(socket_cliente_kernel);
					break;

				case THREAD_JOIN_OP:
					log_info(logger, "Se recibió interrupción THREAD_JOIN_OP");
					manejar_thread_join_op(socket_cliente_kernel);
					break;

				case IO_SYSCALL:
					log_info(logger, "Se recibió syscall de tipo IO");
					manejar_syscall_io(socket_cliente_kernel);
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
		switch (op){
			case HANDSHAKE:
				log_info(logger, "recibi handshake de memoria");
				break;
			case INSTRUCCIONES: {
						log_info(logger, "Instrucción recibida de memoria");

						// Recibir el TID, PID y la instrucción
						t_list *paquete = recibir_paquete(conexion_cpu_memoria);
						int tid = *(int *)list_remove(paquete, 0);
						int pid = *(int *)list_remove(paquete, 0);
						char *instruccion = (char *)list_remove(paquete, 0);
						list_destroy_and_destroy_elements(paquete, free);

						// Decodificar y ejecutar la instrucción
						decode(&cpu, instruccion);

						// Enviar el contexto actualizado a memoria
						enviar_contexto_de_memoria(&cpu, pid);

						// Notificar si hubo interrupciones generadas
						if (cpu_genero_interrupcion(&cpu)) {//basicamente pregunta si hubo segmentation fault
							log_warning(logger, "Notificando interrupción generada memoria");
							notificar_kernel_interrupcion(pid, tid); //ponele que sea cod_op mem dump. Solo le mandaria el contexto de ejecion nomas. 
						}
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
}

