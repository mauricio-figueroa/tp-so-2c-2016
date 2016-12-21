#include "Entrenador.h"
#include "mensajesEntrenador.h"

#define PORT "5050"
#define IP "127.0.0.1"
#define MAXDATASIZE 1024

int INDEX_PROXIMO_MAPA = 0;

int main(int argc, char **argv) {

	errorLogger = log_create("error_logs", "entrenador", false,
			LOG_LEVEL_ERROR);
	infoLogger = log_create("info_logs", "entrenador", false, LOG_LEVEL_INFO);

	entrenadorActual = malloc(sizeof(t_entrenador));

	cargarMetadataEntrenador(argc, argv);

	pthread_t hiloSeniales;
	crearHilo(&hiloSeniales, (void *) senialesEntrenador, NULL);

	aventuraEntrenador();

	free(entrenadorActual);

	return 0;

}

void cargarMetadataEntrenador(int argc, char** argv) {

	char* infoLogFaltanParametros = string_new();
	char* infoLogNombreEntrenador = string_new();

	if (argc < 2) {

		string_append(&infoLogFaltanParametros,
				"Pasar por parametro nombre del entrenador y ruta del FS. \n");
		log_info(infoLogger, infoLogFaltanParametros);

		exit(1);

	}

	cargarParametros(argv);

	string_append(&infoLogNombreEntrenador, "Soy el entrenador ");
	string_append(&infoLogNombreEntrenador, entrenadorActual->nombre);
	log_info(infoLogger, infoLogNombreEntrenador);

	t_config* configEntrenador = crearConfigEntrenador();
	setEntrenador(configEntrenador);

	cargarIPyPuertoDeMapas(entrenadorActual);
	imprimirIPyPuertosDeMapas(entrenadorActual);

	free(infoLogFaltanParametros);
	free(infoLogNombreEntrenador);

}

void senialesEntrenador() {

	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
		printf("\nNo se pudo catchear SIGUSR1\n");
	}

	if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		printf("\nNo se pudo catchear SIGTERM\n");
	}

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("\nNo se pudo catchear SIGINT\n");
	}

}

void aventuraEntrenador() {

	t_actividadEntrenador* actividad = malloc(sizeof(t_actividadEntrenador));

	mapasCompletos = 0;

	char* instanceInicio = temporal_get_string_time();

	int cantidadDeMapas = list_size(entrenadorActual->mapas), seCompleto = 0,
			indexMapas, completoTodo = 0;

	mapaActual = list_get(entrenadorActual->mapas, INDEX_PROXIMO_MAPA);

	for (indexMapas = 0; indexMapas < cantidadDeMapas; indexMapas++) {

		do {
			seCompleto = completarMapa(mapaActual, actividad);

			if (seCompleto) {
				mapaActual->completado = 1;
				printf("Se completo con exito el mapa: %s\n",
						mapaActual->nombre);
				mapasCompletos++;
				INDEX_PROXIMO_MAPA++;
				mapaActual = list_get(entrenadorActual->mapas,
						INDEX_PROXIMO_MAPA);
			}

		} while (!list_all_satisfy(entrenadorActual->mapas,
				(void *) estaCompleto));

		completoTodo = 1;

	}

	char* instanceFin = temporal_get_string_time();
	char* calculo = calcular(instanceInicio, instanceFin);

	string_append(&entrenadorActual->tiempo->totalDeAventura, calculo);

	if (completoTodo) {

		printf("El entrenador %s se ha convertido en maestro pokemon!\n",
				entrenadorActual->nombre);
		printf("Tardo: %s", entrenadorActual->tiempo->totalDeAventura);
		printf(" en completar la aventura.\n");
		printf("Tiempo bloqueado en pokenest: %s\n",
				entrenadorActual->tiempo->bloqueado);
		printf("Cantidad de Deadlocks en los que estuvo involucrado: %i\n",
				entrenadorActual->deadlocks);
		printf("Y murio %i", entrenadorActual->intentos);
		printf(" veces.\n");

	}

	free(actividad);

}

