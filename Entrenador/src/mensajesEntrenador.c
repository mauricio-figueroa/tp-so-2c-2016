#include "mensajesEntrenador.h"
#include "Entrenador.h"

int contador = 0;

void conocerPokenest(int idSocket, char razaPokemon, punto* cordenadaPokenest) {
	t_mensajes_entrenador mensaje = CONOCER_POKENEST;

	librocket_enviarMensaje(idSocket, &mensaje, 4, errorLogger);
	librocket_enviarMensaje(idSocket, &razaPokemon, 1, errorLogger);

	recv(idSocket, cordenadaPokenest, sizeof(punto), 0);
}
;

void moverPosicionEntrenador(int idSocket, int direccion) {

	contador++;
	printf("%d \n", contador);
	t_mensajes_entrenador mensaje = MOVER_POSICION;

	librocket_enviarMensaje(idSocket, &mensaje, 4, errorLogger);
	librocket_enviarMensaje(idSocket, &direccion, 4, errorLogger);

	p_mensajes_mapa mensaje2;

	recv(idSocket, &mensaje2, 4, 0);

}
;

int capturarPokemon(int idSocket, char idPokemon) {

	t_mensajes_entrenador mensaje = CAPTURAR;
	char* pokepath;
	int longitudPath;
	int longitudDirectorioPadre;
	char * directorioPadre;

	librocket_enviarMensaje(idSocket, &mensaje, sizeof(CAPTURAR), errorLogger);
	librocket_enviarMensaje(idSocket, &idPokemon, 1, errorLogger);

	//EN ESTE MOMENTO PUEDE RECIBIR TE DOY EL POKEMON, O ESTAS BLOQUEADO
	//-1 aca tenes tu pokemon
	//-2 estas bloqueado
	int accion;
	recv(idSocket, &accion, sizeof(int), 0);

	switch (accion) {

	//ME ESTA DANDO EL POKEMON
	case -1:
		//
		recv(idSocket, &longitudDirectorioPadre, 4, 0);
		directorioPadre = malloc(sizeof(char) * (longitudDirectorioPadre));
		recv(idSocket, directorioPadre, longitudDirectorioPadre, 0);

		recv(idSocket, &longitudPath, 4, 0);
		pokepath = malloc(sizeof(char) * longitudPath);
		recv(idSocket, pokepath, longitudPath, 0);

		copiarArchivoPokemonDirectorioBill(directorioPadre, pokepath);

		int semaforoArchivo = 0;
		librocket_enviarMensaje(idSocket, &semaforoArchivo, sizeof(int),
				errorLogger);

		free(directorioPadre);
		free(pokepath);

		return 1;
		break;

		// QUEDA BLOQUEADO
	case -2:
		puts("Estoy bloqueado");
		return -2;
		break;
	}

}

void copiarArchivoPokemonDirectorioBill(char * directorioPadre, char * pokepath) {
	char * pathPokemonnEntrenador = string_new();
	char * patPokemonEnMapa = string_new();

	string_append(&pathPokemonnEntrenador, entrenadorActual->path);
	string_append(&pathPokemonnEntrenador, "/Entrenadores/");
	string_append(&pathPokemonnEntrenador, entrenadorActual->nombre);
	string_append(&pathPokemonnEntrenador, "/Dir de Bill/");
	string_append(&pathPokemonnEntrenador, pokepath);

	string_append(&patPokemonEnMapa, entrenadorActual->path);
	string_append(&patPokemonEnMapa, "/Mapas/");
	string_append(&patPokemonEnMapa, mapaActual->nombre);
	string_append(&patPokemonEnMapa, "/PokeNests/");
	string_append(&patPokemonEnMapa, directorioPadre);
	string_append(&patPokemonEnMapa, "/");
	string_append(&patPokemonEnMapa, pokepath);

	copy_file(patPokemonEnMapa, pathPokemonnEntrenador);

}

void notificarFinDeObjetivos(int idSocket) {

	t_mensajes_entrenador mensaje = TERMINE_OBJETIVOS;
	int longitudPath = 0;
	char* pathMedalla;

	librocket_enviarMensaje(idSocket, &mensaje, sizeof(TERMINE_OBJETIVOS),
			errorLogger);

	recv(idSocket, &longitudPath, sizeof(int), 0);
	pathMedalla = malloc(sizeof(char) * longitudPath);
	recv(idSocket, pathMedalla, longitudPath, 0);

	copiarArchivoDeMedalla(pathMedalla);

}
;

int darPokemonMasFuerte(int mapaSocket) {

	int nivel = 0, tamanio, resultadoBatalla;
	char* nombre = string_new();

	nivel = obtenerPokemonMasFuerte(&nombre);

	tamanio = string_length(nombre);
	tamanio++;

	librocket_enviarMensaje(mapaSocket, &nivel, sizeof(int), errorLogger);
	librocket_enviarMensaje(mapaSocket, &tamanio, sizeof(int), errorLogger);
	librocket_enviarMensaje(mapaSocket, nombre, tamanio, errorLogger);

	recv(mapaSocket, &resultadoBatalla, sizeof(int), 0);

	free(nombre);

	return resultadoBatalla;
}

int obtenerPokemonMasFuerte(char** nombre) {

	char* pathDirBill = string_new();
	char* mayorNombre = string_new();

	string_append(&pathDirBill, entrenadorActual->path);
	string_append(&pathDirBill, "/Entrenadores/");
	string_append(&pathDirBill, entrenadorActual->nombre);
	string_append(&pathDirBill, "/Dir de Bill/");

	DIR *directorio;
	struct dirent *archivo;
	struct dirent *archivoPokemonMayorNivel;
	directorio = opendir(pathDirBill);

	int mayorNivel = 0, nivelActual = 0, i = 0;

	if (directorio != NULL) {

		while (archivo = readdir(directorio)) {

			if (string_ends_with(archivo->d_name, ".dat")) {

				char* pathPokemonEspecifico = string_new();

				string_append(&pathPokemonEspecifico, pathDirBill);
				string_append(&pathPokemonEspecifico, archivo->d_name);

				t_config* configPokemon = config_create(pathPokemonEspecifico);
				nivelActual = config_get_int_value(configPokemon, "Nivel");

				if (nivelActual >= mayorNivel) {

					mayorNivel = nivelActual;
					*mayorNombre = '\0';
					string_append(&mayorNombre, archivo->d_name);

				}

				free(pathPokemonEspecifico);
			}
		}

	} else {
		printf("No se pudo abrir el directorio.");
	}

	int tamanioSinDat = string_length(mayorNombre) - string_length("XXX.dat");
	string_append(nombre, string_substring_until(mayorNombre, tamanioSinDat));

	free(mayorNombre);
	free(pathDirBill);

	return mayorNivel;

}
