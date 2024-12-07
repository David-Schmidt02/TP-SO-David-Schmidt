#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <utils/utils.h>

void inicializar_libres();
void inicializar_bitmap(char* mount_dir,uint32_t block_count);
int reservar_bloques();
bool espacio_disponible(uint32_t cantidad);
void liberar_bloque(uint32_t bloque);
void destruir_bitmap();

#endif