bool estaCompleto(t_mapa* unMapa) {

	return unMapa->completado;

}

void cargarParametros(char * argv[]) {

	entrenadorActual->nombre = string_new();
	entrenadorActual->path = string_new();

	string_append(&entrenadorActual->nombre, argv[1]);
	string_append(&entrenadorActual->path, argv[2]);

}

void freeParametros(argumentos_t * parametros) {

	free(parametros->rutaPokedex);
	free(parametros->nombreEntrenador);
	free(parametros);

}

t_config* crearConfigEntrenador() {

	char* pathAMetadata = string_new();
	string_append(&pathAMetadata, entrenadorActual->path);
	string_append(&pathAMetadata, "/Entrenadores/");
	string_append(&pathAMetadata, entrenadorActual->nombre);
	string_append(&pathAMetadata, "/metadata");

	t_config* unEntrenadorConfig = config_create(pathAMetadata);

	free(pathAMetadata);

	return unEntrenadorConfig;

}

void setEntrenador(t_config* configEntrenador) {

	entrenadorActual->mapas = list_create();

	entrenadorActual->nombre = string_new();
	entrenadorActual->simbolo = string_new();

	string_append(&entrenadorActual->nombre,
			config_get_string_value(configEntrenador, "nombre"));
	string_append(&entrenadorActual->simbolo,
			config_get_string_value(configEntrenador, "simbolo"));

	entrenadorActual->tiempo = malloc(sizeof(t_tiempo));
	entrenadorActual->tiempo->totalDeAventura = string_new();
	entrenadorActual->tiempo->bloqueado = string_new();
	string_append(&entrenadorActual->tiempo->bloqueado, "00:00:00:0000");

	entrenadorActual->deadlocks = 0;

	cargarHojaDeViaje(configEntrenador);

	entrenadorActual->vidas = config_get_int_value(configEntrenador, "vidas");
	entrenadorActual->intentos = 0;

	imprimirMapasYObjetivosDeEntrenador(entrenadorActual);

}

void imprimirMapasYObjetivosDeEntrenador() {

	printf("Imprimiendo data del entrenador %s\n", entrenadorActual->nombre);
	int cantidadMapas = list_size(entrenadorActual->mapas);
	int i;

	for (i = 0; i < cantidadMapas; i++) {

		t_mapa* mapa = list_get(entrenadorActual->mapas, i);
		printf("Mapa: %s \n", mapa->nombre);
		printf("Objetivos: ");

		int j = 0;
		int cantidadObjetivos = list_size(mapa->objetivos);
		for (; j < cantidadObjetivos; j++) {
			char* objetivo = list_get(mapa->objetivos, j);
			printf("%s,", objetivo);
		}
		printf("\n");

	}

}

void cargarHojaDeViaje(t_config *configEntrenador) {

	char** hojaDeViaje = config_get_array_value(configEntrenador,
			"hojaDeViaje");

	int i = 0, j = 0;

	char* nombreMapa = string_new();
	string_append(&nombreMapa, hojaDeViaje[i]);

	while (nombreMapa != NULL) {

		char* objetivoDeMapaKey = string_new();
		string_append(&objetivoDeMapaKey, "obj[");
		string_append(&objetivoDeMapaKey, nombreMapa);
		string_append(&objetivoDeMapaKey, "]");

		char** objetivos = config_get_array_value(configEntrenador,
				objetivoDeMapaKey);

		char* objetivo;

		t_mapa* structMapa = malloc(sizeof(t_mapa));
		structMapa->nombre = string_new();

		structMapa->completado = 0;
		string_append(&structMapa->nombre, nombreMapa);
		structMapa->objetivos = list_create();

		objetivo = objetivos[j];

		while (objetivo != NULL) {

			list_add(structMapa->objetivos, objetivo);

			j++;
			objetivo = objetivos[j];

		}

		list_add(entrenadorActual->mapas, structMapa);

		free(objetivos);
		free(objetivoDeMapaKey);

		j = 0;
		i++;
		nombreMapa = hojaDeViaje[i];
	}

	free(nombreMapa);
	free(hojaDeViaje);

}

