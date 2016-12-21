#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <files/fileManager.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <protocolos/p_entrenador.h>
#include <protocolos/p_mapa.h>
#include <time/time.h>
#include <commons/log.h>
#include <threads/threads.h>
#include <dirent.h>
#include <commons/string.h>
#include <connections/networkHandler.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>

// Structs
typedef struct argumentos_t {
	char* nombreEntrenador;
	char* rutaPokedex;
} argumentos_t;

typedef struct tiempo {
	char* bloqueado;
	char* totalDeAventura;
} t_tiempo;

typedef struct entrenadorActual {
	char* nombre;
	char* simbolo;
	char* path;
	t_tiempo* tiempo;
	t_list* mapas;
	int vidas;
	int intentos;
	int deadlocks;
} t_entrenador;

typedef struct mapa {
	char* nombre;
	t_list* objetivos;
	char* ip;
	int puerto;
	int completado;
} t_mapa;

typedef struct mapaInfo {
	char* ip;
	int puerto;
} t_mapaInfo;

typedef struct actividadEntrenador {
	int estado;
	punto* posicionDelEntrenador;
} t_actividadEntrenador;

// Enums
typedef enum {

	SIN_POKENEST = 1,
	EN_CAMINO = 2,
	EN_POKENEST = 3,
	CON_TODOS_LOS_POKEMONS = 4,
	MAPA_COMPLETADO = 5,
	BLOQUEADO = 6,
	EN_DEADLOCK = 7

} t_estado_entrenador;

//Funciones
void cargarMetadataEntrenador(int, char**);
void senialesEntrenador();
void aventuraEntrenador();
void cargarParametros(char **);
void freeParametros(argumentos_t*);
void setEntrenador(t_config*);
t_config *crearConfigEntrenador();
void cargarHojaDeViaje(t_config*);
void imprimirMapasYObjetivosDeEntrenador();
void cargarIPyPuertoDeMapas();
void obtenerInfoDeMetaData(char*, t_mapaInfo*);
void imprimirIPyPuertosDeMapas();
void sig_handler(int);
int completarMapa(t_mapa*, t_actividadEntrenador*);
int handShake(t_mapa*, t_actividadEntrenador*);
int obtenerDireccion(punto*, punto*, int);
void fallecerPorDeadlock(int, t_actividadEntrenador*);
void copiarArchivoDePokemon(char*);
void posicionarNuevaCoordenada(int, t_actividadEntrenador*);
void borrarPokemonesCapturados();
void borrarMedallasObtenidas();
void reiniciarHojaDeViaje();
void tiempoTotalAventura(char*, char*, char*);
void copiarArchivoDeMedalla(char*);
void reiniciarJuego();
int darPokemonMasFuerte(int);
bool estaCompleto(t_mapa*);

// Variables Globales
t_entrenador* entrenadorActual;
t_mapa* mapaActual;
t_log* errorLogger;
t_log* infoLogger;
int mapasCompletos;
int INDEX_PROXIMO_POKEMON;

#endif

