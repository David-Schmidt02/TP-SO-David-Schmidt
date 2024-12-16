#include "bitmap.h"

static t_bitarray* bitmap;
static pthread_mutex_t mutex_bitmap;
static FILE* bitmap_file;
extern uint32_t block_count;
extern int block_size;
extern int retardo_acceso;
extern char* mount_dir;
extern t_config *config;
extern t_log *logger;
int libres;

void inicializar_libres() {
    libres = (int)block_count;
}

void inicializar_bitmap(char* mount_dir, uint32_t block_count) {
    if (block_count == 0) {
        log_error(logger, "No se encontró el valor BLOCK_COUNT en el archivo de configuración.");
        exit(EXIT_FAILURE);
    }

    size_t tamanio_bitmap = (block_count + 7) / 8;

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
        if (fwrite(buffer, sizeof(uint8_t), tamanio_bitmap, bitmap_file) != tamanio_bitmap) {
            log_error(logger, "Error al escribir en el archivo bitmap.dat.");
            free(buffer);
            fclose(bitmap_file);
            free(path_bitmap);
            exit(EXIT_FAILURE);
        }
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


    pthread_mutex_init(&mutex_bitmap, NULL);

    log_info(logger, "Bitmap inicializado correctamente.");
    free(path_bitmap);
}

bool espacio_disponible(uint32_t cantidad) {
    pthread_mutex_lock(&mutex_bitmap);
    for (int i = 0; i < bitarray_get_max_bit(bitmap); i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            libres--;
            if (libres <= cantidad) {
                pthread_mutex_unlock(&mutex_bitmap);
                return true;
            }
        }
    }
    pthread_mutex_unlock(&mutex_bitmap);
    return false;
}

uint32_t reservar_bloque() {
    pthread_mutex_lock(&mutex_bitmap);
    if (!espacio_disponible(1)) {
        pthread_mutex_unlock(&mutex_bitmap);
        log_error(logger, "No hay bloques disponibles para reservar.");
        return -1;
    }
    for (uint32_t i = 0; i < bitarray_get_max_bit(bitmap); i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            bitarray_set_bit(bitmap, i);
            pthread_mutex_unlock(&mutex_bitmap);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex_bitmap);
    return -1; // No hay bloques disponibles
}

void liberar_bloque(uint32_t bloque) {
    pthread_mutex_lock(&mutex_bitmap);
    bitarray_clean_bit(bitmap, bloque);
    pthread_mutex_unlock(&mutex_bitmap);
}

void destruir_bitmap() {
    pthread_mutex_lock(&mutex_bitmap);

    if (!bitmap_file) {
        log_error(logger, "Error al abrir el archivo bitmap.dat para escribir.");
        pthread_mutex_unlock(&mutex_bitmap);
        exit(EXIT_FAILURE);
    }

    // Escribir el bitmap al archivo
    fseek(bitmap_file, 0, SEEK_SET);
    size_t bytes_a_escribir = (bitarray_get_max_bit(bitmap) + 7) / 8;
    if (fwrite(bitmap->bitarray, sizeof(uint8_t), bytes_a_escribir, bitmap_file) != bytes_a_escribir) {
        log_error(logger, "Error al escribir el archivo bitmap.dat.");
        pthread_mutex_unlock(&mutex_bitmap);
        exit(EXIT_FAILURE);
    }
    fflush(bitmap_file);

    // Liberar recursos
    bitarray_destroy(bitmap);
    fclose(bitmap_file);

    pthread_mutex_unlock(&mutex_bitmap);
    pthread_mutex_destroy(&mutex_bitmap);
}