void cargarIPyPuertoDeMapas() {

	t_mapa* mapaActual = malloc(sizeof(t_mapa));
	t_mapaInfo* mapaInfo = malloc(sizeof(t_mapaInfo));

	int cantidadDeMapas = list_size(entrenadorActual->mapas), i = 0;

	for (; i < cantidadDeMapas; i++) {

		mapaActual = list_get(entrenadorActual->mapas, i);

		mapaActual->ip = string_new();

		obtenerInfoDeMetaData(mapaActual->nombre, mapaInfo);

		string_append(&mapaActual->ip, mapaInfo->ip);

		mapaActual->puerto = mapaInfo->puerto;

	}

}

void obtenerInfoDeMetaData(char* mapaActual, t_mapaInfo* mapaInfo) {

	char* path = string_new();

	string_append(&path, entrenadorActual->path);
	string_append(&path, "/Mapas/");

	string_append(&path, mapaActual);
	string_append(&path, "/metadata");

	t_config *configMapa = config_create(path);

	mapaInfo->ip = string_new();
	string_append(&(mapaInfo->ip), config_get_string_value(configMapa, "IP"));
	mapaInfo->puerto = config_get_int_value(configMapa, "Puerto");

	free(path);
	config_destroy(configMapa);

}

void imprimirIPyPuertosDeMapas() {

	printf(
			"Imprimiendo informacion de los mapas correspondientes al entrenador: %s\n",
			entrenadorActual->nombre);

	t_mapa* mapaActual = malloc(sizeof(t_mapa));

	int cantidadMapas = list_size(entrenadorActual->mapas);
	int i;
	for (i = 0; i < cantidadMapas; i++) {
		mapaActual = list_get(entrenadorActual->mapas, i);
		printf("Mapa: %s \n IP: %s \n Puerto: %i \n", mapaActual->nombre,
				mapaActual->ip, mapaActual->puerto);

	}

}

void sig_handler(int signal) {
	switch (signal) {

	case SIGUSR1:
		printf("\nEntrenador gano una vida\n");
		entrenadorActual->vidas++;
		break;

	case SIGTERM:

		if (entrenadorActual->vidas > 0) {
			entrenadorActual->vidas--;
			printf("\nEntrenador perdio una vida\n");
		} else {
			printf("\nEl entrenador perdio su ultima vida...");
			reiniciarJuego();
		}
		break;

	case SIGINT:
		printf("\nEl entrenador %s ha salido del juego.\n",
				entrenadorActual->nombre);
		exit(1);
		break;

	}

}

