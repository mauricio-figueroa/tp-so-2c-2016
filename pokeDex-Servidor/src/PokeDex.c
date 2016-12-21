#include <stdio.h>
#include <stdlib.h>
#include "osada/LectorOsada.h"
#include "osada/OsadaTest.h"
#include <commons/string.h>
#include "conexiones.h"
#include <protocolos/p_pokeDex.h>
#include "osada/FuncionesOsada.h"
#include <protocolos/p_entrenador.h>
#include "globales.h"
#include <pthread.h>
#include <connections/networkHandler.h>

#define PUERTO "4000"


void inicializarLocks();
void loguearCantidadDeArchivos(FILE * fs);


int main(void) {
	puts("!!!Hello PokeDex-Servidor!!!"); /* prints !!!Hello World!!! */


	inicializarLocks();

	librocket_levantarServidorConMultiplesHilos(PUERTO,hiloManejarConexiones);

//	probarMapaDeBits();
//	testearEscribirDirectorio();
//	testearEscrituraArchivoConDirectorios();
//	renombrarArchivoTest();
//	editarArchivoAMasDeSuContenido();
//	osada_leerArchivo("/directorio/128.txt", 129, 0, NULL);
//	recorrerFileSystem();
//	truncarArchivoTest();

	return EXIT_SUCCESS;
}


void inicializarLocks(){
	int i = 0;
	locks = malloc(sizeof(pthread_rwlock_t) * 2048);


	pthread_mutex_init(&lockBitmap,NULL);
	for(;i<2048;i++){
		int result = pthread_rwlock_init(locks + i,NULL);
		if(result != 0) perror("error inicializando rwlock");
	}
	pthread_mutex_init(&obtenerEntradaArchivoLibreMutex,NULL);
	pthread_mutex_init(&obtenerbloqueLibreMutex,NULL);
	pthread_mutex_init(&editarBitEnBitmapsMutex,NULL);
	pthread_mutex_init(&mutexAsignacion,NULL);
}


void loguearCantidadDeArchivos(FILE * fs){
	int cant = obtenerCantidadDeArchivos(fs);
	t_log* logger = log_create("info_logs","pokedex_servidor",false,LOG_LEVEL_INFO);

	char* str = malloc(200);
	sprintf(str,"Pokedex Server iniciado, hay %d archivos\n",cant);
	printf("%s",str);

	log_info(logger,str);
	free(str);
	log_destroy(logger);
}








/**

 */


