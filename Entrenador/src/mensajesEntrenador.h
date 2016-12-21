#ifndef MENSAJESENTRENADOR_H_
#define MENSAJESENTRENADOR_H_

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <protocolos/p_entrenador.h>
#include <protocolos/p_mapa.h>
#include "Entrenador.h"

// Funciones
void conocerPokenest(int, char, punto*);
void moverPosicionEntrenador(int, int);
int capturarPokemon(int, char);
void notificarFinDeObjetivos(int);
void pedirEstado(int, t_estado_entrenador*);
void copiarArchivoPokemonDirectorioBill(char *, char *);
int obtenerPokemonMasFuerte(char**);

#endif
