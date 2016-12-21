#include <stdlib.h>
#include <fcntl.h>
#include "headers/logicaFuse.h"
#include <commons/log.h>
#include <commons/string.h>
#include "headers/pokeClienteConections.h"
#include <semaphore.h>
#include "globales.h"




struct fuse_operations operations = {
		.getattr = osada_getattr,
		.readdir = osada_getdir,
		.open = osada_open,
		.read = osada_read,
		.unlink = osada_unlink,
		.write = osada_write,
		.truncate = osada_truncate,
		.mkdir = osada_crearDirectorio,
		.rename = osada_renombrarArchivo,
		.rmdir = osada_borrarDirectorio,
		.create = osada_crearArchivo,
		.utimens = osada_utimens,
		.flush = osada_flush,
		.access = osada_access,
		.chmod = osada_chmod,
		.chown = osada_chown,
};


/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),
		CUSTOM_FUSE_OPT_KEY("--ip %s",IP,0),
		CUSTOM_FUSE_OPT_KEY("--puerto %s", PUERTO,0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS

int main(int argc, char *argv[]) {
 	t_log* errorLogger = log_create("error_logs","pokedex_cliente",false,LOG_LEVEL_INFO);

	//sem_init(&semConexion,0,0);

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}


	/*OBTENGO IP Y PUERTO PASADOS POR PARAMETRO*/
	argumentos_t *argumentos_conexion = malloc(sizeof(argumentos_t));
	if( runtime_options.IP != NULL ){

	argumentos_conexion->IP = malloc(strlen(runtime_options.IP)+1);
	strcpy(argumentos_conexion->IP,runtime_options.IP);
	printf("Puerto: %s\n", argumentos_conexion->IP);

	}else {
		printf("Parametro faltante: --puerto \"XXX\" ");
		exit(1);
	}

	if(runtime_options.PUERTO != NULL){

		argumentos_conexion->PUERTO = malloc(strlen(runtime_options.PUERTO)+1);
			strcpy(argumentos_conexion->PUERTO,runtime_options.PUERTO);
			printf("Puerto: %s\n", argumentos_conexion->PUERTO);
	}else {
		printf("Parametro faltante: --ip \"localhost\"");
	}


	log_destroy(errorLogger);
	pthread_t * hilo = conectarAPokeDexServidor(argumentos_conexion);
	//sem_wait(&semConexion);
	fuse_main(args.argc, args.argv, &operations, NULL);
	//sem_post(&semConexion);


	pthread_join(*hilo,NULL);
	free(hilo);
	free(argumentos_conexion);
	//sem_destroy(&semConexion);


	 return EXIT_SUCCESS;
}





