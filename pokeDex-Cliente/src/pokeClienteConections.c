#include "headers/pokeClienteConections.h"
#include <connections/cliente.h>
#include  <commons/log.h>
#include <commons/config.h>
#include <connections/networkHandler.h>
#include <connections/cliente.h>
#include <commons/log.h>
#include <protocolos/p_pokeDex.h>
#include <threads/threads.h>
#include <semaphore.h>
#include "globales.h"
#include <sys/types.h>

//Logger
t_log* logger;

//IP DEL POKE DEX SERVIDOR
int socket_servidor;

//Esta funcion se invoca para conectarse al servidor
pthread_t * conectarAPokeDexServidor(argumentos_t* argumentos) {
	logger = log_create("info_logs", "pokedex_cliente", false, LOG_LEVEL_INFO);
	pthread_t * hiloConexion = malloc(sizeof(pthread_t));
	crearHilo(hiloConexion, funcionHiloConexion, (void*) argumentos);

	return hiloConexion;

}

//Funcion interna del file llamada desde el hilo
//retorna el socket de conexion
//si hubo un error retorna -1
int conectarAservidor(char * IP, char* PUERTO) {
	int result = libconnections_conectar_a_servidor(IP, PUERTO,
			&socket_servidor);

	if (!result)
		return socket_servidor;
	else
		return -1;
}

//Recibe un argumentos_t
void* funcionHiloConexion(void* argumentos) {

	argumentos_t * args = (argumentos_t*) argumentos;

	int i = conectarAservidor(args->IP, args->PUERTO);
	if (i == -1) {
		printf("No me pude conectar al servidor");
		fflush(stdout);
		exit(1);
	} else {
		socket_servidor = i;

		printf("Conectado al PokeDex Servidor\n");
		//sem_post(&semConexion);
		fflush(stdout);

	}
}

bool abrirArchivo(char* path, struct fuse_file_info *fi) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeAbrir = ABRIR_ARCHIVO;

	uint32_t pathSize = strlen(path) + 1;
	checkStringEnd(path, pathSize);
	librocket_enviarMensaje(socket_servidor, &mensajeAbrir, 4, logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, 4, logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	//send(socket_servidor, &mensajeAbrir,4,0);//mando el header
	//send(socket_servidor,&pathSize,4,0);
	//send(socket_servidor,path,pathSize,0);

	int32_t existe;
	int result = librocket_recibirMensaje(socket_servidor, &existe,
			sizeof(int32_t), logger);
	checkConexionActiva(result, "abrirArchivo");

	if (existe == 0)
		return true;
	else
		return false;
}

int leerArchivo(size_t size, off_t offset, char * path, char * buf) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeLeer = LEER_ARCHIVO;
	uint32_t pathSize = strlen(path) + 1;
	checkStringEnd(path, pathSize);
	//hago una transformacion del offset a int porque no se por que en el servidor el sizeof es 4 y aca8
	int32_t off = (int32_t) offset;

	librocket_enviarMensaje(socket_servidor, &mensajeLeer, 4, logger);
	librocket_enviarMensaje(socket_servidor, &size, sizeof(size_t), logger);
	librocket_enviarMensaje(socket_servidor, &off, sizeof(int32_t), logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	/*send(socket_servidor, &mensajeLeer,4,0);
	 send(socket_servidor,&size,sizeof(size_t),0); //bytes que quiero leer
	 send(socket_servidor,&off,sizeof(int32_t),0); //desde que byte quiero leer
	 send(socket_servidor,&pathSize,sizeof(uint32_t),0);
	 send(socket_servidor,path,pathSize,0);*/

	char * texto = malloc(size);

	int result = librocket_recibirMensaje(socket_servidor, texto, size, logger);
	checkConexionActiva(result, "leerArchivo");

	if (result > 0) {
		memcpy(buf, texto, size);
	}
	free(texto);
	return 1;

}

int32_t escribirArchivo(size_t size, off_t offset, char * path, char * buf) {
	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeEscribir = ESCRIBIR_ARCHIVO;
	uint32_t pathSize = strlen(path) + 1;
	uint32_t off = (uint32_t) offset;

	librocket_enviarMensaje(socket_servidor, &mensajeEscribir, 4, logger);
	librocket_enviarMensaje(socket_servidor, &size, sizeof(size_t), logger);
	librocket_enviarMensaje(socket_servidor, &off, sizeof(uint32_t), logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);
	librocket_enviarMensaje(socket_servidor, buf, size, logger);

	/*send(socket_servidor, &mensajeEscribir,4,0);
	 send(socket_servidor,&size,sizeof(size_t),0);
	 send(socket_servidor,&off,sizeof(uint32_t),0);
	 send(socket_servidor,&pathSize, sizeof(uint32_t),0);
	 send(socket_servidor,path,pathSize,0);
	 send(socket_servidor,buf, size,0); //datos a escribir*/

	int32_t bytesEscritos;
	int result = librocket_recibirMensaje(socket_servidor, &bytesEscritos,
			sizeof(int32_t), logger);
	checkConexionActiva(result, "escribirArchivo");

	return bytesEscritos;

}

