#ifndef MAPA_H_
#define MAPA_H_

// Includes
#include <tad_items.h>
#include <pkmn/battle.h>
#include <pkmn/factory.h>
#include <stdlib.h>
#include <curses.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/string.h>
#include <connections/cliente.h>
#include <connections/networkHandler.h>
#include <protocolos/p_entrenador.h>
#include <signal.h>
#include <threads/threads.h>
#include <pthread.h>
#include <semaphore.h>

// Enums
typedef enum ENTRENADOR_ULTIMA_ACCION {

	NADA = 0,
	SE_MOVIO = 1,
	SE_DESCONECTO = 2,
	SE_CONECTO = 3,
	SE_BLOQUEO = 4,
	SE_DESBLOQUEO = 5,

} ENTRENADOR_ULTIMA_ACCION;

// Structs
typedef struct {
	int tiempoChequeoDeadlock;
	int batalla;
	int algoritmo;
	int quantum;
	int retardo;
	char* IP;
	char* puerto;
	char* pathMapa;
	char * pathToMetadata;
	char * pathToPuntoMontaje;
} t_mapa;

typedef struct {

	punto coordenada;
	char identificador;
	char* tipoPokemon;
	int cant;
	char* species;

} t_pokenest;

typedef struct {
	char simboloEntrenador;
	int sockfd; // socket de conexion con el entrenador
	punto punto;
	t_pokenest* proximaPokenest;
	t_list* listaRecursos; ///  listas de los recursos que tiene y quiere
	char recursoQueQuiere;
	char * nombre;
	char *pathEntrenador;
	ENTRENADOR_ULTIMA_ACCION ultimaAccion;
	int direccionDesconocida;
} t_pcb;

typedef struct {
	char * species;
	t_level lvlPokemon;
} t_pokemona;

//hay que realocar mmemoria dinamicamente a medida que se conecten entrenadores,
//la cantidad de columnas es la cantidad de recursos diferentes, que contenga el mapa.
int elRecursoEstaDiponible(char*, t_list *);
int tieneRecrusoMayorADisponible(t_pcb *, t_list *);
t_pokemona * obtenerPokemonMasFuerteEntrenador(t_pcb *);
void * detectarDeadLock(void * args);
void copiarEntrenadores(t_list*);
void cargarListaRecursos(t_list*);
void marcarEntrenadores(t_list *, t_list *, t_list *, t_list*);
void* alarmaInterbloqueo();
void levantarHiloConexiones();
void* iniciarGUI(void * args);
void planificate();
void sig_handler();
void liberarRecursos();
void srdfAlgorithm();
void recargarConfiguracionMetadataMapa();
void cargarConfiguracionMapa(int, char**);
t_config* crearConfigMapa(char *argv[]);
void setMapa(t_config*, t_mapa*);
void roundRobinAlgorithm();
void impirmirConfiguracion(char*);
t_pcb* getPcbBySockFdListos(int);
t_pcb* getPcbBySockFdBloqueados(int);
t_pokenest * getPokenestById(char);
void inicializarVariablesGlobales();
void configurarSeniales();
int calcularDistancia(t_pcb*);
char* itos(int numero);
void senialesMapa();
bool direccionDesconocida(t_pcb*);
void dibujarGUI();

// Variables
int ** recursosTotales; //matriz recursos Totales
int ** recursosAsignados; //matriz de recursos asignados
int ** recursosPedidos; //matriz de recursos pedidos

int retardoDeadlock;
int retardoPlanificacionMapa;

t_pcb* procesoCorriendo;
t_log* errorLogger;
t_log* logger;
t_log* infoLogger;
t_pkmn_factory* pokemon_factory;

t_config *configMapa;
t_config *configPokenest;

pthread_mutex_t mutexListaListos;
pthread_mutex_t mutex2;
pthread_mutex_t mutexListaBloqueados;
pthread_mutex_t mutexListaPokenest;

char * nombreMapaActual;
t_mapa* mapaActual;

sem_t semGui;
sem_t semConexion;
sem_t semEntrenadorYaSeDibujo;
sem_t semPlanificar;

t_list* listaListos;
t_list* listaBloqueados;
t_list* listaErrados;
t_list* listaFinalizados;
t_list* listaPokenest;
t_list* itemsDeGui;

#endif /* MAPA_H_ */

