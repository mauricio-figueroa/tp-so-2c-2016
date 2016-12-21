/*
 * globales.c
 *
 *  Created on: 30/11/2016
 *      Author: utnso
 */
#include<pthread.h>


pthread_rwlock_t* locks;
pthread_mutex_t lockBitmap;
pthread_mutex_t mutexAsignacion;
pthread_mutex_t obtenerbloqueLibreMutex;
pthread_mutex_t obtenerEntradaArchivoLibreMutex;
int ultimaEntradaLibreDeTabla;
int entradasLibresOtorgadas[2048];
int bloquesLibresOtorgados[2048];
pthread_mutex_t editarBitEnBitmapsMutex;
