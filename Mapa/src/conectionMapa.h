#ifndef CONECTIONMAPA_H_
#define CONECTIONMAPA_H_

// Includes
#include "Mapa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/log.h>
#include <protocolos/p_entrenador.h>
#include <protocolos/p_mapa.h>
#include <dirent.h>
#include <files/fileManager.h>

// Funciones
void eliminarSocketListaListos(int, bool);
void eliminarSocketListaBloqueados(int sock);
void* nuevoEntrenadorConectado(int, struct sockaddr_in);
int mensajeDeEntrenadorRecibido(int);
punto coordenadaPokenest(char);
void liberarRecursosListos(int);
void liberarRecursosBloqueados(t_pcb*);
void reintegrarRecurso(char, t_pcb *);
int obtenerPokenest(int);
int moverPosicion(int);
int capturarPokemon(int);
void reubicarProcesos(int);
void cargarPokenest();
void eliminarArchivo(char *, char *);
int nombreArchivoInvalido(char *);
void obtenerCantidad(char*, char *);
char* adquirirPathPokemonEnEspecifico(t_pokenest*);
punto convertirPunto(char*);
void darMedalla(int);
void recargaMetaData();
void agregarEntrenadorAListos(t_list* lista, t_pcb* pcbEntrenador);

#endif /* CONECTIONMAPA_H_ */
