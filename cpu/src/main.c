#include <main.h>

t_log *logger;
RegistroCPU *cpu_actual;
int socket_conexion_memoria;
sem_t * sem_conexion_memoria;

int socket_conexion_kernel_interrupt;
int socket_conexion_kernel_dispatch;

uint32_t base_actual,limite_actual;

sem_t * sem_conexion_kernel_interrupt;
sem_t * sem_conexion_kernel_cpu_dispatch;

int pid_actual;
int tid_actual;
int tid_de_interrupcion_FIN_QUANTUM;

int flag_hay_contexto;

bool flag = true;



//hay que inicializar
t_list* lista_interrupciones;
sem_t * sem_lista_interrupciones;
sem_t * sem_hay_contexto;

pthread_mutex_t * mutex_fetch;

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
	inicializar_estructuras_cpu();

    //conexiones
	pthread_create(&tid_kernelI, NULL, conexion_kernel_interrupt, (void *)&arg_kernelI);
	pthread_create(&tid_kernelD, NULL, conexion_kernel_dispatch, (void *)&arg_kernelD);
	pthread_create(&tid_memoria, NULL, cliente_conexion_memoria, (void *)&arg_memoria);

	int i=0;
	sem_wait(sem_conexion_kernel_cpu_dispatch);
	sem_wait(sem_conexion_kernel_interrupt);
	sem_wait(sem_conexion_memoria);

	while(flag){
		log_info(logger, "EL PID ACTUAL ES IGUAL A: %d", pid_actual);
		if(pid_actual == 0){
			sem_wait(sem_lista_interrupciones);
			checkInterrupt();
		}
		else 
		{
			log_info(logger, "SE BUSCA LA SIGUIENTE INSTRUCCION EN EL FECTH");
			fetch();
		}
		log_info(logger, "Nro vuelta: %d",i);
		i++;
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
	t_list *paquete;
	int tid;

	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor de Dispatch listo para recibir al cliente Kernel");
	
	pthread_mutex_lock(mutex_kernel_dispatch);
	socket_conexion_kernel_dispatch = esperar_cliente(server);
	pthread_mutex_unlock(mutex_kernel_dispatch);

	log_info(logger, "Se realizó la conexion Kernel - Cpu Dispatch");
	sem_post(sem_conexion_kernel_cpu_dispatch);	
	while(true){
		pthread_mutex_lock(mutex_kernel_dispatch);
		int cod_op = recibir_operacion(socket_conexion_kernel_dispatch);
		switch (cod_op){
			case INFO_HILO:
				log_info(logger, "Recibí un hilo para ejecutar de parte de Kernel");
				t_list *paquete = recibir_paquete(socket_conexion_kernel_dispatch);
				tid_actual = *(int *)list_remove(paquete, 0);
				pid_actual = *(int *)list_remove(paquete, 0);
				list_destroy(paquete);
				char* texto[3];
				texto[0] = malloc(5);
				texto[1] = malloc(5);
				texto[2] = malloc(5);
				strcpy(texto[1],string_itoa(tid_actual)); 
				strcpy(texto[2],string_itoa(pid_actual)); 
				pthread_mutex_lock(mutex_lista_interrupciones);
				log_info(logger, "Encolo la interrupcion del hilo en la lista de interrupciones");
				encolar_interrupcion(INFO_HILO,3,texto);
				pthread_mutex_unlock(mutex_lista_interrupciones);
				break;
			case -1:
				log_error(logger, "el cliente se desconecto. Terminando servidor");
				pthread_mutex_unlock(mutex_kernel_dispatch);
				return (void *)EXIT_FAILURE;
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		pthread_mutex_unlock(mutex_kernel_dispatch);
		}
		
	close(socket_conexion_kernel_dispatch);
    return (void *)EXIT_SUCCESS;
}

void *conexion_kernel_interrupt(void* arg_kernelI)
{
	argumentos_thread * args = arg_kernelI; 
	t_list *paquete;
	int tid;

	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor de Interrupt listo para recibir al cliente Kernel");
	
	pthread_mutex_lock(mutex_kernel_interrupt);
	socket_conexion_kernel_interrupt = esperar_cliente(server);
	pthread_mutex_unlock(mutex_kernel_interrupt);

	log_info(logger, "Se realizó la conexion Kernel - Cpu Interrupt");
	sem_post(sem_conexion_kernel_interrupt);
	while(true){
		int cod_op = recibir_operacion(socket_conexion_kernel_interrupt);
		switch (cod_op)
		{
			case FIN_QUANTUM:
				paquete = recibir_paquete(socket_conexion_kernel_interrupt);
				tid_de_interrupcion_FIN_QUANTUM = *(int *)list_remove(paquete, 0);
				list_destroy(paquete);
				char* texto[1];
				pthread_mutex_lock(mutex_lista_interrupciones);
				encolar_interrupcion(FIN_QUANTUM, 3, texto);
				pthread_mutex_unlock(mutex_lista_interrupciones);
				log_info(logger, "Se recibió interrupción FIN_QUANTUM");
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
	close(socket_conexion_kernel_interrupt);
    return (void *)EXIT_SUCCESS;
}

void *cliente_conexion_memoria(void * arg_memoria){

	argumentos_thread * args = arg_memoria;
	do
	{
		socket_conexion_memoria = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(socket_conexion_memoria == -1);
	sem_post(sem_conexion_memoria);
}

void inicializar_estructuras_cpu(){
	lista_interrupciones = list_create();
	inicializar_registros_cpu();
	inicializar_semaforos_cpu();

}
