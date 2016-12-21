#include "Mapa.h"
#include "DeteccionDeadlock.h"
#include "conectionMapa.h"

void* detectarDeadLock(void * args) {

	//printf("Corriendo chekeo de deadlock \n");

	pthread_mutex_lock(&mutexListaBloqueados);
	int cantBloqueados = list_size(listaBloqueados);
	pthread_mutex_unlock(&mutexListaBloqueados);
	int cantidadMarcados;

	if (cantBloqueados != 0 && mapaActual->batalla) {


		//puts("Ejecutandose deadlock");

		log_info(infoLogger, "Ejecutando deadlock");

		t_list* entrenadoresBloqueadosAuxiliar = list_create();
		t_list* entrenadoresMarcados = list_create();
		t_list* recursosMapaAuxiliar = list_create();
		t_list* entrenadoresNoMarcados = list_create();

		//lista de t_pcb
		copiarEntrenadores(entrenadoresBloqueadosAuxiliar);
		//lista de t_pokenest
		cargarListaRecursos(recursosMapaAuxiliar);

		//marco los personajes que tienen recursos y piden un recurso que no esta disponible temporalmente
		marcarEntrenadores(entrenadoresMarcados, entrenadoresNoMarcados,
				recursosMapaAuxiliar, entrenadoresBloqueadosAuxiliar);
		//Libera los entrenadores que tienen menos de los que
		liberarNoMarcados(entrenadoresNoMarcados);

		cantidadMarcados = list_size(entrenadoresMarcados);

		while (cantidadMarcados > 1) {

			batalla(entrenadoresMarcados);
			cantidadMarcados = list_size(entrenadoresMarcados);
			//hasta que la cantidad de marcados sea menor a 2, lo cual termina el algoritmo de deadlock
		}

	}

	liberarBloqueados();

	//no se si deberia llamar a si misma n veces ¿?
	//detectarDeadLock();

}

void liberarBloqueados() {

	int sizeBloqueados = list_size(listaBloqueados);
	int sizeRecursos = list_size(listaPokenest);

	int i, j, mensaje;
	t_pcb* entrenadorBloqueado;
	t_pokenest* recurso;

	for (i = 0; i < sizeBloqueados; i++) {

		entrenadorBloqueado = list_get(listaBloqueados, i);

		for (j = 0; j < sizeRecursos; j++) {

			recurso = list_get(listaPokenest, j);

			if (entrenadorBloqueado->proximaPokenest->identificador
					== recurso->identificador) {

				if (recurso->cant >= 1) {

					//ESTAS_DESBLOQUEADO
					mensaje = 11;
					librocket_enviarMensaje(entrenadorBloqueado->sockfd,
							&mensaje, sizeof(int), errorLogger);
					entrenadorBloqueado->ultimaAccion = SE_DESBLOQUEO;
					sem_post(&semGui);
					sem_wait(&semEntrenadorYaSeDibujo);
					list_remove(listaBloqueados, i);

				}

			}

		}

	}

}

