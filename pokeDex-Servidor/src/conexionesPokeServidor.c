#include "conexiones.h"
#include "osada/FuncionesOsada.h"
#include <commons/log.h>
#include <commons/string.h>

//TODO HARDCODED
#define PUERTO "4500"

//logger del servidor
t_log* logger;


//recordar liberar el string
char* obtenerNumeroSocketComoString(int numero){
	char* socketString = malloc(15);
	sprintf(socketString,"%d",numero);
	return socketString;
}

/*
 * Habra una instancia de este hilo por cada PokeDex Cliente
 * Debe responder ante todos los mensajes del protocolo, y manejar la sincronizacion
 *
 */
void * hiloManejarConexiones(void* sockfd) {

	FILE* osada_fs = fopen("nuevoDisco.bin", "rb+");

	if(osada_fs == NULL) {
		printf("------------------------------------------------------------------------\n");
		printf("UBICACION DEL ARCHIVO DE DISCO INVALIDA\n");
		printf("-----------------------------------------------------------------------\n");
	}

	//Crear en el main
	logger = log_create("info_logs","pokedex_servidor",false,LOG_LEVEL_INFO);

	int clienteSocket = (int) sockfd;
	char* str = string_new();
	string_append(&str,"Conectado un pokeDex cliente con ID: ");
	char* numeroSocketEnString = obtenerNumeroSocketComoString(sockfd);
	string_append(&str,numeroSocketEnString);

	log_info(logger,str);
	free(str);
	free(numeroSocketEnString);

	printf("Hilo creado con socket %d \n", clienteSocket);


	char* buffer;
	buffer = malloc(4); // (4 bytes porque es el tamanio que definimos para identificar el tipo de mensaje)

	//uint32_t tipoMsj;
	MSG_FROM_FS_CLIENTE_TO_SERVER tipoMensaje;

	while (1) {
		//recibo el header
		int result = librocket_recibirMensaje(clienteSocket, &tipoMensaje, 4, logger);


		switch (result) {
		case 0: //cliente se desconecto
			printf("Socket %d cerrado, cerramos el hilo\n", clienteSocket);
			fflush(stdout);
			free(buffer);
			close(clienteSocket);

			return NULL;
		case -1: //error
			printf("Recv sobre un pokedex cliente devolvio -1\n");
			break;

		case 1: //datos recibidos bien
			//ver que mensaje mando el pokedex cliente y hacer lo que corresponda
			interpretarMensaje(tipoMensaje, clienteSocket, osada_fs);
			break;
		}

	}
	log_destroy(logger);

	return NULL;

}

void interpretarMensaje(MSG_FROM_FS_CLIENTE_TO_SERVER tipoMensaje, int sockfd, FILE* fs) {

	switch (tipoMensaje) {
	case ABRIR_ARCHIVO:
		abrirArchivo(sockfd, fs);
		break;
	case LEER_ARCHIVO:
		leerArchivo(sockfd, fs);
		break;
	case ESCRIBIR_ARCHIVO:
		escribirArchivo(sockfd, fs);
		break;
	case CREAR_ARCHIVO:
		crearArchivo(sockfd, fs);
		break;
	case CREAR_DIRECTORIO:
		crearDirectorio(sockfd, fs);
		break;
	case BUSCAR_DIRECTORIO:
		buscarDirectorio(sockfd, fs);
		break;
	case RENOMBRAR_ARCHIVO:
		renombrarArchivo(sockfd, fs);
		break;
	case GET_DIRECTORIO:
		getEstructuraDirectorio(sockfd, fs);

		break;
	case GET_ATTR:
		getAttr(sockfd, fs);
		break;
	case ELIMINAR_ARCHIVO:
		eliminarArchivo(sockfd, fs);
		break;
	case TRUNCAR_ARCHIVO:
		truncarArchivo(sockfd, fs);
		break;
	case UTIMENS:
		server_utimens(sockfd, fs);
		break;
	}
}
;

