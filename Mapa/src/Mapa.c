#include "Mapa.h"
#include "commons/log.h"
#include "conectionMapa.h"

#define LOGFILE "planificacion.log"

int main(int argc, char *argv[]) {

	errorLogger = log_create("error_logs", "mapa", false, LOG_LEVEL_INFO);

	mapaActual = malloc(sizeof(t_mapa));

	logger = log_create("planificacion_log", "Mapa", false, LOG_LEVEL_ERROR);
	infoLogger = log_create("info_logs", "Mapa", false, LOG_LEVEL_INFO);

	inicializarVariablesGlobales();

	cargarConfiguracionMapa(argc, argv);

	//printf("PATH: %s\n", mapaActual->pathMapa);

	// Hilo Deadlock;
	pthread_t hiloDeadLock;
	crearHilo(&hiloDeadLock, (void*) alarmaInterbloqueo, NULL);

	/*ESCUCHO CONEXIONES Y MENSAJES DE ENTRENADORES CON SELECT()*/
	// Hilo Conexiones;
	pthread_t hiloConexiones;
	crearHilo(&hiloConexiones, (void*) levantarHiloConexiones, NULL);

	// Hilo Planificacion;
	pthread_t hiloPlanif;
	crearHilo(&hiloPlanif, (void*) planificate, NULL);

	// Hilo Señales
	pthread_t hiloSignals;
	crearHilo(&hiloSignals, (void*) senialesMapa, NULL);

	//pthread_t gui;
	//crearHilo(&gui,(void*) dibujarGUI,NULL);

	/*ESPERO QUE FINALICEN LOS HILOS*/
	//Falta hilo gui
	pthread_join(hiloConexiones, NULL);
	pthread_join(hiloPlanif, NULL);
	pthread_join(hiloSignals, NULL);
	pthread_join(hiloDeadLock, NULL);

	destroy_pkmn_factory(pokemon_factory);
	free(mapaActual);
	return EXIT_SUCCESS;
}

void inicializarVariablesGlobales() {

	listaListos = list_create();
	listaBloqueados = list_create();
	listaPokenest = list_create();
	sem_init(&semGui, 0, 1);
	sem_init(&semConexion, 0, 1);
	sem_init(&semEntrenadorYaSeDibujo, 0, 0);
	sem_init(&semPlanificar, 0, 0);
	pokemon_factory = create_pkmn_factory();
	itemsDeGui = list_create(); //usada para cuando se conecten los entrenadores

}

void levantarHiloConexiones() {

	librocket_levantarServidorMultiplexado(mapaActual->puerto,
			(void*) nuevoEntrenadorConectado);

}

void cargarConfiguracionMapa(int argc, char** argv) {

	//Aviso por pantalla la falta de parametros
	if (argc < 2) {

		printf("Pasar por parametro nombre del mapa y ruta del FS, Saliendo...\n");
		log_info(infoLogger,
				"Pasar por parametro nombre del mapa y ruta del FS, Saliendo...\n");

		exit(1);
	}

	t_config* configMapa = crearConfigMapa(argv);

	setMapa(configMapa, mapaActual);

	config_destroy(configMapa);

}

t_config* crearConfigMapa(char *argv[]) {

	nombreMapaActual = string_new();
	string_append(&nombreMapaActual, argv[1]);

	mapaActual->pathMapa = string_new();
	mapaActual->pathToPuntoMontaje = string_new();

	string_append(&mapaActual->pathToPuntoMontaje, argv[2]);
	string_append(&mapaActual->pathMapa, argv[2]);
	string_append(&mapaActual->pathMapa, "/Mapas/");
	string_append(&mapaActual->pathMapa, argv[1]);

	char* pathToMetadata = string_new();
	string_append(&pathToMetadata, argv[2]);
	string_append(&pathToMetadata, "/Mapas/");
	string_append(&pathToMetadata, nombreMapaActual);
	string_append(&pathToMetadata, "/metadata");

	mapaActual->pathToMetadata = string_new();
	string_append(&mapaActual->pathToMetadata, pathToMetadata);

	t_config* configMapa = config_create(mapaActual->pathToMetadata);

	free(pathToMetadata);
	return configMapa;
}

