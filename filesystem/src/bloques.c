#include "bloques.h"
extern t_config* config;
extern t_log* logger;
extern int retardo_acceso;
static FILE* metadata_file;
static FILE* bloques_file;
int index_block=0;
extern uint32_t block_count;
extern int block_size;
extern int retardo_acceso;
extern char* mount_dir;
int bloques_libres;
extern uint32_t num_bloque;

extern char* nombre_archivo;
extern uint32_t tamanio;
extern uint32_t *datos;
extern char * ruta_files;
extern uint32_t index_libre;
extern int libres;

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
    bloques_file = fopen(path_bloques, "rb+");
    if (!bloques_file) {
        bloques_file = fopen(path_bloques, "wb+");
        if (!bloques_file) {
            log_error(logger, "Error al crear el archivo bloques.dat");
            exit(EXIT_FAILURE);
        }

        ftruncate(fileno(bloques_file), block_size * block_count);
    }

    bloques = mmap(NULL, block_size * block_count, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(bloques_file), 0);
    if (bloques == MAP_FAILED) {
        log_error(logger, "Error al mapear bloques.dat en memoria");
        exit(EXIT_FAILURE);
    }
    free(path_bloques);
    log_info(logger, "Bloques inicializado correctamente.");

    
}

void escribir_bloque(int bloque, void *contenido, size_t tamanio) {
    if (bloque < 0 || bloque >= block_count) {
        log_error(logger, "El índice de bloque %d está fuera del rango permitido.", bloque);
        return;
    }
    usleep(retardo_acceso * 1000);
    fseek(bloques_file, (bloque * block_size), SEEK_CUR);
    fwrite(contenido, tamanio, 1, bloques_file);
    log_info(logger, "## Bloque asignado: %d con %lu bytes", bloque, libres);
}

void *leer_bloque(int bloque) {
    int block_count = config_get_int_value(config, "BLOCK_COUNT");
    if (bloque < 0 || bloque >= block_count) {
        log_error(logger, "El índice de bloque %d está fuera del rango permitido (0 - %d).", bloque, block_count - 1);
        return NULL;
    }
    usleep(retardo_acceso * 1000);
    void *direccion_bloque = bloques + (bloque * block_size);
    log_info(logger, "## Bloque %d leído correctamente. Dirección: %p, Tamaño: %d bytes", bloque, direccion_bloque, block_size);

    return direccion_bloque;
}


int crear_archivo_metadata(char* nombre_archivo,uint32_t tamanio) {
    log_info(logger, "## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d", num_bloque, nombre_archivo, bloques_libres);
    if (block_count == 0) {
        log_error(logger, "No se encontró el valor BLOCK_COUNT en el archivo de configuración.");
        return -1;
    }

    size_t path_length = strlen(ruta_files) + strlen(nombre_archivo) + 2;
    char *path_metadata = malloc(path_length);
    if (path_metadata == NULL) {
        log_error(logger, "Error: No se pudo asignar memoria para path_metadata.\n");
        exit(EXIT_FAILURE);
    }
    snprintf(path_metadata, path_length, "%s/%s", ruta_files, nombre_archivo);
    if (!metadata_file) {
        metadata_file = fopen(path_metadata, "wb+");
        if (!metadata_file) {
            log_error(logger, "Error al crear el archivo metadata");
            free(path_metadata);
            exit(EXIT_FAILURE);
        }else log_info(logger, "Archivo Creado: %s - Tamaño: %d", nombre_archivo, tamanio);
        index_block++;
        size_t buffer_size = snprintf(NULL, 0, "SIZE=%u\nINDEX_BLOCK=%d\n", block_size, index_block) + 1;
        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            log_error(logger, "Error al asignar memoria para el buffer.");
            fclose(metadata_file);
            free(path_metadata);
            exit(EXIT_FAILURE);
        }
        snprintf(buffer, buffer_size, "SIZE=%d\nINDEX_BLOCK=%d\n", block_size, index_block);
        if (fwrite(buffer, sizeof(char), strlen(buffer), metadata_file) != strlen(buffer)) {
            log_error(logger, "Error al escribir en el archivo metadata.");
            fclose(metadata_file);
            free(path_metadata);
            exit(EXIT_FAILURE);
        }

        fclose(metadata_file);
        log_info(logger, "Archivo metadata creado con éxito: %s", path_metadata);
    } else {
        log_info(logger, "Archivo metadata ya existe: %s", path_metadata);
        fclose(metadata_file);
    }
    log_info(logger, "Archivo Creado: %s - Tamaño: %d", nombre_archivo, tamanio);
    free(path_metadata);

    return 0;
}

