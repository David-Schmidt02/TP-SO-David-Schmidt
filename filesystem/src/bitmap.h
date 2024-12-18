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

typedef struct {
    uint32_t bloque_indice;      // Número del bloque donde esta el indice
    uint32_t* bloques_datos;     // un array que en cada pos indica donde esta cada bloque
    uint32_t cantidad_bloques;   // Cantidad de bloques reservados
} t_reserva_bloques;

void inicializar_libres();
void inicializar_bitmap();
t_reserva_bloques* reservar_bloques(uint32_t size);
bool espacio_disponible(uint32_t cantidad);
void liberar_bloque(uint32_t bloque);
void destruir_bitmap();
int cargar_bitmap();


#endif