int completarMapa(t_mapa* unMapa, t_actividadEntrenador* actividad) {

	int resultadoCaptura;
	int mapaSocket = handShake(unMapa, actividad);

	INDEX_PROXIMO_POKEMON = 0;

	if (mapaSocket == -1) {
		//No se pudo conectar al mapa
		exit(1);
	}

	int ultimaDireccion = SIN_DIRECCION;

	char* proximoPokemon;
	char* inicioDelBloqueo = string_new();
	char* finDelBloqueo = string_new();

	punto* puntoProximaPokenest = malloc(sizeof(punto));
	char idPokemon;
	int desbloqueo, resultadoDeadlock = 0;

	while (1) {

		switch (actividad->estado) {

		case SIN_POKENEST:
			proximoPokemon = list_get(unMapa->objetivos, INDEX_PROXIMO_POKEMON);
			idPokemon = proximoPokemon[0];
			conocerPokenest(mapaSocket, idPokemon, puntoProximaPokenest);
			actividad->estado = EN_CAMINO;
			break;

		case EN_CAMINO:
			ultimaDireccion = obtenerDireccion(puntoProximaPokenest,
					actividad->posicionDelEntrenador, ultimaDireccion);

			moverPosicionEntrenador(mapaSocket, ultimaDireccion);
			posicionarNuevaCoordenada(ultimaDireccion, actividad);

			if (actividad->posicionDelEntrenador->puntox
					== puntoProximaPokenest->puntox
					&& actividad->posicionDelEntrenador->puntoy
							== puntoProximaPokenest->puntoy) {
				actividad->estado = EN_POKENEST;
			}

			break;

		case EN_POKENEST:
			resultadoCaptura = capturarPokemon(mapaSocket, idPokemon);

			if (resultadoCaptura == 1) {
				actividad->estado = SIN_POKENEST;
				INDEX_PROXIMO_POKEMON++;
			}

			if (resultadoCaptura == -2) {

				string_append(&inicioDelBloqueo, temporal_get_string_time());

				actividad->estado = BLOQUEADO;
			}

			if (INDEX_PROXIMO_POKEMON == unMapa->objetivos->elements_count) {
				actividad->estado = CON_TODOS_LOS_POKEMONS;
			}

			break;

		case BLOQUEADO:

			recv(mapaSocket, &desbloqueo, sizeof(int), 0);

			if (desbloqueo == ESTAS_BLOQUEADO) {
				actividad->estado = BLOQUEADO;
			}

			if (desbloqueo == ESTAS_EN_DEADLOCK) {
				actividad->estado = EN_DEADLOCK;
			}

			// ESTAS_DESBLOQUEADO = 11
			if (desbloqueo == 11) {

				actividad->estado = EN_POKENEST;

				string_append(&finDelBloqueo, temporal_get_string_time());

				entrenadorActual->tiempo->bloqueado = sumar(
						entrenadorActual->tiempo->bloqueado,
						calcular(inicioDelBloqueo, finDelBloqueo));

			}

			break;

		case EN_DEADLOCK:

			entrenadorActual->deadlocks++;

			resultadoDeadlock = darPokemonMasFuerte(mapaSocket);

			if (resultadoDeadlock == MORITE) {
				fallecerPorDeadlock(mapaSocket, actividad);
			}

			//GANASTE
			//	if (resultadoDeadlock == 12) {
			//		actividad->estado = EN_POKENEST;
			//	}

			if (resultadoDeadlock == 12
					|| resultadoDeadlock == ESTAS_EN_DEADLOCK
					|| resultadoDeadlock == ESTAS_BLOQUEADO) {
				actividad->estado = BLOQUEADO;
			}

			break;

		case CON_TODOS_LOS_POKEMONS:
			notificarFinDeObjetivos(mapaSocket);
			actividad->estado = MAPA_COMPLETADO;
			break;

		case MAPA_COMPLETADO:
			close(mapaSocket);
			free(puntoProximaPokenest);
			free(proximoPokemon);
			return 1;
			break;
		}
	}

	return 0;

}

int obtenerDireccion(punto* puntoProximaPokenest, punto* posicionDelEntrenador,
		int ultimaDireccion) {

	int direccion = 0, irDerecha = 0, irIzquierda = 0, sobreAbsisaX = 0,
			irArriba = 0, irAbajo = 0, sobreAbsisaY = 0;

	irDerecha = (puntoProximaPokenest->puntox > posicionDelEntrenador->puntox);
	irIzquierda =
			(puntoProximaPokenest->puntox < posicionDelEntrenador->puntox);
	sobreAbsisaX = (puntoProximaPokenest->puntox
			== posicionDelEntrenador->puntox);

	irArriba = (puntoProximaPokenest->puntoy > posicionDelEntrenador->puntoy);
	irAbajo = (puntoProximaPokenest->puntoy < posicionDelEntrenador->puntoy);
	sobreAbsisaY = (puntoProximaPokenest->puntoy
			== posicionDelEntrenador->puntoy);

	if (irDerecha && ultimaDireccion != DERECHA) {
		return DERECHA;
	}

	if (irIzquierda && ultimaDireccion != IZQUIERDA) {
		return IZQUIERDA;
	}

	if (sobreAbsisaY && irDerecha) {
		return DERECHA;
	}

	if (sobreAbsisaY && irIzquierda) {
		return IZQUIERDA;
	}

	if (irArriba && ultimaDireccion != ARRIBA) {
		return ARRIBA;
	}

	if (irAbajo && ultimaDireccion != ABAJO) {
		return ABAJO;
	}

	if (sobreAbsisaX && irAbajo) {
		return ABAJO;
	}

	if (sobreAbsisaX && irArriba) {
		return ARRIBA;
	}

	if (sobreAbsisaX && sobreAbsisaY) {
		return SOBRE_POKENEST;
	}

	else {
		return SIN_DIRECCION;
	}

	return direccion;

}

