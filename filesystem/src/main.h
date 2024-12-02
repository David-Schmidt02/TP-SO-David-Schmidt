#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <bloques.h>

void *bloques = NULL;
int block_size = 4; 

void *conexion_memoria(void * arg_memoria);
int main();
void levantar_conexiones();

#endif