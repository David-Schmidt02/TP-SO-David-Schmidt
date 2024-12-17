#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/utils.h>
#include <bitmap.h>
#include <math.h>


extern t_config *config;
extern t_log *logger;

extern void *bloques;
extern int block_size;

void inicializar_bloques();
void escribir_bloque(int bloque, void *contenido, size_t tamanio);
void *leer_bloque(int bloque);
int crear_archivo_metadata(char* nombre_archivo, uint32_t tamanio);

int crear_archivo_dump(char* nombre_archivo, uint32_t tamanio, void* datos);
#endif