void getAttr(int sockfd, FILE* fs) {
	uint32_t pathSize;
	int result;


	result = librocket_recibirMensaje(sockfd,&pathSize,4,logger);//ya recibimos el header (9) que indica el listar los atributos, ahora recibimos el largo del path
	if (result > 0) {
		char* path = malloc(pathSize);

		result =librocket_recibirMensaje(sockfd,path,pathSize,logger);
		char * cadenaALoguear = string_new();
		string_append(&cadenaALoguear,"pokedex numero ");
		char * numPokedex = obtenerNumeroSocketComoString(sockfd);
		string_append(&cadenaALoguear, numPokedex);
		string_append(&cadenaALoguear, " hizo getAttr sobre ");
		string_append(&cadenaALoguear,path);
		free(numPokedex);
		log_info(logger,cadenaALoguear);
		free(cadenaALoguear);

		if (result > 0) {

			directory_attr* atributos = osada_getAtributosArchivo(path, fs);


			if(atributos->existe){
				int32_t existe = 0;
				librocket_enviarMensaje(sockfd,&existe,sizeof(int32_t), logger);


			}else {
				int32_t existe = 1;
				librocket_enviarMensaje(sockfd,&existe,sizeof(int32_t), logger);//no existe!

				free(path);
				return;
			}

			bool esArchivo;
			free(path);
			if (atributos->tipoArchivo == ARCHIVO) {
				esArchivo = true;
				librocket_enviarMensaje(sockfd,&esArchivo,sizeof(bool), logger);
				librocket_enviarMensaje(sockfd,&atributos->size,sizeof(uint32_t), logger);
				librocket_enviarMensaje(sockfd,&atributos->lastmod,sizeof(uint32_t), logger);

			} else if (atributos->tipoArchivo == DIRECTORIO) {
				esArchivo = false;
				librocket_enviarMensaje(sockfd,&esArchivo,sizeof(bool), logger);
				librocket_enviarMensaje(sockfd,&atributos->lastmod,sizeof(uint32_t), logger);

			}
		} else {
			log_error(logger,"Error on recv pathSize, getAttr-> POKEDEX SERVIDOR");
		}
	} else {
		log_error(logger, "Error on recv path, getAttr-> POKEDEX SERVIDOR");
	}
}
;

void eliminarArchivo(int sockfd, FILE* fs) {

	char * bufferPath;
	int pathSize;
	int result = librocket_recibirMensaje(sockfd,&pathSize,4,logger);

	if (result > 0) {
		bufferPath = malloc(sizeof(char) * pathSize);

		result = librocket_recibirMensaje(sockfd,bufferPath,pathSize,logger);
		char * cadenaALoguear = string_new();
				string_append(&cadenaALoguear,"pokedex numero ");
				char * numPokedex = obtenerNumeroSocketComoString(sockfd);
				string_append(&cadenaALoguear, numPokedex);
				string_append(&cadenaALoguear, " pidio eliminar el archivo ");
				string_append(&cadenaALoguear,bufferPath);
				free(numPokedex);
				log_info(logger,cadenaALoguear);
				free(cadenaALoguear);

		if (result > 0) {
			//Desde logica fuse no hay HARD, con lo cual sera siempre en 0
			uint32_t hard = 0;
			int32_t resultado = osada_eliminarArchivo(bufferPath, hard, fs);
			if(resultado == -1){ //No elimino nada
				log_error(logger,"No se eliminar el archivo");
			}
			librocket_enviarMensaje(sockfd,&resultado,sizeof(int32_t),logger);
			} else {
			log_error(logger,
					"Error on recv bufferPath, eliminarArchivo POKEDEX SERVIDOR");
		}

	} else {
		log_error(logger,
				"Error on recv pathSize, eliminarArchivo POKEDEX SERVIDOR");
	}

}

