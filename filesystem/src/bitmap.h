#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <utils/utils.h>
#include <math.h>

void inicializar_libres();
void inicializar_bitmap();
uint32_t reservar_bloques(uint32_t size);
bool espacio_disponible(uint32_t cantidad);
void liberar_bloque(uint32_t bloque);
void destruir_bitmap();

#endif