int truncarArchivo(const char* path, off_t offset) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensaje = TRUNCAR_ARCHIVO;
	uint32_t pathSize = strlen(path) + 1;

	librocket_enviarMensaje(socket_servidor, &mensaje, 4, logger);
	librocket_enviarMensaje(socket_servidor, &offset, sizeof(uint32_t), logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	int32_t creado;
	librocket_recibirMensaje(socket_servidor, &creado, sizeof(int32_t), 0);

	if (creado == -1) {
		log_error(logger, "Ocurrio un error al truncar el archivo ");
	}
	if (creado == 1)
		return 0;
	else
		return creado;

}

int32_t borrarArchivo(const char* path) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeBorrar = ELIMINAR_ARCHIVO;
	uint32_t pathSize = strlen(path) + 1;

	librocket_enviarMensaje(socket_servidor, &mensajeBorrar, 4, logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, 4, logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	int32_t creado;
	librocket_recibirMensaje(socket_servidor, &creado, sizeof(int32_t), logger);

	if (creado == -1) {
		log_error(logger, "Ocurrio un error al eliminar el archivo");
	}
	return creado;

}

/*Tiene que recibir la lista de archivos y directorios que se encuentran en
 * el directorio indicado
 * No se por que carajos me aparecen cosas en rojo pero se compila y ejecuta sin problema
 * */
void getDir(const char *path, void *buf, fuse_fill_dir_t filler) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeLeerDirectorio = GET_DIRECTORIO;
	uint32_t pathSize = strlen(path) + 1;

	checkStringEnd(path, pathSize);
	printf("Mando GET DIR con path %s y %d bytes \n", path, pathSize);

	librocket_enviarMensaje(socket_servidor, &mensajeLeerDirectorio, 4, logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, 4, logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	/*send(socket_servidor, &mensajeLeerDirectorio,4,0);
	 send(socket_servidor,&pathSize,4,0);
	 send(socket_servidor,path,pathSize,0);*/

	MSG_FS_SERVER_TO_CLIENT mensajeRecibido;

	int result = librocket_recibirMensaje(socket_servidor, &mensajeRecibido,
			sizeof(MSG_FS_SERVER_TO_CLIENT), logger);
	checkConexionActiva(result, "getDir");

	if (mensajeRecibido != ENVIO_DATA_DIRECTORIO) {
		log_error(logger, "Esperaba un mensaje de tipo ENVIO_DATA_DIRECTORIO");
		return;
	}

	int32_t cantidadElementos;
	result = librocket_recibirMensaje(socket_servidor, &cantidadElementos,
			sizeof(int32_t), logger);
	checkConexionActiva(result);

	int i = 0;
	for (; i < cantidadElementos; i++) {
		char nombre[17];
		tipo_archivo_t tipoArchivo;

		result = librocket_recibirMensaje(socket_servidor, &nombre, 17, logger);
		checkConexionActiva(result);

		result = librocket_recibirMensaje(socket_servidor, &tipoArchivo,
				sizeof(tipo_archivo_t), logger);
		checkConexionActiva(result);

		filler(buf, nombre, NULL, 0);

	}

}

