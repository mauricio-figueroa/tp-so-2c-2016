/*
 * globales.h
 *
 *  Created on: 30/11/2016
 *      Author: utnso
 */

#ifndef GLOBALES_H_
#define GLOBALES_H_


extern pthread_rwlock_t*  locks;
extern pthread_mutex_t mutexAsignacion;
extern pthread_mutex_t lockBitmap;
extern pthread_mutex_t obtenerbloqueLibreMutex;
extern pthread_mutex_t obtenerEntradaArchivoLibreMutex;
extern int ultimaEntradaLibreDeTabla; // apunta al siguiente indice de la tabla de paginas donde se deberia buscar una entrada libre
extern int entradasLibresOtorgadas[2048]; //array que en cada posicion informa si esa entrada fue dada a un hilo como libre, usado mas que nada para cuando doy una entrada libre pero todavia el hilo no llego a escribirla
extern int bloquesLibresOtorgados[2048]; // lo mismo pero para bloques
extern pthread_mutex_t editarBitEnBitmapsMutex;
#endif /* GLOBALES_H_ */