void server_utimens(int sockfd, FILE* fs) {

	char * bufferPath;
	uint32_t pathSize;
	int r1=librocket_recibirMensaje(sockfd,&pathSize,sizeof(uint32_t),logger);
	__time_t segundos;
	int32_t resultado=-1;

	bufferPath = malloc(sizeof(char) * pathSize);

	int r2=librocket_recibirMensaje(sockfd,bufferPath,pathSize,logger);
	int r3=librocket_recibirMensaje(sockfd,&segundos,sizeof(__time_t),logger);

	if (r1>0 && r2>0 && r3>0){
		resultado = osada_utimens(bufferPath, segundos, fs);
	}
	else{
		char* string = string_new();
		string_append(&string, "No se pudo actualizar la fecha del archivo ");
		string_append(&string, bufferPath);
		log_error(logger,bufferPath);
	}

	librocket_enviarMensaje(sockfd,&resultado,sizeof(int32_t),logger);

}

void getEstructuraDirectorio(int sockfd, FILE* fs) {

	int pathSize; //ya recibimos el header (8) que identifica que se quiere obtener la estructura de deirectorio, ahora recibimos le largo del path a obtenerEstructura
	int result = librocket_recibirMensaje(sockfd,&pathSize,4,logger);


	if (result > 0) {
		char* path = malloc(pathSize);
		result = librocket_recibirMensaje(sockfd,path,pathSize,logger);
		if (result > 0) {
			char * cadenaALoguear = string_new();
					string_append(&cadenaALoguear,"pokedex numero ");
					char * numPokedex = obtenerNumeroSocketComoString(sockfd);
					string_append(&cadenaALoguear, numPokedex);
					string_append(&cadenaALoguear, "pidio info del Directorio ");
					string_append(&cadenaALoguear,path);
					free(numPokedex);
					log_info(logger,cadenaALoguear);
					free(cadenaALoguear);

			uint32_t cantidadObjetosDirectorio;
			t_list* directoryArray;
			directoryArray = osada_estructuraDirectorio(path,
					&cantidadObjetosDirectorio, fs);
			MSG_FS_SERVER_TO_CLIENT tipoMensaje = ENVIO_DATA_DIRECTORIO;
			librocket_enviarMensaje(sockfd,&tipoMensaje,4, logger); //informo que estoy devolviendo info de un directorio
			librocket_enviarMensaje(sockfd,&cantidadObjetosDirectorio,4, logger);//-->envio la cantidad de estructuras directory_element, que va a recibir
			if (cantidadObjetosDirectorio > 0) {
				int i;
				for (i = 0; i < cantidadObjetosDirectorio; i++) {
					//send--->
					directory_element* directorioActual = (directory_element*) list_get(directoryArray, i);
					librocket_enviarMensaje(sockfd,&(directorioActual->nombre),17, logger);
					librocket_enviarMensaje(sockfd,&(directorioActual->tipoArchivo),sizeof(tipo_archivo_t), logger);

				}
			}
		} else {
			log_error(logger,
					"Error recibiendo el path Funcion-> getEstructuraDirectorio, conexionesPokeServidor, Pokedex->Server");
		}
	} else {
		log_error(logger,
				"Error recibiendo el pathSize Funcion-> getEstructuraDirectorio, conexionesPokeServidor, Pokedex->Server");
	}
}