void setMapa(t_config* configMapa, t_mapa* unMapa) {

	if (configMapa == NULL) {

		log_error(errorLogger, "El configMapa pasado a setMapa es nulo");
		//printf("El configMapa pasado a setMapa es nulo\n");
		perror("error:");
		return;

	}

	char * algoritmo = string_new();
	unMapa->IP = string_new();
	unMapa->puerto = string_new();
	unMapa->batalla = config_get_int_value(configMapa, "Batalla");
	unMapa->quantum = config_get_int_value(configMapa, "quantum");
	unMapa->retardo = config_get_int_value(configMapa, "retardo");
	unMapa->tiempoChequeoDeadlock = config_get_int_value(configMapa,
			"TiempoChequeoDeadlock");

	retardoDeadlock = mapaActual->tiempoChequeoDeadlock / 10000;
	retardoPlanificacionMapa = unMapa->retardo / 10000;

	string_append((&unMapa->IP), config_get_string_value(configMapa, "IP"));
	string_append((&unMapa->puerto),
			config_get_string_value(configMapa, "Puerto"));
	string_append(&algoritmo, config_get_string_value(configMapa, "algoritmo"));

	impirmirConfiguracion(algoritmo);

	if (!strcmp(algoritmo, "RR")) {
		unMapa->algoritmo = 1;
	}
	if (!strcmp(algoritmo, "SRDF")) {
		unMapa->algoritmo = 2;
	}

	cargarPokenest();

	/*INICIO LA INTERFAZ GRAFICA EN UN HILO */
	pthread_t hiloGUI;
	crearHilo(&hiloGUI, iniciarGUI, NULL);
	//puts("Termino recarga de configuración");
}

void impirmirConfiguracion(char * algoritmo) {

	char* dataLogger = string_new();
	string_append(&dataLogger, "Algoritmo: ");
	string_append(&dataLogger, algoritmo);
	string_append(&dataLogger, " Quantum: ");
	string_append(&dataLogger, itos(mapaActual->quantum));
	string_append(&dataLogger, " Tiempo de chequeo de Deadlock: ");
	string_append(&dataLogger, itos(mapaActual->tiempoChequeoDeadlock));
	string_append(&dataLogger, " Tiempo de retardo: ");
	string_append(&dataLogger, itos(mapaActual->retardo));

	log_info(infoLogger, dataLogger);
	free(dataLogger);

}

char* itos(int numero) {
	char* cadena = malloc(sizeof(char) * 15);
	sprintf(cadena, "%i", numero);
	return cadena;

}

void planificate() {

	while (1) {

	//	puts("Planificando");

		//sem_wait(&semPlanificar);
		int sizeListaListos;

		pthread_mutex_lock(&mutexListaListos);
		sizeListaListos = list_size(listaListos);
		pthread_mutex_unlock(&mutexListaListos);

		if (sizeListaListos > 0) {

			switch (mapaActual->algoritmo) {

			case 1:
				log_info(infoLogger, "Cambio de proceso");
				//	puts("RoundRobin")
				roundRobinAlgorithm();

				break;

			case 2:
			//	puts("SRDF");
				log_info(infoLogger, "Cambio de proceso");
				srdfAlgorithm();
				//sem_post(&semConexion);
				break;

			default:
			//	puts("No hay asignado ningun algoritmo de planificación");
				log_info(logger,
						"No hay asignado ningun algoritmo de planificacion");

			}
		}
		sleep(retardoPlanificacionMapa);
	}
}

