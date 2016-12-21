#include "conectionMapa.h"
#define PORT 9034

int contadorMovimiento = 0;

//TODO verificar ese result;
//TODO loguear errores
//funcion llamada cuando se acepta la conexion de un nuevo entrenador
void* nuevoEntrenadorConectado(int socketFd, struct sockaddr_in remoteaddr) {


	//printf("Nuevo  entrenador conectado \n");
	t_log* infoLogger = log_create("error_logs", "entrenador", false,
			LOG_LEVEL_INFO);


	char simboloEntrenador;

	int result = recv(socketFd, &simboloEntrenador, 1, 0);

	log_info(infoLogger, "Nuevo entrenador conectado!");

	//Agrego a la cola de listos el socketId
	t_pcb* pcbEntrenador = malloc(sizeof(t_pcb));
	pcbEntrenador->listaRecursos = list_create();
	punto punto;
	punto.puntox = 0;
	punto.puntoy = 0;
	pcbEntrenador->punto.puntox = punto.puntox;
	pcbEntrenador->punto.puntoy = punto.puntoy;
	pcbEntrenador->simboloEntrenador = simboloEntrenador;
	pcbEntrenador->sockfd = socketFd;
	pcbEntrenador->nombre = string_new();
	pcbEntrenador->direccionDesconocida = 1;

	char * pathEntrenador;
	int longitudPath;

	recv(socketFd, &longitudPath, sizeof(int), 0);
	pathEntrenador = malloc(sizeof(char) * longitudPath);
	recv(socketFd, pathEntrenador, longitudPath, 0);
	pcbEntrenador->pathEntrenador = pathEntrenador;

	char** pathEntrenadorVector = string_split(pathEntrenador, "/");

	int i = 0;

	while (pathEntrenadorVector[i] != NULL) {

		i++;

	}

	i--;

	string_append(&pcbEntrenador->nombre, pathEntrenadorVector[i]);

	char primerObjetivo;
	recv(socketFd, &primerObjetivo, sizeof(char), 0);

	pcbEntrenador->proximaPokenest = getPokenestById(primerObjetivo);
	pcbEntrenador->ultimaAccion = SE_CONECTO;

	agregarEntrenadorAListos(listaListos, pcbEntrenador);

	char* dataLogger = string_new();
	string_append(&dataLogger, "Acaba de entrar el entrenador ");
	string_append(&dataLogger, pcbEntrenador->nombre);
	log_info(infoLogger, dataLogger);
	free(dataLogger);
//	log_destroy(infoLogger);

	return NULL;

}

void agregarEntrenadorAListos(t_list* lista, t_pcb* pcbEntrenador) {

	pthread_mutex_lock(&mutexListaListos);

	list_add(listaListos, pcbEntrenador);
	//sem_post(&semPlanificar);// si el tamanio antes de agregar era 0 habilito el planificador

	pthread_mutex_unlock(&mutexListaListos);

	sem_post(&semGui);
	sem_wait(&semEntrenadorYaSeDibujo);

	char* dataLogger = string_new();
	string_append(&dataLogger, "Se agrego al entrenador ");
	string_append(&dataLogger, pcbEntrenador->nombre);
	string_append(&dataLogger, " a la lista de listos");
	log_info(infoLogger, dataLogger);
	free(dataLogger);

}

//todo revisar eliminarSocket,m se conecta un entrenador pero la lista tienew 4 ???
//deberia poder interpretar todos los mensajes por lo cual al no poder interpretar, asumo entrenador desconectado y libero recursos
//funcion llamada cuando un entrenador envia un mensaje
//TODO agregar takeAndRemove y luego add tras procesar el mensaje del entrenador
int mensajeDeEntrenadorRecibido(int sockfd) {
/*
	t_log* infoLogger = log_create("info_logs", "entrenador", false,
			LOG_LEVEL_INFO);
	t_log* errorLogger = log_create("error_logs", "entrenador", false,
			LOG_LEVEL_INFO);
*/
	t_mensajes_entrenador tipoMensaje;
	int quitarListaListos = 0;

	int result = recv(sockfd, &tipoMensaje, sizeof(int32_t), 0);
	//////sem_wait(&semConexion);

	if (result <= 0) {
		log_error(errorLogger, "Error al recibir mensaje del mapa");
	}

	switch (tipoMensaje) {

	case CONOCER_POKENEST:
		quitarListaListos = obtenerPokenest(sockfd);
		sem_post(&semConexion);
		break;

	case MOVER_POSICION:
		quitarListaListos = moverPosicion(sockfd);
		sem_post(&semConexion);
		break;

	case CAPTURAR:
		quitarListaListos = capturarPokemon(sockfd);
		sem_post(&semConexion);
		break;

	case TERMINE_OBJETIVOS:
		darMedalla(sockfd);
		liberarRecursosListos(sockfd);
		eliminarSocketListaListos(sockfd, false);
		//////sem_wait(&semConexion);

		quitarListaListos = 1;
		break;

	default:
		//ENTRENADOR DESCONECTADO LIBERAR RECURSOS
		//puts("Entrenador Desconectado");
		log_info(infoLogger, "Entrenador desconectado");
		liberarRecursosListos(sockfd);
		eliminarSocketListaListos(sockfd, false);
		//sem_wait(&semConexion);
		quitarListaListos = 1;
		close(sockfd);
		break;

	}


	return quitarListaListos;

}

