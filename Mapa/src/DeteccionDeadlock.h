#ifndef DETECCIONDEADLOCK_H_
#define DETECCIONDEADLOCK_H_

// Includes
#include <pkmn/battle.h>
#include <pkmn/factory.h>

// Funciones
void* detectarDeadLock(void*);
void batalla(t_list*);
t_pokemona* obtenerPokemonMasFuerteEntrenador(t_pcb*);
void marcarEntrenadores(t_list*, t_list*, t_list*, t_list*);
int elRecursoEstaDisponible(char*, t_list*);
int tieneRecursoMayorADisponible(t_pcb*, t_list*);
void cargarListaRecursos(t_list*);
void copiarEntrenadores(t_list*);
int resolverDeadlock(t_pokemona*, t_pokemona*);
void liberarNoMarcados(t_list*);
void sumarADisponible(t_pcb*);
void liberarBloqueados();

// Variables Globales
t_list* entrenadoresBloqueadosAuxiliar;
t_list* entrenadoresMarcados;
t_list* recursosMapaAuxiliar;
t_list* entrenadoresNoMarcados;

#endif
