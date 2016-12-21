#ifndef PROTOCOLO_POKEDEX_H_
#define PROTOCOLO_POKEDEX_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum MSG_FROM_FS_CLIENTE_TO_SERVER_{

	ABRIR_ARCHIVO = 1,
	LEER_ARCHIVO = 2,
	ESCRIBIR_ARCHIVO = 3,
	CREAR_ARCHIVO =4,
	CREAR_DIRECTORIO = 5,
	BUSCAR_DIRECTORIO =6,
	RENOMBRAR_ARCHIVO=7,
	GET_DIRECTORIO = 8,
	GET_ATTR = 9,
	ELIMINAR_ARCHIVO=10,
	TRUNCAR_ARCHIVO=11,
	UTIMENS=12,
} MSG_FROM_FS_CLIENTE_TO_SERVER;

typedef enum ERRORES_OSADA{

	NOMBRE_MUY_LARGO=-1,
	NO_HAY_ESPACIO=-2,
	NO_HAY_DESCRIPTORES=-3,
	ARCHIVO_INEXISTENTE=-4,
	ARCHIVO_EXISTENTE=-5,
	_PATH_INVALIDO=-6,
	DIRECTORIO_NO_VACIO=-7,
} ERRORES_OSADA;


typedef enum MSG_FS_SERVER_TO_CLIENT{
	ENVIO_DATA_DE_ARCHIVO =1,
	ENVIO_DATA_DIRECTORIO = 2,
}MSG_FS_SERVER_TO_CLIENT;

typedef enum TIPO_ARCHIVO{
	ARCHIVO=1,
	DIRECTORIO=2
}tipo_archivo_t;

//struct que representa cada uno de los archivos contenidos en un directorio
//y que es enviado por socket
typedef struct DIRECTORY_ELEMENT{
	tipo_archivo_t   tipoArchivo;
	unsigned char nombre[17];

}directory_element;

typedef struct FILE_ATTR{
	tipo_archivo_t tipoArchivo;
	uint32_t size;
	bool existe;
	uint32_t lastmod;
} directory_attr;

#endif