//TODO eliminarlo de la lista de listos y {{cerrar la conexion???}}
void liberarRecursosListos(int sockfd) {

	t_pcb * pcbALiberar = getPcbBySockFdListos(sockfd);

	int cantidadDeRecursos = list_size(pcbALiberar->listaRecursos);
	int i;
	for (i = 0; i < cantidadDeRecursos; i++) {

		char* recursoALiberar = string_new();
		char* recurso;

		recurso = list_get(pcbALiberar->listaRecursos, i);

		string_append(&recursoALiberar, recurso);
		reintegrarRecurso(recurso[0], pcbALiberar);
	}

}

void liberarRecursosBloqueados(t_pcb* pcbALiberar) {

	int cantidadDeRecursos = list_size(pcbALiberar->listaRecursos);
	int i;
	for (i = 0; i < cantidadDeRecursos; i++) {

		char* recursoALiberar = string_new();
		char* recurso;

		recurso = list_get(pcbALiberar->listaRecursos, i);

		string_append(&recursoALiberar, recurso);
		reintegrarRecurso(recurso[0], pcbALiberar);
	}

}

void reintegrarRecurso(char identificadorPokenest, t_pcb * pcbALiberar) {

	t_pokenest * pokenest = getPokenestById(identificadorPokenest);
	char * nombreDirectorioPadre = pokenest->species;
	char * pathDirBill = string_new();

	string_append(&pathDirBill, pcbALiberar->pathEntrenador);
	string_append(&pathDirBill, "/Dir de Bill/");

	DIR *directorio;
	struct dirent *archivo;
	directorio = opendir(pathDirBill);

	if (directorio != NULL) {

		while (archivo = readdir(directorio)) {
			char * pathPokemonEspecifico = string_new();
			char * directorioPokemonMapa = string_new();
			string_append(&directorioPokemonMapa, mapaActual->pathMapa);
			string_append(&directorioPokemonMapa, "/PokeNests/");
			string_append(&directorioPokemonMapa, pokenest->species);
			string_append(&directorioPokemonMapa, "/");

			if (string_starts_with(archivo->d_name, nombreDirectorioPadre)) {
				string_append(&pathPokemonEspecifico, pathDirBill);
				string_append(&pathPokemonEspecifico, archivo->d_name);

				string_append(&directorioPokemonMapa, archivo->d_name);

				copy_file(pathPokemonEspecifico, directorioPokemonMapa);

				int ret = remove(pathPokemonEspecifico);

				if (ret == 0) {
				//	printf("\nSe borro el recurso: %s", archivo->d_name);
					//printf(" del path: %s\n", pathPokemonEspecifico);
				} else {
					//printf("Error: unable to delete");
				}

			}

			free(directorioPokemonMapa);
			free(pathPokemonEspecifico);
		}

		bool pokenestBuscada(t_pokenest *puntero) {
			return (puntero->identificador == identificadorPokenest);
		}

		t_pokenest* pknst = list_find(listaPokenest, (void *) pokenestBuscada);

		pknst->cant++;

	}
}

t_pokenest * getPokenestById(char id) {

	pthread_mutex_lock(&mutexListaPokenest);

	t_pokenest * currentPokenest;
	int i;
	int cantidadListos = list_size(listaPokenest);
	for (i = 0; i < cantidadListos; i++) {
		currentPokenest = list_get(listaPokenest, i);
		if (currentPokenest->identificador == id) {
			i = cantidadListos;
		}
	}

	pthread_mutex_unlock(&mutexListaPokenest);
	t_pokenest* nuevoPokenest = malloc(sizeof(t_pokenest));
	memcpy(nuevoPokenest, currentPokenest, sizeof(t_pokenest));
	return nuevoPokenest;
}