int crear_archivo_dump(char* nombre_archivo, uint32_t tamanio, void* datos) {
    // Verificar si hay espacio suficiente en el bitmap
    uint32_t num_bloques_necesarios = (uint32_t)ceil((double)tamanio / block_size); // Redondear hacia arriba
    if (!espacio_disponible(num_bloques_necesarios + 1)) { // 1 adicional para el bloque de índices
        log_error(logger, "No hay suficiente espacio en el bitmap para crear el archivo.");
        return -1;
    }  
    // Reservar el bloque de índices
    uint32_t bloque_indice = reservar_bloques(num_bloques_necesarios+1);
    if (bloque_indice == -1) {
        log_error(logger, "Error al reservar el bloque de índices.");
        return -1;
    }

    // Reservar los bloques de datos
    //agarrar los datos, ponerlos en los bloques arrancando desde index_libre +1
    
    uint32_t indice_datos[num_bloques_necesarios];
    for (int i = 0; i < num_bloques_necesarios; i++){
        indice_datos[i] = (i+bloque_indice)*block_size;
    }
    escribir_bloque(index_libre, indice_datos, block_size);
    escribir_bloque(index_libre+1, datos, tamanio*block_size);

    // Crear el archivo de metadata
    int resultado_metadata = crear_archivo_metadata(nombre_archivo, tamanio);
    if (resultado_metadata != 0) {
        log_error(logger, "Error al crear el archivo de metadata.");
        // Liberar los bloques reservados
        liberar_bloque(bloque_indice);
        for (uint32_t i = bloque_indice; i < num_bloques_necesarios; i++) {
            liberar_bloque(i);
        }
        return -1;
    }

    // // Escribir referencias a los bloques de datos en el bloque de índices
    // FILE* fs = fopen("filesystem.dat", "rb+"); // Abrir el sistema de archivos
    // if (fs == NULL) {
    //     log_error(logger, "Error al abrir el archivo del filesystem.");
    //     liberar_bloque(bloque_indice);
    //     for (uint32_t i = 0; i < num_bloques_necesarios; i++) {
    //         liberar_bloque(bloques_datos[i]);
    //     }
    //     return -1;
    // }

    // fseek(fs, bloque_indice * block_size, SEEK_SET); // Posicionar en el bloque de índices
    // if (fwrite(bloques_datos, sizeof(uint32_t), num_bloques_necesarios, fs) != num_bloques_necesarios) {
    //     log_error(logger, "Error al escribir en el bloque de índices.");
    //     fclose(fs);
    //     liberar_bloque(bloque_indice);
    //     for (uint32_t i = 0; i < num_bloques_necesarios; i++) {
    //         liberar_bloque(bloques_datos[i]);
    //     }
    //     return -1;
    // }

    // // Escribir los datos en los bloques de datos
    // for (uint32_t i = 0; i < num_bloques_necesarios; i++) {
    //     fseek(fs, bloques_datos[i] * block_size, SEEK_SET); // Posicionar en el bloque de datos

    //     size_t bytes_a_escribir = (tamanio > block_size) ? block_size : tamanio; // Bytes a escribir en este bloque
    //     if (fwrite(datos, 1, bytes_a_escribir, fs) != bytes_a_escribir) {
    //         log_error(logger, "Error al escribir datos en el bloque %u.", bloques_datos[i]);
    //         fclose(fs);
    //         liberar_bloque(bloque_indice);
    //         for (uint32_t j = 0; j < num_bloques_necesarios; j++) {
    //             liberar_bloque(bloques_datos[j]);
    //         }
    //         return -1;
    //     }

    //     datos += bytes_a_escribir / sizeof(uint32_t); // Avanzar el puntero de datos
    //     tamanio -= bytes_a_escribir; // Reducir los bytes restantes
    // }

    log_info(logger, "Archivo dump creado exitosamente: %s", nombre_archivo);
    return 0;
}