void renombrarArchivo(int sockfd, FILE* fs) {

	uint32_t oldNameSize; //ya recibimos el header que identifica que es una busquedaDeDirectorio, ahora recibimos el largo del nombre del archivo a cambiar de nombre;
	int rcv1=librocket_recibirMensaje(sockfd, &oldNameSize, sizeof(uint32_t), logger);

	unsigned char* from = malloc(oldNameSize);
	int rcv2=librocket_recibirMensaje(sockfd, from, oldNameSize, logger);

	uint32_t newNameSize;
	int rcv3=librocket_recibirMensaje(sockfd, &newNameSize, sizeof(uint32_t), logger);

	unsigned char* to = malloc(newNameSize);
	int rcv4=librocket_recibirMensaje(sockfd, to, newNameSize, logger);

	if (rcv1 > 0 && rcv2 > 0 && rcv3 > 0 && rcv4>0) {
		int32_t result = osada_renombrarArchivo(from, to, fs);
		printf("Se cambio el nombre del archivo %s por %s \n",from,to);
		librocket_enviarMensaje(sockfd, &result, sizeof(int32_t), logger);

	} else {
		log_error(logger,
				"Error on recv oldNameSize or newNameSize, busquedaDeDirectorio POKEDEX SERVIDOR");

	}

}

void buscarDirectorio(int sockfd, FILE* fs) {

	int pathSize; //ya recibimos el header que identifica que es una busquedaDeDirectorio, ahora recibimos el largo del path de busqueda.
	int result = librocket_recibirMensaje(sockfd,&pathSize,4,logger);

	if (result > 0) {
		char* buffer = malloc(pathSize);
		int recvPath = librocket_recibirMensaje(sockfd,buffer,pathSize,logger);
		if (recvPath > 0) {
			//DELEGAR EN FUNCION DANI
		} else {
			log_error(logger,
					"Error on recv Directory path, busquedaDeDirectorio POKEDEX SERVIDOR");
		}
	} else {
		log_error(logger,
				"Error on recv directory pathSize, busquedaDeDirectorio POKEDEX SERVIDOR");
	}

}

void crearDirectorio(int sockfd, FILE* fs) {

	uint32_t pathSize; //ya recibimos el header que identifica que es una creacion de directorio, ahora recibimos el largo del path de creacion.
	uint32_t result = librocket_recibirMensaje(sockfd,&pathSize,sizeof(uint32_t),logger);

	if (result > 0) {
		char* buffer = malloc(pathSize);
		int recvPath = librocket_recibirMensaje(sockfd,buffer,pathSize,logger);
		if (recvPath > 0) {
			char * cadenaALoguear = string_new();
					string_append(&cadenaALoguear,"pokedex numero ");
					char * numPokedex = obtenerNumeroSocketComoString(sockfd);
					string_append(&cadenaALoguear, numPokedex);
					string_append(&cadenaALoguear, "pidio crear el directorio ");
					string_append(&cadenaALoguear,buffer);
					free(numPokedex);
					log_info(logger,cadenaALoguear);
					free(cadenaALoguear);
				int16_t resultado =	osada_crearDirectorio(buffer, fs);
				if(resultado == -1){ //fallo la creacion del directorio
					log_error(logger,"No se pudo crear el directorio");
				}else if(resultado == -2){
					log_error(logger,"No se pudo crear el directorio, tabla de archivos llena");
				}else {
					printf("Se creo el directorio %s",buffer);
				}
				librocket_enviarMensaje(sockfd,&resultado,sizeof(int16_t), logger);
		} else {
			log_error(logger,
					"Error recibiendo el path Funcion-> Crear Directorio, conexiones, Pokedex, Server");
		}
		free(buffer);

	} else {
		log_error(logger,
				"Error on recv directory pathSize, creacionDirectorio POKEDEX SERVIDOR");
	}

}