//TODO preguntar si el hilo graficador, consulta la lista de listos constantemente????
int obtenerPokenest(int sockfd) {
/*
	t_log* errorLogger = log_create("error_logs", "entrenador", false,
			LOG_LEVEL_INFO);

*/
	t_pcb* entrenadorActual = getPcbBySockFdListos(sockfd);
	char pokemon;
	if ((recv(sockfd, &pokemon, 1, 0)) <= 0) {
		log_error(errorLogger,
				"error recibiendo el pokemon, CONOCER POKEMON conection.c");
		return 1;
	} else {

		t_pokenest* proximaPokenest = getPokenestById(pokemon);

		entrenadorActual->proximaPokenest = proximaPokenest;

		librocket_enviarMensaje(sockfd,
				&(entrenadorActual->proximaPokenest->coordenada), sizeof(punto),
				errorLogger);

		return 2;

	}
}

int moverPosicion(int sockfd) {

//t_log* errorLogger = log_create("error_logs", "mapa", false,LOG_LEVEL_INFO);
	int direccion;

	int result;
	result = recv(sockfd, &direccion, 4, 0);

	if (result <= 0) {
		log_error(errorLogger,
				"error recibiendo la direccion de movimiento, moverPosicion conecction mapa, se desconectara al entrenador");
		return 1;
	} else {
		t_pcb* currentPcb = getPcbBySockFdListos(sockfd);

		punto punto;

		switch (direccion) {
		case (IZQUIERDA):
			punto.puntox = currentPcb->punto.puntox - 1;
			punto.puntoy = currentPcb->punto.puntoy;
			currentPcb->punto = punto;
			break;
		case (DERECHA):
			punto.puntox = currentPcb->punto.puntox + 1;
			punto.puntoy = currentPcb->punto.puntoy;
			currentPcb->punto = punto;
			break;
		case (ARRIBA):
			punto.puntox = currentPcb->punto.puntox;
			punto.puntoy = currentPcb->punto.puntoy + 1;
			currentPcb->punto = punto;
			break;
		case (ABAJO):
			punto.puntox = currentPcb->punto.puntox;
			punto.puntoy = currentPcb->punto.puntoy - 1;
			currentPcb->punto = punto;
			break;
		}

		currentPcb->ultimaAccion = SE_MOVIO;
		sem_post(&semGui);
		sem_wait(&semEntrenadorYaSeDibujo);
		//puts("Punto");
		//printf("%d \n", punto.puntox);
		//printf("%d \n", punto.puntoy);

		p_mensajes_mapa mensaje = OK;
		librocket_enviarMensaje(sockfd, &mensaje, 4, errorLogger);
		return 0;

	}
}

t_pcb* getPcbBySockFdListos(int sockFd) {

	pthread_mutex_lock(&mutexListaListos);

	t_pcb* currentPcb;
	int i;
	int cantidadListos = list_size(listaListos);
	for (i = 0; i < cantidadListos; i++) {
		currentPcb = list_get(listaListos, i);
		if (currentPcb->sockfd == sockFd) {
			i = cantidadListos;
		}
	}
	pthread_mutex_unlock(&mutexListaListos);
	return currentPcb;

}

t_pcb* getPcbBySockFdBloqueados(int sockFd) {

	pthread_mutex_lock(&mutexListaBloqueados);

	t_pcb* currentPcb;
	int i;
	int cantidadBloqueados = list_size(listaListos);
	for (i = 0; i < cantidadBloqueados; i++) {
		currentPcb = list_get(listaBloqueados, i);
		if (currentPcb->sockfd == sockFd) {
			i = cantidadBloqueados;
		}
	}
	pthread_mutex_unlock(&mutexListaBloqueados);
	return currentPcb;

}

void conocerPokenest(int sockfd) {
	//t_log* errorLogger = log_create("error_logs", "entrenador", false,LOG_LEVEL_INFO);

	char * pokemon = malloc(1);
	if ((recv(sockfd, pokemon, 1, 0)) < 0) {
		log_error(errorLogger,
				"error recibiendo el pokemon, CONOCER POKEMON conection.c");
	}
}

