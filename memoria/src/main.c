#include <main.h>

t_log *logger;
int socket_cliente_cpu; //necesito que sea global para usarlo desde sistema.c
int conexion_memoria_fs;

t_memoria *memoria_usuario;
pthread_mutex_t * mutex_pcb;

pthread_mutex_t * mutex_part_fijas;
pthread_mutex_t * mutex_huecos;
pthread_mutex_t * mutex_procesos_din;
pthread_mutex_t * mutex_espacio;

pthread_mutex_t * mutex_conexion_kernel;
pthread_mutex_t * mutex_conexion_cpu;

pthread_mutex_t * mutex_lista_peticiones;


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

	mutex_pcb = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_pcb, NULL);
	mutex_part_fijas = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_part_fijas, NULL);
	mutex_huecos = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_huecos, NULL);
	mutex_procesos_din = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_procesos_din, NULL);
	mutex_espacio = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_espacio, NULL);

	mutex_conexion_kernel = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_conexion_kernel, NULL);

	mutex_conexion_cpu = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_conexion_cpu, NULL);

	mutex_lista_peticiones = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_lista_peticiones, NULL);

	//inicializar memoria
	fit fit;
	char * fit_text;
	particiones tipo_particion;
	char * tipo_particion_text;
	tipo_particion_text = config_get_string_value(config, "ESQUEMA");
	fit_text = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
	if(!strcmp(fit_text, "FIRST")){
		fit = FIRST_FIT;
	}else if(!strcmp(fit_text, "BEST")){
		fit = BEST_FIT;
	}else fit = WORST_FIT;
	if(!strcmp(tipo_particion_text,"FIJAS")){
		tipo_particion = FIJAS;
	}else if (!strcmp(tipo_particion_text, "DINAMICAS")){
		tipo_particion = DINAMICAS;
	}
	t_list *particiones = list_create();
	char ** particiones_string = config_get_array_value(config, "PARTICIONES");
	cargar_lista_particiones(particiones, particiones_string);
	string_array_destroy(particiones_string);
	inicializar_memoria(tipo_particion, config_get_int_value(config, "TAM_MEMORIA"), particiones, fit); //1 fija 0 dinamica

    //conexiones
	arg_cpu.puerto = config_get_string_value(config, "PUERTO_CPU");
    arg_kernel.puerto = config_get_string_value(config, "PUERTO_KERNEL");
    arg_fs.puerto = config_get_string_value(config, "PUERTO_FILESYSTEM");
    arg_fs.ip = config_get_string_value(config, "IP_FILESYSTEM");

    //conexiones
	pthread_create(&tid_cpu, NULL, conexion_cpu, (void *)&arg_cpu);
	pthread_create(&tid_kernel, NULL, server_multihilo_kernel, (void *)&arg_kernel);
	pthread_create(&tid_fs, NULL, cliente_conexion_filesystem, (void *)&arg_fs);
	//conexiones

	intptr_t ret_value_int = (intptr_t)ret_value;
    //espero fin conexiones
	pthread_join(tid_cpu, ret_value);
	log_info(logger,"conexion con cpu cerrada con status code: %ld", ret_value_int);
	pthread_join(tid_kernel, ret_value);
	log_info(logger,"conexion con server muiltihilo kernel cerrada con status code: %ld", ret_value_int);
	pthread_join(tid_fs, ret_value);
	log_info(logger,"conexion con filesystem cerrada con status code: %ld", ret_value_int);
	//espero fin conexiones

}

void inicializar_memoria(particiones tipo_particion, int size, t_list *particiones, fit fit){

	memoria_usuario = malloc(sizeof(t_memoria));
	memoria_usuario->lista_pcb = list_create();
	memoria_usuario->espacio=malloc(size);
	memoria_usuario->size = size;
	memoria_usuario->fit = fit;

	switch(tipo_particion){
		case DINAMICAS: // particiones dinamicas
			memoria_usuario->tipo_particion = DINAMICAS;
			init_tablas_dinamicas();
			break;
		
		case FIJAS: // fijas
			memoria_usuario->tipo_particion = FIJAS;
			memoria_usuario->tabla_particiones_fijas = list_create();
			inicializar_tabla_particion_fija(particiones);
			break;
	}
	log_info(logger, "Memoria inicializada");
}
void cargar_lista_particiones(t_list * particiones, char **particiones_array){
	
	char * elemento;
	int size = string_array_size(particiones_array);
	for (int i = 0; i < size; i++)
	{
		elemento = string_array_pop(particiones_array);

		list_add_in_index(particiones, -1, (void*)atoi(elemento));
	}
}