void roundRobinAlgorithm() {
	int result;
	int flag = 0;

	pthread_mutex_lock(&mutexListaListos);
	procesoCorriendo = list_get(listaListos, 0);
	char * processLog= string_new();

	string_append(&processLog, "El proceso corriendo es: ");
	string_append(&processLog, procesoCorriendo->nombre);
	log_info(infoLogger, processLog);

	pthread_mutex_unlock(&mutexListaListos);

	int remain = mapaActual->quantum;
	int primerTurno = 1;

	while (remain != 0 && flag == 0) {

		result = mensajeDeEntrenadorRecibido(procesoCorriendo->sockfd);

		if (result == 1) { //se bloqueo o desconecto
			remain = 0;
			flag = 1;
		}

		if (result == 2) { //termino sus objetivos

			if (primerTurno == 1) {
				//	sem_post(&semPlanificar);
				primerTurno = 0;
			}

			break;
		}
		remain--;

	}

	if (result == 1) {
		eliminarSocketListaListos(procesoCorriendo->sockfd, true);
	}
	if (result == 0) { //termino por quantum
		//GASTO TODO SU QUANTUM, LO SACO DE LA COLA Y LO ENCOLO AL FINAL
		pthread_mutex_lock(&mutexListaListos);
		list_remove(listaListos, 0);
		list_add(listaListos, procesoCorriendo);
		//sem_post(&semPlanificar);
		pthread_mutex_unlock(&mutexListaListos);
	}

	int size = list_size(listaListos);

	if (size != 0) {
		//sem_post(&semPlanificar);
	}

}

void srdfAlgorithm() {

	int result = 0;

	t_pcb* entrenadorElegido;
	t_list* listaDistancias = list_create();

	listaDistancias = list_map(listaListos, (void*) calcularDistancia);

	int indexDistancias;
	int indexMenor = 0;
	int tamanioListaDistancias = list_size(listaDistancias);

	//existe entrenador que no conoce su distancia?

	t_list* entrenadoresSinDireccion = list_filter(listaListos,
			(void*) direccionDesconocida);

	if (list_is_empty(entrenadoresSinDireccion)) {

		for (indexDistancias = 0; indexDistancias < tamanioListaDistancias;
				indexDistancias++) {

			int distanciaIndex = (int) list_get(listaDistancias,
					indexDistancias);
			int distanciaMenor = (int) list_get(listaDistancias, indexMenor);

			if (distanciaIndex <= distanciaMenor) {

				indexMenor = indexDistancias;

			}
		}

		entrenadorElegido = list_get(listaListos, indexMenor);

	} else {

		entrenadorElegido = list_get(entrenadoresSinDireccion, 0);

	}

	result = mensajeDeEntrenadorRecibido(entrenadorElegido->sockfd);

	if (result == 2) {
		result = mensajeDeEntrenadorRecibido(entrenadorElegido->sockfd);
		entrenadorElegido->direccionDesconocida = 0;
	}



	int size = list_size(listaListos);

	if (size != 0) {
		//sem_post(&semPlanificar);
	}

}

bool direccionDesconocida(t_pcb* entrenador) {

	return entrenador->direccionDesconocida;

}

int calcularDistancia(t_pcb* pcb) {

	punto puntoActualEntrenador = pcb->punto;
	punto puntoProximaPokenest = pcb->proximaPokenest->coordenada;

	int distancia = 0;
	distancia = (puntoProximaPokenest.puntox - puntoActualEntrenador.puntox)
			+ (puntoProximaPokenest.puntoy - puntoActualEntrenador.puntoy);

	return distancia;
}

void senialesMapa() {

	if (signal(SIGUSR2, sig_handler) == SIG_ERR) {

		log_error(errorLogger, "No se pudo catchear la señal SIGUSR2");

	}
}

void sig_handler(int signo) {

	if (signo == SIGUSR2) {
	//	printf("Señal recibida SIGUSR2\n");
		log_info(infoLogger, "Señal recibida SIGUSR2\n");
		recargarConfiguracionMetadataMapa();

	} else {

		fprintf(stdout, "Señal recibida %d\n", signo);
		log_info(infoLogger, "Señal recibida %d\n");

	}

}

void recargarConfiguracionMetadataMapa() {

	t_config* configMapa = config_create(mapaActual->pathToMetadata);

	setMapa(configMapa, mapaActual);

	//puts("Configuracion recargada mapa\n");

	log_info(infoLogger, "Configuracion recargada en Mapa");


}