//TODO reubicarPRoceso
//TODO ubicar el pokemon en la lista de recursos del correspondiente pcb, con los datos para la batalla
int capturarPokemon(int sockfd) {

	char pokemon;
	int result, longitud, accion, longitudDirectorioPadre;
	char* pokemonEnEspecifico = string_new();

	t_pcb * pcbActual = getPcbBySockFdListos(sockfd);

	result = recv(sockfd, &pokemon, 1, 0);
	if (result <= 0) {
		log_info(logger, "Error recibiendo el pokemon que quiere capturar");
		return 1;
	} else {
		t_pokenest* pokenest = getPokenestById(pokemon);

		if (pokenest->identificador != pokemon) {
			log_info(logger, "No existe pokenest para ese pokemon");

			return 3;
		}

		if (0 < pokenest->cant) {

			string_append(&pokemonEnEspecifico,
					adquirirPathPokemonEnEspecifico(pokenest));
			longitudDirectorioPadre = string_length(pokenest->species);
			longitud = string_length(pokemonEnEspecifico);

			//Envio Accion de que le voy a dar el pokemon
			accion = -1;

			librocket_enviarMensaje(sockfd, &accion, sizeof(signed int),
					errorLogger);

			longitudDirectorioPadre++;
			librocket_enviarMensaje(sockfd, &longitudDirectorioPadre,
					sizeof(int), errorLogger);

			librocket_enviarMensaje(sockfd, pokenest->species,
					longitudDirectorioPadre, errorLogger);

			//printf("%s", pokenest->species);

			longitud++;
			librocket_enviarMensaje(sockfd, &longitud, sizeof(int),
					errorLogger);
			librocket_enviarMensaje(sockfd, pokemonEnEspecifico, longitud,
					errorLogger);

			bool pokenestBuscada(t_pokenest *puntero) {
				return (puntero->identificador == pokemon);
			}

			t_pokenest* pknst = list_find(listaPokenest,
					(void *) pokenestBuscada);

			pknst->cant--;

			int eliminarArchivoSemaforo;
			result = recv(sockfd, &eliminarArchivoSemaforo, sizeof(int), 0);
			if (result <= 0) {
				return 1;
			}

			char * pokemonString = malloc(2);
			pokemonString[0] = pokemon;
			pokemonString[1] = '\0';

			list_add(pcbActual->listaRecursos, pokemonString);
			pcbActual->direccionDesconocida = 1;

			eliminarArchivo(pokemonEnEspecifico, pokenest->species);

			free(pokemonEnEspecifico);
			return 0;

		} else {

			accion = -2;
			librocket_enviarMensaje(sockfd, &accion, sizeof(signed int),
					errorLogger);

			pthread_mutex_lock(&mutexListaBloqueados);
			t_pcb * currentPcb = getPcbBySockFdListos(sockfd);
			list_add(listaBloqueados, currentPcb);
			sem_post(&semGui);
			sem_wait(&semEntrenadorYaSeDibujo);
			pthread_mutex_unlock(&mutexListaBloqueados);

			return 1;
		}

		// TODO
		//reubicarProcesos(estaBloqueado);
	}
}

char* adquirirPathPokemonEnEspecifico(t_pokenest* pokenest) {

	char* path = string_new();
	string_append(&path, mapaActual->pathMapa);
	string_append(&path, "/PokeNests/");
	string_append(&path, pokenest->species);
	string_append(&path, "/");

	DIR *directorio;
	struct dirent *archivo;
	directorio = opendir(path);
	char* nombreDirectorio = string_new();
	char * pokemonEnEspecifico = string_new();

	if (directorio != NULL) {

		while (archivo = readdir(directorio)) {

			nombreDirectorio = archivo->d_name;

			if (nombreArchivoInvalido(nombreDirectorio)) {

			} else {

				string_append(&pokemonEnEspecifico, nombreDirectorio);
				break;
			}

		}

		(void) closedir(directorio);
	} else {
		perror("Couldn't open the directory");
	}

	return pokemonEnEspecifico;
}

void eliminarArchivo(char * pokemonEnEspecifico, char * directorioPadre) {

	char * patPokemonEnMapa = string_new();

	string_append(&patPokemonEnMapa, mapaActual->pathMapa);
	string_append(&patPokemonEnMapa, "/PokeNests/");
	string_append(&patPokemonEnMapa, directorioPadre);
	string_append(&patPokemonEnMapa, "/");
	string_append(&patPokemonEnMapa, pokemonEnEspecifico);

	int ret = remove(patPokemonEnMapa);

	if (ret == 0) {
	//	printf("File deleted successfully");
	} else {
	//	printf("Error: unable to delete");
	}

}

void cargarPokenest() {

	int file_count = 0;

	char * nombreDirectorio;

	char * path = string_new();

	string_append(&path, mapaActual->pathMapa);
	string_append(&path, "/PokeNests/");

	DIR *dp;
	struct dirent *ep;
	dp = opendir(path);

	if (dp != NULL) {

		while (ep = readdir(dp)) {
			nombreDirectorio = ep->d_name;

			if (nombreArchivoInvalido(nombreDirectorio)) {

			} else {

				file_count++;

				obtenerCantidad(path, nombreDirectorio);

			}

		}

		(void) closedir(dp);
	} else {
		perror("Couldn't open the directory");
	}

}