void *server_multihilo_kernel(void* arg_server){

	argumentos_thread * args = arg_server;
	pthread_t aux_thread;
	t_list *lista_t_peticiones = list_create();
	void* ret_value;
	protocolo_socket cod_op;
	int flag = 1; //1: operar, 0: TERMINATE
	pthread_t *thread_copia;

	int server = iniciar_servidor(args->puerto); //abro server
	log_info(logger, "Servidor listo para recibir nueva peticion");

	int socket_cliente_kernel;
	
	while (flag)
	{
		log_info(logger, "esperando nueva peticion de kernel");
		//pthread_mutex_lock(mutex_conexion_kernel);
		socket_cliente_kernel = esperar_cliente(server); //pausado hasta que llegue una peticion nueva (nuevo cliente)

		cod_op = recibir_operacion(socket_cliente_kernel);
		log_info(logger, "SE RECIBIO UNA PETICION DESDE KERNEL CON EL CODIGO DE OPERACION: %i", cod_op);
		switch (cod_op)
		{
			case PROCESS_CREATE_OP:
				
				pthread_create(&aux_thread, NULL, peticion_kernel_NEW_PROCESS, (void *)&socket_cliente_kernel);
				sleep(1);
				thread_copia = malloc(sizeof(pthread_t));
				*thread_copia = aux_thread;
				pthread_mutex_lock(mutex_lista_peticiones);
				list_add(lista_t_peticiones, thread_copia);
				pthread_mutex_unlock(mutex_lista_peticiones);
				log_info(logger, "Nueva petición de PROCESS CREATE");
				break;
			case THREAD_CREATE_OP:
				pthread_create(&aux_thread, NULL, peticion_kernel_NEW_THREAD, (void *)&socket_cliente_kernel);
				pthread_mutex_lock(mutex_lista_peticiones);
				thread_copia = malloc(sizeof(pthread_t));
				*thread_copia = aux_thread;
				list_add(lista_t_peticiones, thread_copia);
				pthread_mutex_unlock(mutex_lista_peticiones);
				log_info(logger, "Nueva petición de THREAD CREATE");
				break;
			case PROCESS_EXIT_OP:
				pthread_create(&aux_thread, NULL, peticion_kernel_END_PROCESS, (void *)&socket_cliente_kernel);
				pthread_mutex_lock(mutex_lista_peticiones);
				thread_copia = malloc(sizeof(pthread_t));
				*thread_copia = aux_thread;
				list_add(lista_t_peticiones, thread_copia);
				pthread_mutex_unlock(mutex_lista_peticiones);
				log_info(logger, "Nueva petición de PROCESS EXIT");
				break;
			case THREAD_EXIT_OP:
				pthread_create(&aux_thread, NULL, peticion_kernel_END_THREAD, (void *)&socket_cliente_kernel);
				pthread_mutex_lock(mutex_lista_peticiones);
				thread_copia = malloc(sizeof(pthread_t));
				*thread_copia = aux_thread;
				list_add(lista_t_peticiones, thread_copia);
				pthread_mutex_unlock(mutex_lista_peticiones);
				log_info(logger, "Nueva petición de THREAD EXIT");
				break;
			case DUMP_MEMORY_OP:
				pthread_create(&aux_thread, NULL, peticion_kernel_DUMP, (void *)&socket_cliente_kernel);
				pthread_mutex_lock(mutex_lista_peticiones);
				thread_copia = malloc(sizeof(pthread_t));
				*thread_copia = aux_thread;
				list_add(lista_t_peticiones, thread_copia);
				pthread_mutex_unlock(mutex_lista_peticiones);
				log_info(logger, "Nueva petición de DUMP MEMORY");
				break;
			case TERMINATE:
				log_error(logger, "TERMINATE recibido de KERNEL");
				pthread_mutex_unlock(mutex_conexion_kernel);
				flag=0;
				break;
			default:
				log_warning(logger,"Peticion invalida %d", cod_op);
				pthread_mutex_unlock(mutex_conexion_kernel);
				break;
		}
		
	}
	int size = list_size(lista_t_peticiones);
	for(int i=0;i<size;i++){ //en caso de que el while de arriba termine, espera a todas las peticiones antes de finalizar el server
		pthread_mutex_lock(mutex_lista_peticiones);
		if (list_size(lista_t_peticiones) == 0) {
			log_warning(logger, "Intento de eliminar de una lista vacía");
			break;
		}
		pthread_t *aux = list_remove(lista_t_peticiones, 0);
		pthread_join(*aux, ret_value);
		free(aux);
		log_info(logger, "peticion de kernel terminada con status code: %d", (int)ret_value);
		pthread_mutex_unlock(mutex_lista_peticiones);
	}
	pthread_mutex_lock(mutex_conexion_kernel);
	close(server);
	pthread_mutex_unlock(mutex_conexion_kernel);
	
	log_info(logger, "## Kernel Conectado - FD del socket: %d", socket_cliente_kernel);

    pthread_exit(EXIT_SUCCESS);

}