void posicionarNuevaCoordenada(int ultimaDireccion,
		t_actividadEntrenador* actividad) {

	switch (ultimaDireccion) {

	case IZQUIERDA:
		actividad->posicionDelEntrenador->puntox--;
		break;

	case DERECHA:
		actividad->posicionDelEntrenador->puntox++;
		break;

	case ARRIBA:
		actividad->posicionDelEntrenador->puntoy++;
		break;

	case ABAJO:
		actividad->posicionDelEntrenador->puntoy--;
		break;

	}

	if (ultimaDireccion == SOBRE_POKENEST) {
		actividad->estado = EN_POKENEST;
	}

}

void copiarArchivoDePokemon(char* pokepath) {

	char** rutas = string_split(pokepath, "/");
	int indicePartes = 0;
	char *rutaPokeMonEnMapa = string_new();
	;

	while (rutas[indicePartes] != NULL) {
		puts(rutas[indicePartes]);
		indicePartes++;
	}
	indicePartes--;
	int i = 0;
	int flag = indicePartes - 1;
	string_append(&rutaPokeMonEnMapa, "/");
	while (indicePartes != i) {
		string_append(&rutaPokeMonEnMapa, rutas[i]);

		if (flag != i) {
			string_append(&rutaPokeMonEnMapa, "/");
		}
		i++;
	}
	indicePartes--;

	string_append(&rutaPokeMonEnMapa, "/");
	string_append(&rutaPokeMonEnMapa, rutas[indicePartes]);
	string_append(&rutaPokeMonEnMapa, "001.dat");

	char* pathNuevoPokemonACopiar = string_new();

	string_append(&pathNuevoPokemonACopiar, entrenadorActual->path);
	string_append(&pathNuevoPokemonACopiar, "/Entrenadores/");
	string_append(&pathNuevoPokemonACopiar, entrenadorActual->nombre);
	string_append(&pathNuevoPokemonACopiar, "/Dir de Bill/");
	string_append(&pathNuevoPokemonACopiar, rutas[indicePartes]);
	string_append(&pathNuevoPokemonACopiar, "001.dat");

	copy_file(rutaPokeMonEnMapa, pathNuevoPokemonACopiar);

}

void copiarArchivoDeMedalla(char* pathMedalla) {

	char** rutas = string_split(pathMedalla, "/");
	int indicePartes = 0;

	while (rutas[indicePartes] != NULL) {
		indicePartes++;
	}

	char* pathNuevaMedalla = string_new();

	string_append(&pathNuevaMedalla, entrenadorActual->path);
	string_append(&pathNuevaMedalla, "/Entrenadores/");
	string_append(&pathNuevaMedalla, entrenadorActual->nombre);
	string_append(&pathNuevaMedalla, "/medallas/");
	string_append(&pathNuevaMedalla, rutas[--indicePartes]);

	copy_file(pathMedalla, pathNuevaMedalla);

}

void fallecerPorDeadlock(int mapaSocket, t_actividadEntrenador* actividad) {

	printf("\nCausa de muerte: Deadlock\n");

	close(mapaSocket);

	if (entrenadorActual->vidas > 0) {

		handShake(mapaActual, actividad);
		entrenadorActual->vidas--;

	} else {

		reiniciarJuego();

	}

}