void escribirArchivo(int sockfd, FILE* fs) {

	int result;
	uint32_t  pathSize; //largo del nombre del path del archivo, directorios+nombre y extension. ejemplo /dir1/dir2/ejemplo.txt
	size_t bytesToWrite;

	uint32_t offset; //desplazamiento
	char * path;
	char* datosAEscribir;
	result = librocket_recibirMensaje(sockfd,&bytesToWrite,sizeof(size_t),logger);
		if (result > 0) {
			result = librocket_recibirMensaje(sockfd,&offset,sizeof(uint32_t),logger);
			if (result > 0) {
				result = librocket_recibirMensaje(sockfd,&pathSize,sizeof(uint32_t),logger);
				if(result>0){
					path = malloc(pathSize);
					datosAEscribir = malloc(bytesToWrite);
				int result2 = 	librocket_recibirMensaje(sockfd,path,pathSize,logger);
				int result = 	librocket_recibirMensaje(sockfd,datosAEscribir,bytesToWrite,logger);

					if(result > 0 && result2 >0){
						char * cadenaALoguear = string_new();
								string_append(&cadenaALoguear,"pokedex numero ");
								char * numPokedex = obtenerNumeroSocketComoString(sockfd);
								string_append(&cadenaALoguear, numPokedex);
								string_append(&cadenaALoguear, " pidio escribir sobre ");
								string_append(&cadenaALoguear,path);
								free(numPokedex);
								log_info(logger,cadenaALoguear);
								free(cadenaALoguear);
						int32_t bytesEscritos =  osada_editarArchivo(path, offset, bytesToWrite, datosAEscribir,obtenerNumeroSocketComoString(sockfd), fs);
						printf("Se escribieron %d bytes en el archivo %s de %d pedidos a partir del byte %d \n",bytesEscritos,path,bytesToWrite,offset);
						librocket_enviarMensaje(sockfd,&bytesEscritos,sizeof(uint32_t), logger);
						free(path);
						free(datosAEscribir);
					}
				}


			}

		}



}

void truncarArchivo(int sockfd, FILE* fs) {

	int result;
	uint32_t offset;
	uint32_t tamanioPath;
	char * path;

	result = librocket_recibirMensaje(sockfd, &offset, sizeof(uint32_t), logger);
	if (result > 0) {
		result = librocket_recibirMensaje(sockfd, &tamanioPath, sizeof(uint32_t), logger);

		if (result > 0) {
			path = malloc(tamanioPath);
			result = librocket_recibirMensaje(sockfd, path, tamanioPath, logger);

			if(result>0){
				char * cadenaALoguear = string_new();
						string_append(&cadenaALoguear,"pokedex numero ");
						char * numPokedex = obtenerNumeroSocketComoString(sockfd);
						string_append(&cadenaALoguear, numPokedex);
						string_append(&cadenaALoguear, "pidio truncar el archivo ");
						string_append(&cadenaALoguear,path);
						free(numPokedex);
						log_info(logger,cadenaALoguear);
						free(cadenaALoguear);
				int32_t out =  osada_TruncarArchivo(path, offset,sockfd, fs);

				free(path);
				librocket_enviarMensaje(sockfd, &out, sizeof(int32_t), logger);
				return;
			}
		}
	}
	librocket_enviarMensaje(sockfd, -1, sizeof(int32_t), logger);
}

void leerArchivo(int sockfd, FILE* fs) {

	size_t tamanioALeer;
	int32_t offset;
	uint32_t tamanioPath;
	unsigned char* buffer;
	unsigned char* archivo;

	int result = librocket_recibirMensaje(sockfd,&tamanioALeer,sizeof(size_t),logger);
	if (result > 0) {
		result = librocket_recibirMensaje(sockfd,&offset,sizeof(int32_t),logger);
		if (result > 0) {
			result = librocket_recibirMensaje(sockfd,&tamanioPath,sizeof(uint32_t),logger);
			if (result > 0) {
				buffer = malloc(tamanioPath);
				result = librocket_recibirMensaje(sockfd,buffer,tamanioPath,logger);
				if (result > 0) {
					char * cadenaALoguear = string_new();
							string_append(&cadenaALoguear,"pokedex numero ");
							char * numPokedex = obtenerNumeroSocketComoString(sockfd);
							string_append(&cadenaALoguear, numPokedex);
							string_append(&cadenaALoguear, "pidio leer el archivo ");
							string_append(&cadenaALoguear,buffer);
							free(numPokedex);
							log_info(logger,cadenaALoguear);
							free(cadenaALoguear);
					archivo  = malloc(tamanioALeer);
					uint32_t bytesLeidos;
					archivo = osada_leerArchivo(buffer, tamanioALeer, offset, &bytesLeidos, fs);
					printf("Leido el archivo %s \n",buffer);
					librocket_enviarMensaje(sockfd,archivo,tamanioALeer, logger);
					free(archivo);
				} else {
					log_error(logger,
							"Error on recv buffer , leerArchivo POKEDEX SERVIDOR");
				}
			} else {
				log_error(logger,
						"Error on recv tamanioPath , leerArchivo POKEDEX SERVIDOR");
			}
		} else {
			log_error(logger,
					"Error on recv offset , leerArchivo POKEDEX SERVIDOR");
		}
	} else {
		log_error(logger,
				"Error on recv tamanioALeer , leerArchivo POKEDEX SERVIDOR");

	}

}