void *peticion_kernel_NEW_PROCESS(void* arg_peticion){
	int socket = *((int*)arg_peticion);
	t_pcb *pcb = malloc(sizeof(t_pcb));
	//atender peticion
	t_list * paquete_list;
	t_paquete * paquete_send;

	paquete_list = recibir_paquete(socket);

	pcb->pid = * (int *)list_remove(paquete_list, 0);
	pcb->memoria_necesaria = * (int *)list_remove(paquete_list, 0);
	pcb->limite = * (int *)list_remove(paquete_list, 0);

	if (crear_proceso(pcb)){
		log_error(logger, "No se pudo agregar el proceso a memoria por falta de espacio");
		paquete_send = crear_paquete(ERROR);
		enviar_paquete(paquete_send, socket);
		eliminar_paquete(paquete_send);
		list_destroy(paquete_list);
		return (void *)EXIT_FAILURE;
	}
	log_info(logger, "Se creo un nuevo proceso PID: %d", pcb->pid);
	
	//notificar resultado a kernel
	paquete_send = crear_paquete(OK);
	enviar_paquete(paquete_send, socket);
	pthread_mutex_unlock(mutex_conexion_kernel);

	eliminar_paquete(paquete_send);
	list_destroy(paquete_list);
	close(socket); //cerrar socket
	log_info(logger, "Finalizó el proccess create: %d", pcb->pid);
	return(void*)EXIT_SUCCESS; //finalizar hilo
}
void *peticion_kernel_NEW_THREAD(void* arg_peticion){
	int socket = *((int*)arg_peticion);
	t_tcb *tcb = malloc(sizeof(t_tcb));
	tcb->registro = malloc(sizeof(RegistroCPU));
	t_list * instrucciones = list_create();
	int pid;
	//atender peticion
	t_list * paquete_list;
	t_paquete * paquete_send;

	//pthread_mutex_lock(mutex_conexion_kernel);
	paquete_list = recibir_paquete(socket);
	tcb->tid = *(int *)list_remove(paquete_list, 0);
	tcb->pid = *(int *)list_remove(paquete_list, 0);
	tcb->prioridad = *(int *)list_remove(paquete_list, 0);
	tcb->estado = *(int *)list_remove(paquete_list, 0);
	tcb->quantum_restante = * (int *)list_remove(paquete_list, 0);

	t_list_iterator *iterator = list_iterator_create(paquete_list);
	char * aux_instruccion;
	while(list_iterator_has_next(iterator)){
		aux_instruccion = list_iterator_next(iterator);
		list_add(instrucciones, aux_instruccion);
	}
	tcb->instrucciones = list_create();
	tcb->instrucciones = instrucciones;
	tcb->registro->AX= 0;
    tcb->registro->BX= 0;
    tcb->registro->CX= 0;
    tcb->registro->DX= 0;
    tcb->registro->EX= 0;
    tcb->registro->FX= 0;
    tcb->registro->GX= 0;
    tcb->registro->HX= 0;
    tcb->registro->PC= 0;

	crear_thread(tcb);
	log_info(logger, "Se creo un nuevo thread TID: %d y PID: %d", tcb->tid, tcb->pid);
	
	//notificar resultado a kernel
	paquete_send = crear_paquete(OK);
	log_info(logger, "Se crea el paquete de respuesta de thread create");
	enviar_paquete(paquete_send, socket);
	pthread_mutex_unlock(mutex_conexion_kernel);

	eliminar_paquete(paquete_send);
	list_destroy(paquete_list);
	close(socket); //cerrar socket
	return(void*)EXIT_SUCCESS; //finalizar hilo
}
void *peticion_kernel_END_PROCESS(void* arg_peticion){
	int *socket = arg_peticion;
	int pid;
	//atender peticion
	t_list * paquete_list;
	t_paquete * paquete_send;
	//pthread_mutex_lock(mutex_conexion_kernel);

	paquete_list = recibir_paquete(*socket);
	pid = * (int *)list_remove(paquete_list, 0);

	fin_proceso(pid);
	log_info(logger, "Se finalizo el proceso PID: %d", pid);
	
	//notificar resultado a kernel
	paquete_send = crear_paquete(OK);
	enviar_paquete(paquete_send, *socket);
	pthread_mutex_unlock(mutex_conexion_kernel);

	eliminar_paquete(paquete_send);
	list_destroy(paquete_list);
	close(*socket); //cerrar socket
	return(void*)EXIT_SUCCESS; //finalizar hilo
}
void *peticion_kernel_END_THREAD(void* arg_peticion){
	int *socket = arg_peticion;
	int tid, pid;
	//atender peticion
	t_list * paquete_list;
	t_paquete * paquete_send;
	//pthread_mutex_lock(mutex_conexion_kernel);

	paquete_list = recibir_paquete(*socket);
	tid = *(int *)list_remove(paquete_list, 0);
	pid = *(int *)list_remove(paquete_list, 0);


	fin_thread(tid, pid);
	log_info(logger, "Se finalizo el Thread TID: %d", tid);
	
	//notificar resultado a kernel
	paquete_send = crear_paquete(OK);
	enviar_paquete(paquete_send, *socket);
	pthread_mutex_unlock(mutex_conexion_kernel);

	eliminar_paquete(paquete_send);
	list_destroy(paquete_list);
	close(*socket); //cerrar socket
	return(void*)EXIT_SUCCESS; //finalizar hilo
}

