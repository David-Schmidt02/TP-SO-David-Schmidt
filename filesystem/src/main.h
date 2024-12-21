#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <commons/log.h>
#include <bloques.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

void *bloques = NULL;
int block_size = 4; 

void *conexion_memoria(void * arg_memoria);
int main();
void levantar_conexiones();
char* crear_directorio(char* ruta_a_agregar);

typedef struct
{
	uint32_t * ptr;
    char *nombre_archivo;
	
} t_bloque_ptr;


#endif