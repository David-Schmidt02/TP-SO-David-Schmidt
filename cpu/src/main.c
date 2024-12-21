#include <main.h>

t_log *logger;

int socket_conexion_memoria;
int socket_conexion_kernel_interrupt;
int socket_conexion_kernel_dispatch;


sem_t * sem_estado_hilo_ejecutar;
//Variables globales
uint32_t base_actual;
uint32_t limite_actual;
RegistroCPU *cpu_actual;
int pid_actual;
int tid_actual;
char * instruccion_actual;
bool flag_hay_interrupcion;
t_interrupcion* interrupcion_actual;
t_instruccion_partida * instruccion_actual_partida;
//

int tid_de_interrupcion_FIN_QUANTUM;

//Lista de interrupciones
t_list* lista_interrupciones;
pthread_mutex_t *mutex_lista_interrupciones;
//

sem_t * sem_registros_actualizados;
//No se usa

//

//Mutexs para proteger las conexiones
pthread_mutex_t *mutex_kernel_interrupt;
pthread_mutex_t *mutex_kernel_dispatch;
pthread_mutex_t *mutex_conexion_memoria;
//

//Semáforos para esperar a que las conexiones se hagan
sem_t * sem_conexion_kernel_interrupt;
sem_t * sem_conexion_kernel_cpu_dispatch;
sem_t * sem_conexion_memoria;
//

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

	sem_estado_hilo_ejecutar = malloc(sizeof(sem_t));
    sem_init(sem_estado_hilo_ejecutar, 0, 0);


    //conexiones
	pthread_create(&tid_kernelI, NULL, conexion_kernel_interrupt, (void *)&arg_kernelI);
	pthread_create(&tid_kernelD, NULL, conexion_kernel_dispatch, (void *)&arg_kernelD);
	pthread_create(&tid_memoria, NULL, cliente_conexion_memoria, (void *)&arg_memoria);

	sem_wait(sem_conexion_kernel_cpu_dispatch);
	sem_wait(sem_conexion_kernel_interrupt);
	sem_wait(sem_conexion_memoria);

	//ciclo de instruccion
	log_info(logger,"Creacion del hilo para el ciclo de instrucción\n");
    pthread_t hilo_ciclo_instruccion;
    pthread_create(&hilo_ciclo_instruccion, NULL, (void *)ciclo_instruccion, NULL);

	pthread_join(tid_memoria, ret_value);
	pthread_join(tid_kernelI, ret_value);
	pthread_join(tid_kernelD, ret_value);
    pthread_join(hilo_ciclo_instruccion, ret_value);

	//espero fin conexiones
}

void *ciclo_instruccion(void* args)
{
	while ((1))
	{
		sem_wait(sem_estado_hilo_ejecutar);
		obtener_contexto_de_memoria();
		while (!flag_hay_interrupcion)
		{
			fetch();
			instruccion_actual_partida = decode(instruccion_actual);
			execute(instruccion_actual_partida);
		}
		checkInterrupt();
		enviar_contexto_de_memoria();
		devolver_motivo_a_kernel(interrupcion_actual->tipo, interrupcion_actual->parametro);
		inicializar_registros_cpu();
	}
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
                sem_wait(sem_registros_actualizados);
				log_info(logger, "Recibí un hilo para ejecutar de parte de Kernel");
				t_list *paquete = recibir_paquete(socket_conexion_kernel_dispatch);
				tid_actual = *(int *)list_remove(paquete, 0);
				pid_actual = *(int *)list_remove(paquete, 0);
				log_error(logger, "El hilo a ejecutar es PID: %d TID:%d", pid_actual, tid_actual);
				list_destroy(paquete);
				flag_hay_interrupcion = false;
				sem_post(sem_estado_hilo_ejecutar);
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
                if(list_size(lista_interrupciones)>1)
                    log_info(logger, "Se descartó interrupción FIN_QUANTUM");
                else{
				if(tid_actual == tid_de_interrupcion_FIN_QUANTUM){
					pthread_mutex_lock(mutex_lista_interrupciones);
					encolar_interrupcion(FIN_QUANTUM, 3, texto);
					pthread_mutex_unlock(mutex_lista_interrupciones);
					log_info(logger, "Se recibió interrupción FIN_QUANTUM");
				}
                }
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
	inicializar_semaforos_cpu();
	inicializar_registros_cpu();
}