//pido los atributos de un archivo
bool getAttr(const char *path, struct stat *stbuf) {
	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeGetAttr = GET_ATTR;
	uint32_t sizePath = strlen(path) + 1;
	int32_t existe;
	uint32_t lastModif;
	bool esArchivo;

	checkStringEnd(path, sizePath);

	printf("Mando GETATTR sobre el path %s con %d bytes \n", path, sizePath);
	int sendResult;

	sendResult = librocket_enviarMensaje(socket_servidor, &mensajeGetAttr, 4,
			logger);

	if (sendResult == -1)
		perror("send:");
	sendResult = librocket_enviarMensaje(socket_servidor, &sizePath, 4, logger);
	if (sendResult == -1)
		perror("send:");
	sendResult = librocket_enviarMensaje(socket_servidor, path, sizePath,
			logger);
	if (sendResult == -1)
		perror("send:");

	//primero veo si existe el archivo
	int result = librocket_recibirMensaje(socket_servidor, &existe,
			sizeof(int32_t), logger);
	checkConexionActiva(result, "getAttr");

	if (existe > 0) {
		return false;
	}
	result = librocket_recibirMensaje(socket_servidor, &esArchivo, sizeof(bool),
			logger);
	checkConexionActiva(result, "getAttr");

	if (esArchivo) {
		//es archivo
		//obtengo el tamanio del mismo
		//Obtengo fecha modif
		uint32_t size;
		librocket_recibirMensaje(socket_servidor, &size, sizeof(uint32_t),
				logger);
		result = librocket_recibirMensaje(socket_servidor, &lastModif,
				sizeof(uint32_t), logger);
		checkConexionActiva(result, "getAttr");
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = size;

	} else {
		//es directorio

		result = librocket_recibirMensaje(socket_servidor, &lastModif,
				sizeof(uint32_t), logger);
		checkConexionActiva(result, "getAttr");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;

	}

	stbuf->st_mtim.tv_sec = lastModif;
	stbuf->st_mtim.tv_nsec = lastModif * 1000000;

	return true;

}

void checkConexionActiva(int result, char * peticion) {
	if (result == 0)
		servidorDesconectado(peticion);
}

void servidorDesconectado(char * peticion) {
	printf(
			"Pokedex servidor desconectado mientras se realizaba una peticion %s",
			peticion);
	log_error(logger,
			"El pokedex servidor se desconecto mientras intente hacer la peticion de %s",
			peticion);
	log_destroy(logger);
	exit(1);
}

uint32_t crearDirectorio(char* path) {
	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeCrearDir = CREAR_DIRECTORIO;
	uint32_t pathSize = strlen(path) + 1;

	librocket_enviarMensaje(socket_servidor, &mensajeCrearDir, 4, logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	int16_t creado;

	int result = librocket_recibirMensaje(socket_servidor, &creado,
			sizeof(uint16_t), logger);
	checkConexionActiva(result, "crear directorio");

	return creado;

}

int32_t crearArchivo(char* path) {
	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeCrearDir = CREAR_ARCHIVO;
	uint32_t pathSize = strlen(path) + 1;

	librocket_enviarMensaje(socket_servidor, &mensajeCrearDir, sizeof(int32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);

	int16_t creado;
	int result = librocket_recibirMensaje(socket_servidor, &creado,
			sizeof(int16_t), logger);

	checkConexionActiva(result, "crear directorio");

	return creado;

}

uint32_t renombrarArchivo(const char* from, const char* to) {

	MSG_FROM_FS_CLIENTE_TO_SERVER mensaje = RENOMBRAR_ARCHIVO;
	uint32_t pathSizeFrom = strlen(from) + 1;
	uint32_t pathSizeTo = strlen(to) + 1;

	librocket_enviarMensaje(socket_servidor, &mensaje, sizeof(uint32_t),
			logger);

	librocket_enviarMensaje(socket_servidor, &pathSizeFrom, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, from, pathSizeFrom, logger);
	librocket_enviarMensaje(socket_servidor, &pathSizeTo, sizeof(uint32_t),
			logger);
	librocket_enviarMensaje(socket_servidor, to, pathSizeTo, logger);

	int32_t renombrado;
	int result = librocket_recibirMensaje(socket_servidor, &renombrado,
			sizeof(int32_t), logger);
	checkConexionActiva(result, "Renombrando archivo");

	return renombrado;
}

void checkStringEnd(char* cadena, int size) {
	char ultimo = cadena[size];
	if (ultimo != '\0') {
		cadena[size] = '\0';
	}
}

int32_t cliente_utimens(const char* path, const struct timespec ts[2]) {
	MSG_FROM_FS_CLIENTE_TO_SERVER mensajeBorrar = UTIMENS;
	uint32_t pathSize = strlen(path) + 1;

	librocket_enviarMensaje(socket_servidor, &mensajeBorrar, 4, logger);
	librocket_enviarMensaje(socket_servidor, &pathSize, 4, logger);
	librocket_enviarMensaje(socket_servidor, path, pathSize, logger);
	librocket_enviarMensaje(socket_servidor, &ts[1].tv_sec, sizeof(__time_t ),
			logger);

	int32_t creado;
	librocket_recibirMensaje(socket_servidor, &creado, sizeof(int32_t), logger);

	if (creado == -1) {
		log_error(logger,
				"Ocurrio un error al modificar el lastmodify del el archivo");
	}
	if (creado == 1)
		return 0;
	else
		return -1;
}