int nombreArchivoInvalido(char * nombreArchivo) {

	if (!strcmp(nombreArchivo, ".") || !strcmp(nombreArchivo, "..")
			|| !strcmp(nombreArchivo, "metadata")) {
		return 1;
	}
	return 0;

}

void obtenerCantidad(char * pathPokenests, char* nombreDirectorioPokemon) {

	t_pokenest* pokenest = malloc(sizeof(t_pokenest));
	char* identificadorPokemon = string_new();
	char* nombreDirectorio;

	int count = 0;

	char* pathMetadataPokenest = string_new();
	char* pathDirectorioPokenest = string_new();

	pokenest->tipoPokemon = string_new();
	pokenest->species = string_new();

	char * nuevaCoordenada = string_new();

	string_append(&pathDirectorioPokenest, pathPokenests);
	string_append(&pathDirectorioPokenest, "/");
	string_append(&pathDirectorioPokenest, nombreDirectorioPokemon);

	string_append(&pathMetadataPokenest, pathDirectorioPokenest);
	string_append(&pathMetadataPokenest, "/metadata");

	t_config* configMapa = config_create(pathMetadataPokenest);

	string_append(&pokenest->tipoPokemon,
			config_get_string_value(configMapa, "Tipo"));
	string_append(&pokenest->species, nombreDirectorioPokemon);
	string_append(&nuevaCoordenada,
			config_get_string_value(configMapa, "Posicion"));
	string_append(&identificadorPokemon,
			config_get_string_value(configMapa, "Identificador"));
	pokenest->identificador = identificadorPokemon[0];

	pokenest->coordenada = convertirPunto(nuevaCoordenada);

	DIR *dp;
	struct dirent *ep;
	dp = opendir(pathDirectorioPokenest);

	if (dp != NULL) {
		while (ep = readdir(dp)) {
			nombreDirectorio = ep->d_name;

			if (nombreArchivoInvalido(nombreDirectorio)) {

			} else {
				count++;
			}

		}

		pokenest->cant = count;

		list_add(listaPokenest, pokenest);

		(void) closedir(dp);

	} else {
		perror("Couldn't open the directory");
	}

}

punto convertirPunto(char * palabra) {

	char ** arrayPuntos = (char**) malloc(5 * sizeof(char*));

	arrayPuntos = string_split(palabra, ";");

	punto punto;
	punto.puntox = atoi(arrayPuntos[0]);
	punto.puntoy = atoi(arrayPuntos[1]);

	return punto;

}

void darMedalla(int sockfd) {

	char* pathMedalla = string_new();

	string_append(&pathMedalla, mapaActual->pathMapa);
	string_append(&pathMedalla, "/medalla-");
	string_append(&pathMedalla, nombreMapaActual);
	string_append(&pathMedalla, ".jpg");

	int longitudPath = 0;
	longitudPath = string_length(pathMedalla);
	longitudPath++;

	librocket_enviarMensaje(sockfd, &longitudPath, sizeof(int), errorLogger);
	librocket_enviarMensaje(sockfd, pathMedalla, longitudPath, errorLogger);

	free(pathMedalla);

}

void eliminarSocketListaListos(int sock, bool pasoABloqueado) {
	pthread_mutex_lock(&mutexListaListos);
	int sizeList = list_size(listaListos);
	int i;

	if(sizeList==0){
		return;
	}

	for (i = 0; i < sizeList; i++) {
		t_pcb *currentPCB = list_get(listaListos, i);

		if (currentPCB->sockfd == sock) {
			if (pasoABloqueado) {
				currentPCB->ultimaAccion = SE_BLOQUEO;
			} else {
				currentPCB->ultimaAccion = SE_DESCONECTO;
			}
			sem_post(&semGui);
			sem_wait(&semEntrenadorYaSeDibujo);
			list_remove(listaListos, i);
			break;
		}

	}
	pthread_mutex_unlock(&mutexListaListos);

}

void eliminarSocketListaBloqueados(int sock) {

	int sizeList = list_size(listaBloqueados);
	int i;

	pthread_mutex_lock(&mutexListaBloqueados);
	for (i = 0; i < sizeList; i++) {
		t_pcb *currentPCB = list_get(listaBloqueados, i);

		if (currentPCB->sockfd == sock) {

			//sem_post(&semGui);
			//sem_wait(&semEntrenadorYaSeDibujo);
			list_remove(listaBloqueados, i);
			break;
		}

	}
	pthread_mutex_unlock(&mutexListaBloqueados);

}