//TODO verificar los malloc
void batalla(t_list* marcados) {

	int perdedor;

	t_pcb * primerEntrenadorABatallar = list_get(marcados, 0);
	t_pcb * segundoEntrenadorABatallar = list_get(marcados, 1);

	//TODO CAMBIAR EL 4
	int mensaje = ESTAS_EN_DEADLOCK, mensajeGanador = 0;

	librocket_enviarMensaje(primerEntrenadorABatallar->sockfd, &mensaje,
			sizeof(int), errorLogger);
	librocket_enviarMensaje(segundoEntrenadorABatallar->sockfd, &mensaje,
			sizeof(int), errorLogger);

	t_pokemona* pokemonPrimerEntrenador = obtenerPokemonMasFuerteEntrenador(
			primerEntrenadorABatallar);
	t_pokemona* pokemonSegundoEntrenador = obtenerPokemonMasFuerteEntrenador(
			segundoEntrenadorABatallar);


	char * entrenadorLog=string_new();
	string_append(&entrenadorLog, "los entrenadores selecionados para la batalla son: ");
	string_append(&entrenadorLog,primerEntrenadorABatallar->nombre );
	string_append(&entrenadorLog," y " );
	string_append(&entrenadorLog,segundoEntrenadorABatallar->nombre);

	log_info(infoLogger, entrenadorLog);


	//BATALLA!
	perdedor = resolverDeadlock(pokemonPrimerEntrenador,
			pokemonSegundoEntrenador);

	if (perdedor == 1) {

		if (list_size(marcados) == 2) {
			mensaje = MORITE;
			liberarRecursosBloqueados(primerEntrenadorABatallar);
			eliminarSocketListaBloqueados(primerEntrenadorABatallar->sockfd);

		} else {
			mensaje = ESTAS_EN_DEADLOCK;
		}

		//GANASTE
		mensajeGanador = 12;

		librocket_enviarMensaje(primerEntrenadorABatallar->sockfd, &mensaje,
				sizeof(int), errorLogger);
		librocket_enviarMensaje(segundoEntrenadorABatallar->sockfd,
				&mensajeGanador, sizeof(int), errorLogger);

		list_remove(marcados, 1);

	}

	if (perdedor == 2) {

		if (list_size(marcados) == 2) {
			//MORITE
			mensaje = 1;
			liberarRecursosBloqueados(segundoEntrenadorABatallar);
			eliminarSocketListaBloqueados(segundoEntrenadorABatallar->sockfd);
		} else {
			//ESTAS_EN_DEADLOCK
			mensaje = 4;
		}

		//GANASTE
		mensajeGanador = 12;

		librocket_enviarMensaje(segundoEntrenadorABatallar->sockfd, &mensaje,
				sizeof(int), errorLogger);
		librocket_enviarMensaje(primerEntrenadorABatallar->sockfd,
				&mensajeGanador, sizeof(int), errorLogger);

		list_remove(marcados, 0);

	}

//actualizar lista de entrenadoresBloqueadosAuxiliar
//y vector de disponibles, volver a correr desde cargarListaRecuros

}
//TODO preguntar si lo hago a mano o lo saco del file system, (para mi es mejor tener en memoria esto)

//TODO ahora obtengo el primerop pero deberia obtener el mas fuerte

t_pokemona* obtenerPokemonMasFuerteEntrenador(t_pcb * entrenador) {

	int nivel = 0, longitudNombre = 0;

	recv(entrenador->sockfd, &nivel, sizeof(int), 0);
	recv(entrenador->sockfd, &longitudNombre, sizeof(int), 0);

	char* nombre = malloc(longitudNombre);

	recv(entrenador->sockfd, nombre, longitudNombre, 0);

	t_pokemona* pokemonMasFuerte = malloc(sizeof(t_pokemona));
	pokemonMasFuerte->species = string_new();

	pokemonMasFuerte->lvlPokemon = nivel;
	string_append(&pokemonMasFuerte->species, nombre);

	return pokemonMasFuerte;

}

void marcarEntrenadores(t_list* listaMarcados, t_list* listaNoMarcados,
		t_list* listaRecursos, t_list * entrenadoresBloqueados) {
	int cantidadBloqueados = list_size(entrenadoresBloqueados);
	int i;
	int recurso;
	for (i = 0; i < cantidadBloqueados; i++) {
		t_pcb * currentPcb = list_get(entrenadoresBloqueados, i);
		recurso = tieneRecursoMayorADisponible(currentPcb, listaRecursos);

		if (recurso == 1) {
			list_add(listaMarcados, currentPcb);
		} else {
			list_add(listaNoMarcados, currentPcb);
		}

	}
}

//TODO verificar identificador pokenest
//TODO revisar esta logica de disponibilidad :0
//si devuelve 0 no esta disponible, si devuelve 1 si lo esta.
int elRecursoNoEstaDisponible(char* identificadorPokenest, t_list * recursos) {

	int cantidadDeTiposDeRecursosEnElMapa = list_size(recursos);
	int i;
	int disponible = 1;

	for (i = 0; i < cantidadDeTiposDeRecursosEnElMapa; i++) {
		t_pokenest * currentPokenest = list_get(recursos, i);
		if (currentPokenest->identificador == identificadorPokenest[0]) {
			if (currentPokenest->cant >= 1) {
				disponible = 0;
			}
		}
	}

	return disponible;
}

//retorna 1 si tieneRecursoMayorAlDisponible
int tieneRecursoMayorADisponible(t_pcb * currentPcb, t_list * recursos) {
	char * identificadorPokenest;
	int cantidadRecursosQuePosee = list_size(currentPcb->listaRecursos);
	int i;
	int noDisponible;

	for (i = 0; i < cantidadRecursosQuePosee; i++) {
		identificadorPokenest = list_get(currentPcb->listaRecursos, i);
		noDisponible = elRecursoNoEstaDisponible(identificadorPokenest,
				recursos);

		if (noDisponible == 1) {
			break;
		}
	}

	return noDisponible;

}

