//conexiones.h

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <stdio.h>
#include <connections/networkHandler.h>
#include <threads/threads.h>
#include  <protocolos/p_pokeDex.h>
#include <sys/types.h>




pthread_t * abrirConexiones();
void* hiloAbrirConexiones();
void * hiloManejarConexiones(void* sockfd);

void interpretarMensaje(MSG_FROM_FS_CLIENTE_TO_SERVER tipoMensaje, int sockfd, FILE* fs);
void abrirArchivo(int sockfd, FILE* fs);
void leerArchivo(int sockfd, FILE* fs);
void crearArchivo(int sockfd, FILE* fs);
void crearDirectorio(int sockfd, FILE* fs);
void buscarDirectorio(int sockfd, FILE* fs);
void renombrarArchivo(int sockfd, FILE* fs);
void escribirArchivo(int sockfd, FILE* fs);
void getEstructuraDirectorio(int sockfd, FILE* fs);
void getAttr(int sockfd, FILE* fs);


#endif /* CONEXIONES_H_ */