void crearArchivo(int sockfd, FILE* fs) {

	int nameSize; //ya recibimos el header que identifica que es una creacion de archivo, ahora recibimos el largo del nombre del archivo.
	int result = librocket_recibirMensaje(sockfd,&nameSize,sizeof(int32_t),logger);

	if (result > 0) {
		char* buffer = malloc(nameSize);
		int recvPath = librocket_recibirMensaje(sockfd,buffer,nameSize,logger);

		if (recvPath > 0) {
			char * cadenaALoguear = string_new();
					string_append(&cadenaALoguear,"pokedex numero ");
					char * numPokedex = obtenerNumeroSocketComoString(sockfd);
					string_append(&cadenaALoguear, numPokedex);
					string_append(&cadenaALoguear, "pidio crear el archivo ");
					string_append(&cadenaALoguear,buffer);
					free(numPokedex);
					log_info(logger,cadenaALoguear);
					free(cadenaALoguear);
			int16_t resultado = osada_crearArchivo(buffer, fs);
			if(resultado == PATH_INVALIDO){ //fallo la creacion del directorio
				log_error(logger,"No se pudo crear el archivo");
			}else if (resultado == NO_HAY_DESCRIPTORES){
				log_error(logger,"Se quiso crear un archivo pero la tabla esta llena");
			}else{
				printf("Creado el archivo %s\n",buffer);
				log_info(logger,"Se creo el archivo");
			}
			librocket_enviarMensaje(sockfd,&resultado,sizeof(int16_t), logger);
		} else {
			log_error(logger,
					"Error on recv name, crearArchivo POKEDEX SERVIDOR");
		}
	} else {
		log_error(logger,
				"Error on recv nameSize, crearArchivo POKEDEX SERVIDOR");
	}

}

void abrirArchivo(int sockfd, FILE* fs) {

	int pathSize; //ya recibimos el header que identifica que es una creacion de archivo, ahora recibimos el largo del nombre del archivo.
	int result =librocket_recibirMensaje(sockfd,&pathSize,4,logger);


	if (result > 0) {
		char * buffer = malloc(pathSize);
		int recvPath =librocket_recibirMensaje(sockfd,buffer,pathSize,logger);
		if (recvPath > 0) {
			int32_t existe = osada_existeArchivo(buffer, fs);
			int32_t result;
			if (existe == 0) {
				result = 0;
				librocket_enviarMensaje(sockfd,&result,sizeof(int32_t),logger);
				free(buffer);
			} else {
				result = 1;
				librocket_enviarMensaje(sockfd,&result,sizeof(int32_t),logger);
				printf("Se quiso abrir el  archivo NO existente %s \n",buffer);
			}
		} else {
			log_error(logger,
					"Error on recv path directory, existeArchivo POKEDEX SERVIDOR");
		}
	} else {
		log_error(logger,
				"Error on recv directory pathSize, existeArchivo POKEDEX SERVIDOR");
	}
}

