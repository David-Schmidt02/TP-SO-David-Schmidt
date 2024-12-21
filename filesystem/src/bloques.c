#include "bloques.h"
extern t_config* config;
extern t_log* logger;
extern int retardo_acceso;

extern uint32_t block_count;
extern int block_size;
extern int retardo_acceso;
extern char* mount_dir;
int bloques_libres;

extern char* nombre_archivo;
extern uint32_t tamanio;
extern uint32_t *datos;
extern char * ruta_files;
extern uint32_t bits_ocupados;
extern int libres;

extern pthread_mutex_t *mutex_logs;

void inicializar_bloques() {
    
    if (mount_dir == NULL) {
        log_info(logger, "Error: No se encontró 'MOUNT_DIR' en la configuración.");
        return exit(EXIT_FAILURE);
    }

    size_t path_length = strlen(mount_dir) + strlen("/bloques.dat") + 1;
    char *path_bloques = malloc(path_length);
    if (path_bloques == NULL) {
        log_info(logger, "Error: No se pudo asignar memoria para path_bloques.");
        exit(EXIT_FAILURE);
    }
    

    // Construir la ruta completa
    snprintf(path_bloques, path_length, "%s/bloques.dat", mount_dir);
    log_info(logger, "path bloques:%s",path_bloques);
    FILE* bloques_file = fopen(path_bloques, "rb+");
    if (!bloques_file) {
        bloques_file = fopen(path_bloques, "wb+");
        if (!bloques_file) {
            log_info(logger, "Error al crear el archivo bloques.dat");
            exit(EXIT_FAILURE);
        }

        ftruncate(fileno(bloques_file), block_size * block_count);
    }

    bloques = mmap(NULL, block_size * block_count, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(bloques_file), 0);
    if (bloques == MAP_FAILED) {
        log_info(logger, "Error al mapear bloques.dat en memoria");
        exit(EXIT_FAILURE);
    }
    free(path_bloques);
    fclose(bloques_file);
    log_info(logger, "Bloques inicializado correctamente.");

    
}


int crear_archivo_metadata(char* nombre_archivo,uint32_t tamanio, int indice_bloque, int cant_bloque) {

    if (block_count == 0) {
        log_info(logger, "No se encontró el valor BLOCK_COUNT en el archivo de configuración.");
        return -1;
    }
    FILE* metadata_file = NULL;
    size_t path_length = strlen(ruta_files) + strlen(nombre_archivo) + 2;
    char *path_metadata = malloc(path_length);
    if (path_metadata == NULL) {
        log_info(logger, "Error: No se pudo asignar memoria para path_metadata.\n");
        exit(EXIT_FAILURE);
    }
    snprintf(path_metadata, path_length, "%s/%s", ruta_files, nombre_archivo);
    if (!metadata_file) {
        metadata_file = fopen(path_metadata, "wb+");
        if (!metadata_file) {
            log_info(logger, "Error al crear el archivo metadata");
            free(path_metadata);
            exit(EXIT_FAILURE);
        }

        size_t buffer_size = snprintf(NULL, 0, "SIZE=%u\nINDEX_BLOCK=%d\n", cant_bloque, indice_bloque) + 1;
        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            log_info(logger, "Error al asignar memoria para el buffer.");
            fclose(metadata_file);
            free(path_metadata);
            exit(EXIT_FAILURE);
        }
        snprintf(buffer, buffer_size, "SIZE=%d\nINDEX_BLOCK=%d\n", cant_bloque, indice_bloque);
        if (fwrite(buffer, sizeof(char), strlen(buffer), metadata_file) != strlen(buffer)) {
            log_info(logger, "Error al escribir en el archivo metadata.");
            fclose(metadata_file);
            free(path_metadata);
            exit(EXIT_FAILURE);
        }

        fclose(metadata_file);
        log_info(logger, "Archivo metadata creado con éxito: %s", path_metadata);
    } else {
        log_info(logger, "Archivo metadata ya existe: %s", path_metadata);
        return 0; 
    }
    free(path_metadata);
    return 0;
}

int crear_archivo_dump(char* nombre_archivo, uint32_t tamanio, void* datos) {
    
    // Verificar si hay espacio suficiente en el bitmap
    uint32_t num_bloques_necesarios = (uint32_t)ceil((double)tamanio / block_size); // Redondear hacia arriba
    if (!espacio_disponible(num_bloques_necesarios + 1)) { // 1 adicional para el bloque de índices
        log_info(logger, "No hay suficiente espacio en el bitmap para crear el archivo.");
        return -1;
    }  
    
    // Reservar el bloque + bloque_indice
    t_reserva_bloques *reserva = reservar_bloques(num_bloques_necesarios+1);
    
    if (reserva == NULL) {
        log_info(logger, "Error al reservar el bloque completo");
        return -1;
    }
    
    int check;
    int indice_bloque = list_get(reserva->lista_indices,0); 
    int cant_bloque = list_size(reserva->lista_indices);

    check = cargar_bloques(list_size(reserva->lista_indices), datos, reserva->lista_indices, tamanio);
    if (check){
        log_info(logger,"no cargo los bloques");
        return -1;
    } 
    log_info(logger,"cargo los bloques");
    crear_archivo_metadata(nombre_archivo, tamanio, indice_bloque, cant_bloque);
    return 0;
}

int cargar_bloques(uint32_t cantidad_bloques, void *datos, t_list* lista_indices, int tamanio){
    size_t path_length = strlen(mount_dir) + strlen("/bloques.dat") + 1;
    char* path_bloques = malloc(path_length);
    strcpy(path_bloques,"");
    strcat(path_bloques, mount_dir);
    strcat(path_bloques, "/bloques.dat");
    FILE* bloques_file = fopen(path_bloques, "rb+");
    if (bloques_file == NULL) {
        log_info(logger, "Error al abrir el archivo de bloques");
    }
    int block_sizeAUX;
    uint32_t dir_bloque[block_size/4];
    size_t posicion = list_remove(lista_indices,0);
    posicion *= block_size;
    int index=0;
    for(int i = 0; i<block_size/4;i++)
        dir_bloque[i] = 0;
    t_list_iterator* iterador = list_iterator_create(lista_indices);
    while (list_iterator_has_next(iterador))
    {
        dir_bloque[index] = list_iterator_next(iterador);
        dir_bloque[index]*=32;
        index++;   
    }
    
    for (uint32_t i = 0; i < cantidad_bloques; i++) {
        if (i==0){
            fseek(bloques_file,posicion,SEEK_SET);
            fwrite(dir_bloque, block_size, 1, bloques_file);
            continue;
        }
        if (i == cantidad_bloques-1)
            block_sizeAUX=(tamanio%block_size);
            if(block_sizeAUX == 0)
                block_sizeAUX=block_size;
        else
            block_sizeAUX=block_size;
        
        posicion = (int)list_get(lista_indices,i-1)*block_size;
        if(fseek(bloques_file,posicion,SEEK_SET) != 0){
            log_info(logger, "error al mover el puntero: %d", list_get(lista_indices,i-1));
            return -1;
        }
        void *  fragmento_datos = datos + ((i-1)*block_size);
        pthread_mutex_lock(mutex_logs);
        if (fwrite(fragmento_datos, block_sizeAUX, 1, bloques_file) != 1) {
            log_info(logger, "Error al escribir en el bloque de punteros");
            pthread_mutex_unlock(mutex_logs);
            return -1;
        }
        pthread_mutex_unlock(mutex_logs);
    }

    fclose(bloques_file);
    return 0;
}