void *peticion_kernel_DUMP(void* arg_peticion){
	int *socket = arg_peticion;
	int tid;
	int pid;
	protocolo_socket respuesta;
	//atender peticion
	t_list * paquete_list;
	t_paquete * paquete_send;
	//pthread_mutex_lock(mutex_conexion_kernel);

	paquete_list = recibir_paquete(*socket);
	tid = * (int *)list_remove(paquete_list, 0);
	pid = * (int *)list_remove(paquete_list, 0);
		
	if(send_dump(pid, tid) == -1){
		respuesta = ERROR;
		log_info(logger, "Error Dump");
	}else{
		respuesta = OK;
		log_info(logger, "Dump Exitoso");
	}

	
	//notificar resultado a kernel
	paquete_send = crear_paquete(respuesta);
	enviar_paquete(paquete_send, *socket);
	pthread_mutex_unlock(mutex_conexion_kernel);

	eliminar_paquete(paquete_send);
	list_destroy(paquete_list);
	close(*socket); //cerrar socket
	return(void*)EXIT_SUCCESS; //finalizar hilo
}
void *conexion_cpu(void* arg_cpu)
{
	argumentos_thread *args = arg_cpu; 
	t_list *paquete_recv;
	
	t_config *config = config_create("config/memoria.config");
	int delay = config_get_int_value(config, "RETARDO_RESPUESTA");
	
	int server = iniciar_servidor(args->puerto);
	log_info(logger, "Servidor listo para recibir al cliente CPU");
	socket_cliente_cpu = esperar_cliente(server);
	log_info(logger, "Se realizó la conexion con CPU");

	uint32_t direccion;
	uint32_t valor;
	int PC;
	int pid, tid;
	t_paquete *recv;
	t_paquete *paquete_send;
	char *ok_respuesta = "OK";
	while(true){
		protocolo_socket cod_op = (protocolo_socket)recibir_operacion(socket_cliente_cpu);
		usleep(delay); // retardo en peticion / cpu
		switch (cod_op)
		{
			pthread_mutex_lock(mutex_conexion_cpu);
			case CONTEXTO_RECEIVE:
				// Recibo el paquete y extraigo pid y tid
				log_info(logger, "Solicitud de contexto recibida (context_receive)");
				paquete_recv = recibir_paquete(socket_cliente_cpu);

				// Extraer PID
				pid = *(int *)list_remove(paquete_recv, 0);

				// Extraer TID
				tid = *(int *)list_remove(paquete_recv, 0);
				enviar_contexto(pid, tid);

				break;
			case CONTEXTO_SEND:
				log_info(logger, "Solicitud de actualizacion de contexto recibida (context_send)");
				actualizar_contexto_ejecucion();
				break;
			case OBTENER_INSTRUCCION:
				paquete_recv = recibir_paquete(socket_cliente_cpu);
				PC = *(int *)list_remove(paquete_recv, 0);
				tid = *(int *)list_remove(paquete_recv, 0);
				pid = *(int *)list_remove(paquete_recv, 0);
				log_info(logger, "Solicitud de instruccion recibida PID: %d, TID: %d, PC: %d", pid, tid, PC);
				obtener_instruccion(PC, tid, pid);
				list_destroy(paquete_recv);
				break;
			case READ_MEM:
				paquete_recv = recibir_paquete(socket_cliente_cpu);
				direccion = *(int *)list_remove(paquete_recv, 0);
				pid = *(int *)list_remove(paquete_recv, 0);
				tid = *(int *)list_remove(paquete_recv, 0);
				valor = read_memory(direccion, pid, tid);
				log_info(logger, "Pedido de lectura, direccion: %d", direccion);
				if(valor != -1){
					paquete_send = crear_paquete(OK);
					agregar_a_paquete(paquete_send, &valor, sizeof(uint32_t));
					enviar_paquete(paquete_send, socket_cliente_cpu);
					log_info(logger, "Envio valor: %d", valor);
					eliminar_paquete(paquete_send);
				}else {
					paquete_send = crear_paquete(SEGMENTATION_FAULT);
					log_error(logger, "Error leyendo en memoria");
					enviar_paquete(paquete_send, socket_cliente_cpu);
					eliminar_paquete(paquete_send);
				}
				break;
			case WRITE_MEM:
				paquete_recv = recibir_paquete(socket_cliente_cpu);
				direccion = *(int *)list_remove(paquete_recv, 0);
				valor = *(int *)list_remove(paquete_recv, 0);
				pid = *(int *)list_remove(paquete_recv, 0);
				tid = *(int *)list_remove(paquete_recv, 0);
				log_info(logger, "Pedido de escritura, direccion: %d, valor: %d", direccion, valor);
				if(valor != -1){
					write_memory(direccion, valor, pid, tid);
					paquete_send = crear_paquete(OK);
					agregar_a_paquete(paquete_send, ok_respuesta, strlen(ok_respuesta)+1);
					enviar_paquete(paquete_send, socket_cliente_cpu);
					eliminar_paquete(paquete_send);

				}else {
					paquete_send = crear_paquete(ERROR_MEMORIA);
					log_error(logger, "Error escribiendo en memoria");
					enviar_paquete(paquete_send, socket_cliente_cpu);
					eliminar_paquete(paquete_send);
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
		pthread_mutex_unlock(mutex_conexion_cpu);
	}
		
	close(server);
	close(socket_cliente_cpu);
    pthread_exit(EXIT_SUCCESS);
}
void *cliente_conexion_filesystem(void * arg_fs){

	argumentos_thread * args = arg_fs;
	protocolo_socket op;
	do
	{
		conexion_memoria_fs = crear_conexion(args->ip, args->puerto);
		sleep(1);

	}while(conexion_memoria_fs == -1);
	log_info(logger, "Filesystem");

	return (void *)EXIT_SUCCESS;
}