//TODO ver si lo que tiene que matchear es el identificador
void cargarListaRecursos(t_list* recursosMapaAuxiliar) {
	pthread_mutex_lock(&mutexListaPokenest);

	int cantidadPokenest = list_size(listaPokenest);
	int j = 0;

	while (j < cantidadPokenest) {
		t_pokenest* pokenest = list_get(listaPokenest, j);
		t_pokenest* currentPokenest = malloc(sizeof(t_pokenest));

		memcpy(currentPokenest, pokenest, sizeof(t_pokenest));

		list_add(recursosMapaAuxiliar, currentPokenest);

		j++;

	}

	pthread_mutex_unlock(&mutexListaPokenest);
}

void copiarEntrenadores(t_list* entrenadoresBloqueadosAux) {

	pthread_mutex_lock(&mutexListaBloqueados);
	int size = list_size(listaBloqueados);

	if (size > 1) {
		int i = 0;
		while (i < size) {
			t_pcb* pcb = list_get(listaBloqueados, i);
			t_pcb* currentPcb = malloc(sizeof(t_pcb));

			memcpy(currentPcb, pcb, sizeof(t_pcb));

			list_add(entrenadoresBloqueadosAux, currentPcb);
			i++;
		}
	}

	pthread_mutex_unlock(&mutexListaBloqueados);

}

int resolverDeadlock(t_pokemona* primerPokemon, t_pokemona* segundoPokemon) {


	/*
	 Creamos una instancia de la Factory
	 t_pkmn_factory, sirve para crear pokémons solo necesita nombre y el nivel
	 dentro tiene un map de todos los pokemons, su nombre y su tipo, por lo cual al pasarle el nombre
	 hace un get por la key
	 */

	//IMPORTANTE EL NOMBRE DEL POKEMON DEBE COMENZAR CON LETRA MAYUSCULA.
	t_pokemon* pokemon1 = create_pokemon(pokemon_factory,
			primerPokemon->species, primerPokemon->lvlPokemon);
	t_pokemon* pokemon2 = create_pokemon(pokemon_factory,
			segundoPokemon->species, segundoPokemon->lvlPokemon);

	//Si el nombre del Pokémon no existe en los primeros 151 o está mal escrito
	//Retornará el puntero NULL (0x0)

	/*printf("Preparando la batalla! \n");
	 printf("Preparando primer pokemon: %s[%s/%s] Nivel: %d\n",
	 pokemon1->species, pkmn_type_to_string(pokemon1->type),

	 //sirve para ver el Tipo de Enum como un String

	 pkmn_type_to_string(pokemon1->second_type), pokemon1->level);*/
	/*printf("Preparando segundo pokemon: %s[%s/%s] Nivel: %d\n",
	 pokemon2->species, pkmn_type_to_string(pokemon2->type),

	 //sirve para ver el Tipo de Enum como un String
	 pkmn_type_to_string(pokemon2->second_type), pokemon2->level);*/

	//Batalla!!!!

	//printf("Batalla \n");


	log_info(infoLogger, "Empieza la batalla de deadlock");


	t_pokemon * loser = pkmn_battle(pokemon1, pokemon2);
	//printf("El Perdedor es: %s\n", loser->species);



	//Como el puntero loser apunta a alguno de los otros 2, no se lo libera
	if (loser != NULL) {
		if (!strcmp(loser->species, pokemon1->species)) {
			return 1;
		}

		if (!strcmp(loser->species, pokemon2->species)) {
			return 2;
		}
	}

	//Liberemos los recursos
	free(pokemon1);
	free(pokemon2);

	return 0;

}

void liberarNoMarcados(t_list* listaNoMarcados) {

	int i;
	int cantElem = list_size(listaNoMarcados);

	for (i = 0; i < cantElem; i++) {

		t_pcb * pcbALiberar = list_get(listaNoMarcados, i);

		pthread_mutex_lock(&mutexListaListos);
		list_add(listaListos, pcbALiberar);
		sem_post(&semPlanificar);
		pthread_mutex_unlock(&mutexListaListos);
	}

}