void borrarPokemonesCapturados() {

	char* pathDirBill = string_new();

	string_append(&pathDirBill, entrenadorActual->path);
	string_append(&pathDirBill, "/Entrenadores/");
	string_append(&pathDirBill, entrenadorActual->nombre);
	string_append(&pathDirBill, "/Dir de Bill/");

	DIR *directorio;
	struct dirent *archivo;
	directorio = opendir(pathDirBill);

	int retBorrado = 0;

	if (directorio != NULL) {

		while (archivo = readdir(directorio)) {

			if (string_ends_with(archivo->d_name, ".dat")) {

				char* pathPokemonEspecifico = string_new();

				string_append(&pathPokemonEspecifico, pathDirBill);
				string_append(&pathPokemonEspecifico, archivo->d_name);

				retBorrado = remove(pathPokemonEspecifico);

				if (retBorrado == 0) {
					printf("Se borro el recurso: %s", archivo->d_name);
					printf(" del path: %s\n", pathPokemonEspecifico);
				} else {
					printf("Error: unable to delete");
				}

				free(pathPokemonEspecifico);
			}
		}
	} else {
		printf("No se pudo abrir el directorio.");
	}

	free(pathDirBill);

}

void borrarMedallasObtenidas() {

	char* pathDirMedallas = string_new();

	string_append(&pathDirMedallas, entrenadorActual->path);
	string_append(&pathDirMedallas, "/Entrenadores/");
	string_append(&pathDirMedallas, entrenadorActual->nombre);
	string_append(&pathDirMedallas, "/medallas/");

	DIR *directorio;
	struct dirent *archivo;
	directorio = opendir(pathDirMedallas);

	int retBorrado = 0;

	if (directorio != NULL) {

		while (archivo = readdir(directorio)) {

			if (string_ends_with(archivo->d_name, ".jpg")) {

				char* pathMedallaEspecifica = string_new();

				string_append(&pathMedallaEspecifica, pathDirMedallas);
				string_append(&pathMedallaEspecifica, archivo->d_name);

				retBorrado = remove(pathMedallaEspecifica);

				if (retBorrado == 0) {
					printf("Se borro el recurso: %s", archivo->d_name);
					printf(" del path: %s\n", pathMedallaEspecifica);
				} else {
					printf("Error: unable to delete");
				}

				free(pathMedallaEspecifica);
			}
		}
	} else {
		printf("No se pudo abrir el directorio.");
	}

	free(pathDirMedallas);

}

void reiniciarHojaDeViaje() {

	INDEX_PROXIMO_MAPA = 0;
	int i;
	int cantidadMapas = list_size(entrenadorActual->mapas);
	t_mapa* unMapa;

	for (i = 0; i < cantidadMapas; i++) {

		unMapa = list_get(entrenadorActual->mapas, i);
		unMapa->completado = 0;

	}

	aventuraEntrenador();

}

void reiniciarJuego() {

	char* kk = string_new();
	char* si = string_new();
	string_append(&si, "s");
	char * respuesta = malloc(2);

	char* no = string_new();
	string_append(&no, "n");

	printf("¿Desea reiniciar el juego? S/N:  \n");

	do {

		scanf("%s", kk);

		string_append(&respuesta, kk);

		if (string_equals_ignore_case(respuesta, si)) {

			entrenadorActual->intentos++;

			borrarPokemonesCapturados();

			borrarMedallasObtenidas();

			reiniciarHojaDeViaje();

		} else if (string_equals_ignore_case(respuesta, no)) {

			printf("El entrenador %s ha salido del juego.\n",
					entrenadorActual->nombre);

		} else {

			printf("¿¿¿Desea reiniciar el juego??? S/N \n");

		}

	} while (!string_equals_ignore_case(respuesta, si)
			&& !string_equals_ignore_case(respuesta, no));

	free(si);
	free(no);
	free(respuesta);

}
