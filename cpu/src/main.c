#include <main.h>

t_log *logger;
RegistroCPU *cpu;
int conexion_cpu_memoria;
sem_t * sem_conexion_memoria;
int conexion_cpu_interrupt;

int socket_kernel_interrupt;
int socket_kernel_dispatch;


sem_t * sem_conexion_cpu_interrupt;
sem_t * sem_conexion_cpu_dispatch;
int pid;
int tid;
int flag_hay_contexto;

bool flag = true;

int tid_actual;

//hay que inicializar
t_list* lista_interrupciones;
sem_t * sem_lista_interrupciones;
sem_t * sem_hay_contexto;

pthread_mutex_t *mutex_lista_interrupciones;
//hay que inicializar

pthread_mutex_t *mutex_kernel_interrupt;

pthread_mutex_t *mutex_kernel_dispatch;

pthread_mutex_t *mutex_conexion_memoria;

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
	inicializar_cpu_contexto();
	inicializar_lista_interrupciones();

    //conexiones
	pthread_create(&tid_kernelI, NULL, conexion_kernel_interrupt, (void *)&arg_kernelI);
	pthread_create(&tid_kernelD, NULL, conexion_kernel_dispatch, (void *)&arg_kernelD);
	pthread_create(&tid_memoria, NULL, cliente_conexion_memoria, (void *)&arg_memoria);


	sem_wait(sem_conexion_cpu_dispatch);
	sem_wait(sem_conexion_cpu_interrupt);
	sem_wait(sem_conexion_memoria);

	while(flag){
		if(pid == 0){
			sem_wait(sem_lista_interrupciones);
			checkInterrupt();
		}else fetch();
	}
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
	
	pthread_mutex_lock(mutex_kernel_dispatch);
	socket_kernel_dispatch = esperar_cliente(server);
	log_info(logger, "Se realizó la conexion kernel-cpu dipatch");
	pthread_mutex_unlock(mutex_kernel_dispatch);
	sem_post(sem_conexion_cpu_dispatch);

	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


		while(true){
			pthread_mutex_lock(mutex_kernel_dispatch);
			int cod_op = recibir_operacion(socket_kernel_dispatch);
			switch (cod_op){
				case INFO_HILO:
					t_list *paquete = recibir_paquete(socket_kernel_dispatch);
					pthread_mutex_unlock(mutex_kernel_dispatch);
					tid = *(int *)list_remove(paquete, 0);
					pid = *(int *)list_remove(paquete, 0);
					list_destroy(paquete);
					char* texto[3];
					texto[0] = malloc(5);
					texto[1] = malloc(5);
					texto[2] = malloc(5);
					strcpy(texto[1],string_itoa(tid)); 
					strcpy(texto[2],string_itoa(pid)); 
					agregar_interrupcion(INFO_HILO,3,texto);
					break;
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					pthread_mutex_unlock(mutex_kernel_dispatch);
					return (void *)EXIT_FAILURE;
					break;
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					pthread_mutex_unlock(mutex_kernel_dispatch);
					break;
				}
			}
		
	close(server);
	close(socket_kernel_dispatch);
    return (void *)EXIT_SUCCESS;
}

void *conexion_kernel_interrupt(void* arg_kernelI)
{
	argumentos_thread * args = arg_kernelI; 
	t_paquete *handshake_send;
	t_list *handshake_recv;
	char * handshake_texto = "handshake";
	t_list *paquete;
	int tid;
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente Kernel");
	
	pthread_mutex_lock(mutex_kernel_interrupt);
	socket_kernel_interrupt = esperar_cliente(server);
	log_info(logger, "Se realizó la conexion kernel-cpu interrupt");
	pthread_mutex_unlock(mutex_kernel_interrupt);

	sem_post(sem_conexion_cpu_interrupt);
	//HANDSHAKE
	handshake_send = crear_paquete(HANDSHAKE);
	agregar_a_paquete (handshake_send, handshake_texto , strlen(handshake_texto)+1);
	//HANDSHAKE_end


		while(true){
			pthread_mutex_lock(mutex_kernel_interrupt);
			int cod_op = recibir_operacion(socket_kernel_interrupt);
			switch (cod_op)
			{
				case HANDSHAKE:
					handshake_recv = recibir_paquete(socket_kernel_interrupt);
					log_info(logger, "me llego: kernel interrupt\n");
					list_iterate(handshake_recv, (void*) iterator);
					enviar_paquete(handshake_send, socket_kernel_interrupt);
					pthread_mutex_unlock(mutex_kernel_interrupt);
					break;
				case FIN_QUANTUM:
					paquete = recibir_paquete(socket_kernel_interrupt);
					tid = *(int *)list_remove(paquete, 0);
					list_destroy_and_destroy_elements(paquete, free);
					char* texto[1];
					pthread_mutex_unlock(mutex_kernel_interrupt);
					agregar_interrupcion(FIN_QUANTUM,3,texto);

					log_info(logger, "Se recibió interrupción FIN_QUANTUM");
					break;
				
				case -1:
					log_error(logger, "el cliente se desconecto. Terminando servidor");
					pthread_mutex_unlock(mutex_kernel_interrupt);
					return (void *)EXIT_FAILURE;
					break;
				default:
					log_warning(logger,"Operacion desconocida. No quieras meter la pata");
					pthread_mutex_unlock(mutex_kernel_interrupt);
					break;
			}
		}
		
	close(server);
	close(socket_kernel_interrupt);
    return (void *)EXIT_SUCCESS;
}

void *cliente_conexion_memoria(void * arg_memoria){

	argumentos_thread * args = arg_memoria;
	t_paquete* send_handshake;
	char *valor = "conexion cpu";
	protocolo_socket op;
	int flag=1;
	t_list *paquete;
	int tid;
	int pid;
	do
	{
		conexion_cpu_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_cpu_memoria == -1);
	sem_post(sem_conexion_memoria);

	/*
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
			case INSTRUCCIONES: 
						log_info(logger, "Instrucción recibida de memoria");

						// Recibir el TID, PID y la instrucción
						t_list *paquete = recibir_paquete(conexion_cpu_memoria);
						tid = *(int *)list_remove(paquete, 0);
						pid = *(int *)list_remove(paquete, 0);
						char *instruccion = (char *)list_remove(paquete, 0);
						list_destroy_and_destroy_elements(paquete, free);

						// Decodificar y ejecutar la instrucción
						decode(&cpu, instruccion);

						// Enviar el contexto actualizado a memoria
						enviar_contexto_de_memoria(&cpu, pid);

						break;
			case TERMINATE:
				flag = 0;
				break;

			case THREAD_JOIN_OP:
					paquete = recibir_paquete(conexion_cpu_memoria);
					tid = *(int *)list_remove(paquete, 0);
					list_destroy_and_destroy_elements(paquete, free);
					agregar_interrupcion(THREAD_JOIN_OP, 2, tid);
					log_info(logger, "Se recibió interrupción THREAD_JOIN_OP");

					break;
				case IO_SYSCALL:
					paquete = recibir_paquete(conexion_cpu_memoria);
					tid = *(int *)list_remove(paquete, 0);
					list_destroy_and_destroy_elements(paquete, free);
					agregar_interrupcion(IO_SYSCALL, 2, tid);
					log_info(logger, "Se recibió syscall de tipo IO");
					break;
			default:
				break;
			}
		}

		eliminar_paquete(send_handshake);
		liberar_conexion(conexion_cpu_memoria);
		return (void *)EXIT_SUCCESS;
	}*/
}

