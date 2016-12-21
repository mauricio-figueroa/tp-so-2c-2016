#include "conectionEntrenador.h"

#define MAX_STRING_SIZE 256

//TODO loguear errores
int handShake(t_mapa* mapa, t_actividadEntrenador* actividad) {
	int mapaSocket;

	INDEX_PROXIMO_POKEMON = 0;

	actividad->estado = SIN_POKENEST;
	actividad->posicionDelEntrenador = malloc(sizeof(punto));
	actividad->posicionDelEntrenador->puntox = 0;
	actividad->posicionDelEntrenador->puntoy = 0;

	int result = libconnections_conectar_a_servidor(mapa->ip,
			string_itoa(mapa->puerto), &mapaSocket);

	if (result == 0) {
		//se conecto al mapa
		printf("Conectado al mapa %s\n", mapa->nombre);
		librocket_enviarMensaje(mapaSocket, entrenadorActual->simbolo, 1,
				errorLogger);

		char* pathEntrenador = string_new();

		string_append(&pathEntrenador, entrenadorActual->path);
		string_append(&pathEntrenador, "/Entrenadores/");
		string_append(&pathEntrenador, entrenadorActual->nombre);

		int longitudPath = string_length(pathEntrenador);
		longitudPath++;

		char * nombreEntrenador = string_new();
		int sizeNombre = string_length(entrenadorActual->nombre);
		string_append(&nombreEntrenador, entrenadorActual->nombre);

		librocket_enviarMensaje(mapaSocket, &longitudPath, sizeof(int),
				errorLogger);

		librocket_enviarMensaje(mapaSocket, pathEntrenador, longitudPath,
				errorLogger);

		char* primerObjetivo = list_get(mapa->objetivos, 0);

		char primerObjetivoChar = primerObjetivo[0];

		librocket_enviarMensaje(mapaSocket, &primerObjetivoChar, sizeof(char),
				errorLogger);

		free(pathEntrenador);

	}else{
		mapaSocket=-1;
		char* string = string_new();
		string_append(&string, "No se pudo conectar a ");
		string_append(&string, mapa->ip);
		string_append(&string, " al puerto ");
		string_append(&string, string_itoa(mapa->puerto));
		printf(string);
	}

	return mapaSocket;
}
