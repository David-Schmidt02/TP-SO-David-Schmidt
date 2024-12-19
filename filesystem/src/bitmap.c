#include "bitmap.h"

static t_bitarray* bitmap;
static FILE* bitmap_file;
extern uint32_t block_count;
extern int block_size;
extern int retardo_acceso;
extern char* mount_dir;
extern t_config *config;
extern t_log *logger;
extern pthread_mutex_t *mutex_logs;
int libres;
uint32_t bits_ocupados;

void inicializar_libres() {
    libres = 0;
    int i = 0;
    for (i; i < bitarray_get_max_bit(bitmap); i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            libres++;
        }
    }
    bits_ocupados = (i+1) - libres;
}

void inicializar_bitmap() {
    if (block_count == 0) {
        log_error(logger, "No se encontró el valor BLOCK_COUNT en el archivo de configuración.");
        exit(EXIT_FAILURE);
    }

    size_t tamanio_bitmap = (size_t)ceil((double)block_count/8);

    size_t path_length = strlen(mount_dir) + strlen("/bitmap.dat") + 1;
    char *path_bitmap = malloc(path_length);
    if (!path_bitmap) {
        log_error(logger, "Error: No se pudo asignar memoria para path_bitmap.");
        exit(EXIT_FAILURE);
    }
    snprintf(path_bitmap, path_length, "%s/bitmap.dat", mount_dir);
    log_info(logger, "Ruta del bitmap: %s", path_bitmap);


    bitmap_file = fopen(path_bitmap, "rb+");
    if (!bitmap_file) {
        bitmap_file = fopen(path_bitmap, "wb+");
        if (!bitmap_file) {
            log_error(logger, "Error al crear el archivo bitmap.dat.");
            free(path_bitmap);
            exit(EXIT_FAILURE);
        }

        uint8_t* buffer = calloc(tamanio_bitmap, sizeof(uint8_t));
        if (!buffer) {
            log_error(logger, "Error al asignar memoria para el buffer inicial.");
            fclose(bitmap_file);
            free(path_bitmap);
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(mutex_logs);
        if (fwrite(buffer, sizeof(uint8_t), tamanio_bitmap, bitmap_file) != tamanio_bitmap) {
            log_error(logger, "Error al escribir en el archivo bitmap.dat.");
            free(buffer);
            fclose(bitmap_file);
            free(path_bitmap);
            pthread_mutex_unlock(mutex_logs);
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(mutex_logs);
        fflush(bitmap_file);
        free(buffer);
    }

    uint8_t* contenido_bitmap = malloc(tamanio_bitmap);
    if (!contenido_bitmap) {
        log_error(logger, "Error al asignar memoria para contenido_bitmap.");
        fclose(bitmap_file);
        free(path_bitmap);
        exit(EXIT_FAILURE);
    }
    rewind(bitmap_file);
    if (fread(contenido_bitmap, sizeof(uint8_t), tamanio_bitmap, bitmap_file) != tamanio_bitmap) {
        log_error(logger, "Error al leer el archivo bitmap.dat.");
        free(contenido_bitmap);
        fclose(bitmap_file);
        free(path_bitmap);
        exit(EXIT_FAILURE);
    }


    bitmap = bitarray_create_with_mode((char*)contenido_bitmap, tamanio_bitmap, LSB_FIRST);
    if (!bitmap) {
        log_error(logger, "Error al inicializar el bitmap.");
        free(contenido_bitmap);
        fclose(bitmap_file);
        free(path_bitmap);
        exit(EXIT_FAILURE);
    }

    

    log_info(logger, "Bitmap inicializado correctamente.");
    free(path_bitmap);
}

bool espacio_disponible(uint32_t cantidad) {
    if(libres >= cantidad){
        return true;
    }
    return false;
}

t_reserva_bloques* reservar_bloques(uint32_t size) {

    
    if (size == 0) {
        log_error(logger, "Error: El tamaño solicitado debe ser mayor que 0.");
        return NULL;
    }

    t_reserva_bloques* reserva = malloc(sizeof(t_reserva_bloques));
    if (reserva == NULL) {
        log_error(logger, "Error al asignar memoria para la reserva.");
        free(reserva);

        return NULL;
    }

    reserva->bloques_datos = calloc(size-1, sizeof(uint32_t)); // bloques_datos es un array de tamano size inicializado en 0.
    if (reserva->bloques_datos == NULL) {
        log_error(logger, "Error al asignar memoria para los bloques de datos.");
        free(reserva);

        return NULL;
    }
    reserva->cantidad_bloques = size;
    
    if (!espacio_disponible(size)) { // YA SE LE AGREGA +1 EN LA ASIGNACION
        log_error(logger, "No hay suficiente espacio en el bitmap.");
        free(reserva->bloques_datos);
        free(reserva);

        return NULL;
    }

    // Reservar el bloque índice
    reserva->bloque_indice = -1; // un check, si se mantiene asi es un error
    reserva->lista_indices =list_create();
    for (uint32_t i = 0; i < block_count; i++) {
        if (!bitarray_test_bit(bitmap, i)) { // Bloque libre encontrado
            bitarray_set_bit(bitmap, i);
            list_add(reserva->lista_indices,i);  
            break;
        }
    }

    if (!list_size(reserva->lista_indices)) {
        log_error(logger, "Error: No se pudo reservar el bloque índice.");
        free(reserva->bloques_datos);
        free(reserva);

        return NULL;
    }

    log_info(logger, "Bloque índice reservado: %u", reserva->bloque_indice);

    // Reservar los bloques de datos
    uint32_t i = 0; 
    uint32_t bloques_reservados = 0;

    for (i; bloques_reservados < size-1 && i < block_count ; i++) {
        if (!bitarray_test_bit(bitmap, i)) { 
            bitarray_set_bit(bitmap, i);
            reserva->bloques_datos[bloques_reservados] = i;
            list_add(reserva->lista_indices, i);
            bloques_reservados++;
        }// sale porque ya se reservaron todos los bloques o porque recorrio todo 
    }
 
    if(list_size(reserva->lista_indices) > (block_size/4)-1){
        log_error(logger, "Error: No se pudieron reservar todos los bloques de datos.");
            // Liberar los bloques ya asignados
            for (uint32_t j = 0; j <= i; j++) {
                bitarray_clean_bit(bitmap, reserva->bloques_datos[j]); // se recorre el array, liberando todos los bloques
            }
            bitarray_clean_bit(bitmap, list_get(reserva->lista_indices,0));// lo pone en 0

            free(reserva->bloques_datos);
            free(reserva);
            list_destroy(reserva->lista_indices);
            
            return NULL;
    }

    // Verificar si todos los bloques de datos fueron reservados
    for (uint32_t i = 0; i < size-1; i++) {

        if (reserva->bloques_datos[i] == 0) { // como estan inicializados, si alguna pos siguie valiendo 0 es porque 
                                              // se recorrio todo el block_count y no se encontro un espacio para asignarle un bit
            log_error(logger, "Error: No se pudieron reservar todos los bloques de datos.");
            // Liberar los bloques ya asignados

            for (uint32_t j = 0; j <= i; j++) {
                bitarray_clean_bit(bitmap, reserva->bloques_datos[j]); // se recorre el array, liberando todos los bloques
            }
            bitarray_clean_bit(bitmap, list_get(reserva->lista_indices,0));// lo pone en 0


            free(reserva->bloques_datos);
            free(reserva);
            list_destroy(reserva->lista_indices);
            return NULL;
        }
    }
    if (cargar_bitmap() != 0) {
        log_error(logger, "Error al sincronizar el bitmap con el archivo.");

        return NULL;
    }

    log_info(logger, "Reserva exitosa: Bloque índice %u, %u bloques de datos asignados.",
             reserva->bloque_indice, size);

    return reserva;
}    



int cargar_bitmap() {
    size_t path_length = strlen(mount_dir) + strlen("/bitmap.dat") + 1;
    char* path_bitmap = malloc(path_length);
    strcpy(path_bitmap,"");
    strcat(path_bitmap, mount_dir);
    strcat(path_bitmap, "/bitmap.dat");
    FILE* bitmap_file = fopen(path_bitmap, "rb+");
    if (bitmap_file == NULL) {
        log_error(logger, "Error al abrir el archivo bitmap.dat para escritura.");
        return -1;
    }

    size_t bytes_bitmap = block_count / 8; // Tamaño en bytes
    pthread_mutex_lock(mutex_logs);
    if (fwrite(bitmap->bitarray, bytes_bitmap, 1, bitmap_file) != 1) {
        log_error(logger, "Error al escribir el bitmap en bitmap.dat.");
        fclose(bitmap_file);
        pthread_mutex_unlock(mutex_logs);
        return -1;
    }
    pthread_mutex_unlock(mutex_logs);
    fclose(bitmap_file);
    log_info(logger, "Bitmap actualizado exitosamente en bitmap.dat.");
    return 0;
}
void destruir_bitmap() {


    if (!bitmap_file) {
        log_error(logger, "Error al abrir el archivo bitmap.dat para escribir.");
        exit(EXIT_FAILURE);
    }

    // Escribir el bitmap al archivo
    fseek(bitmap_file, 0, SEEK_SET);
    size_t bytes_a_escribir = (bitarray_get_max_bit(bitmap) + 7) / 8;
    pthread_mutex_lock(mutex_logs);
    if (fwrite(bitmap->bitarray, sizeof(uint8_t), bytes_a_escribir, bitmap_file) != bytes_a_escribir) {
        log_error(logger, "Error al escribir el archivo bitmap.dat.");
        pthread_mutex_unlock(mutex_logs);
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(mutex_logs);
    fflush(bitmap_file);

    // Liberar recursos
    bitarray_destroy(bitmap);
    fclose(bitmap_file);

}