void logFile(char *message) {

	FILE* f;
	bool logCreated = true;

	if (!logCreated) {
		f = fopen(LOGFILE, "w");
		logCreated = true;
	} else {
		f = fopen(LOGFILE, "a+");

		if (f == NULL) {
			if (logCreated)
				logCreated = false;
			return;
		} else {
			fputs(message, f);
			fclose(f);
		}

		if (f)
			fclose(f);
	}
}

void* alarmaInterbloqueo() {

	while (1) {

		detectarDeadLock(NULL);
		sleep(retardoDeadlock);

	}

}

void* iniciarGUI(void * args) {



	int rows, cols;

	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

	int xCenter = cols / 2;
	int yCenter = rows / 2;

	int i = 0;
	t_pokenest* pokenest = list_get(listaPokenest, i);
	while (i < list_size(listaPokenest) && pokenest != NULL) {
		CrearCaja(itemsDeGui, pokenest->identificador,
				pokenest->coordenada.puntox, pokenest->coordenada.puntoy,
				pokenest->cant);
		i++;
		pokenest = list_get(listaPokenest, i);

	}

	char* nombreMapa = string_new();
	string_append(&nombreMapa, "Mapa ");
	string_append(&nombreMapa, (mapaActual->pathMapa)); //cambiar a solo nombre
	nivel_gui_dibujar(itemsDeGui, nombreMapa);
	free(nombreMapa);

	dibujarGUI(); //va a estar a la espera de cambios en la lista de entrenadores

}

//se hara un post por cada cambio en la lista de bloqueados y de listo
//se redibujara el mapa en cada cambio
void dibujarGUI() {

	log_info(infoLogger, "dibujado de GUI activado");

	while (1) {
		sem_wait(&semGui);
		log_info(infoLogger, "el semaforo semGui habilito el dibujo de GUI");
		int i = 0;
		t_pcb* entrenador = list_get(listaListos, i);
		bool huboCambios = false;
		while (entrenador != NULL && i < list_size(listaListos)) {

			if (entrenador->ultimaAccion == SE_CONECTO) {
				log_info(infoLogger,
						"Muestro por pantalla entrenador recien conectado");

				entrenador->ultimaAccion = NADA;
				CrearPersonaje(itemsDeGui, entrenador->simboloEntrenador,
						entrenador->punto.puntox, entrenador->punto.puntoy);
				huboCambios = true;
			}

			else if (entrenador->ultimaAccion == SE_MOVIO) {
				log_info(infoLogger, "Muevo un entrenador");
				entrenador->ultimaAccion = NADA;
				MoverPersonaje(itemsDeGui, entrenador->simboloEntrenador,
						entrenador->punto.puntox, entrenador->punto.puntoy);
				huboCambios = true;
			}

			else if (entrenador->ultimaAccion == SE_DESCONECTO) {
				log_info(infoLogger, "Un entrenador se desconecto");
				entrenador->ultimaAccion = NADA;
				BorrarItem(itemsDeGui, entrenador->simboloEntrenador);
				huboCambios = true;
			}

			else if (entrenador->ultimaAccion == SE_BLOQUEO) {
				entrenador->ultimaAccion = NADA;
				BorrarItem(itemsDeGui, entrenador->simboloEntrenador);
				huboCambios = true;
			}

			i++;
			entrenador = list_get(listaListos, i);

		}

		//recorro los bloqueados
		i = 0;
		entrenador = list_get(listaBloqueados, i);
		while (entrenador !=NULL && i < list_size(listaBloqueados)) {


			if (entrenador->ultimaAccion == NADA) {
				CrearEnemigo(itemsDeGui, entrenador->simboloEntrenador,
						entrenador->punto.puntox, entrenador->punto.puntoy);
				entrenador->ultimaAccion = NADA;
				huboCambios = true;
			} else if (entrenador->ultimaAccion == SE_DESBLOQUEO) {
				BorrarItem(itemsDeGui, entrenador->simboloEntrenador);
				huboCambios = true;
			}
			i++;
			entrenador = list_get(listaListos, i);

		}
		if (huboCambios) {
			nivel_gui_dibujar(itemsDeGui, mapaActual->pathMapa);
			sem_post(&semEntrenadorYaSeDibujo);
		}

	}

}
