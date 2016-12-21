#ifndef NETWORKHANDLER_H_
#define NETWORKHANDLER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <commons/log.h>
#include <commons/string.h>

int librocket_enviarMensaje(int,const void*, size_t,  t_log*);
int librocket_levantarServidorSimple(char*, int, void(*function)(int mSocket));
int librocket_recibirMensaje(int, void*, size_t,  t_log*);
void loguearUltimoError(t_log* errorLogger, int lineNumber, char* fileName);
int librocket_levantarServidorConMultiplesHilos(char* PUERTO,void* (*funcionParaElHilo)(void sockfd));
int librocket_levantarServidorMultiplexado(char* PUERTO,void(*funcionParaConexionEntrante) (int mSocket, struct sockaddr_in remoteaddr));

void * hiloManejador(void* sockfd);
void funcionReceptoraDeConexion(int sockfd);

#endif /* NETWORKHANDLER_H_ */
