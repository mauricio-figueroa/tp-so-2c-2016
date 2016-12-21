#include <protocolos/p_pokeDex.h>
#include "FuncionesOsada.h"
#include "../globales.h"


/**
 * Escribe un archivo en el path especificado.
 * Devuelve bloques escritos o
 * -1 si hay error o si el archivo ya existe.
 */



/*
 * Devuelve -2 si la tabla de archivos esta llena (EDQUOT)
 * Devuelve -1 si el archivo ya existe o nombre largo
 */
int16_t osada_crearArchivo(unsigned char* path, FILE* osada_fs) {

	int cantArchivos = obtenerCantidadDeArchivos(osada_fs);
		if(cantArchivos>=2048){
			return NO_HAY_DESCRIPTORES;
		}

	osada_header* header=leerHeaderOsada(osada_fs);
	int32_t error = 0;
	int32_t* resultado = malloc(sizeof(int32_t));
	int32_t* posicion = malloc(sizeof(int16_t));



	leerPath(osada_fs, path, resultado, posicion);
	unsigned char* nombre = obtenerNombreDeArchivoDePath(path);

	//TODO: deberia permitir archivos de 17 caracteres
	//Si el archivo no existe, el path es valido y el nombre es menor a 17 bytes
	if (*resultado == ARCHIVO_NO_PATH_VALIDO && strlen(nombre) <= 17){
		error = escribirDirectorioInterna(osada_fs, header, nombre, *posicion, REGULAR);
	}else{
		error=PATH_INVALIDO;
	}
	free(resultado);
	free(posicion);
	return error;
}

/**
 * Edita el archivo especificado en el path.
 * Devuelve bytes escritos o
 * -1 si hay error o si el archivo no existe.
 */



int32_t osada_editarArchivo(unsigned char* path, int32_t inicioBytes, int32_t bytesAEscribir, unsigned char* archivo,char* sockId, FILE* osada_fs) {

	//printf("Osada_editarArchivo: %s, desde: %d offset: %d contenido: %s\n", path, inicioBytes, bytesAEscribir, archivo);

	osada_header* header=leerHeaderOsada(osada_fs);
	int32_t bytesEscritos = 0;
	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));



	osada_file* descriptor = leerPath(osada_fs, path, resultado, posicion);

	if (*resultado == ARCHIVO_EXISTE){


		pthread_rwlock_wrlock(locks + (*posicion));
		bytesEscritos = editarArchivoInterna(osada_fs, header, descriptor, *posicion, bytesAEscribir, inicioBytes, archivo,sockId);
		if (bytesEscritos != NO_HAY_ESPACIO){
			if (bytesAEscribir > descriptor->file_size - inicioBytes)
				descriptor->file_size = bytesAEscribir + inicioBytes;
			reemplazarEstructuraDeArchivo(osada_fs, header, *posicion, descriptor);
		}

		pthread_rwlock_unlock(locks + (*posicion));
	}
	else
		bytesEscritos=ARCHIVO_INEXISTENTE;

	free(descriptor);
	free(resultado);
	free(posicion);
	return bytesEscritos;
}

/**
 * Truncar el archivo especificado en el path.
 * Devuelve bytes escritos o
 * -1 si hay error o si el archivo no existe.
 */



int32_t osada_TruncarArchivo(unsigned char* path, int32_t bytesATruncar,char* sockId, FILE* osada_fs) {

	printf("Osada_truncarArchivo: offset: %d\n", bytesATruncar);

	osada_header* header=leerHeaderOsada(osada_fs);
	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));
	int32_t out;

	osada_file* descriptor = leerPath(osada_fs, path, resultado, posicion);

	if (*resultado == ARCHIVO_EXISTE){
		pthread_rwlock_wrlock(locks + (*posicion));
		out=truncarArchivoInterna(osada_fs, header, descriptor, bytesATruncar);
		if (out==1){
			if (bytesATruncar>descriptor->file_size)
				rellenarConEspacios(osada_fs, header, descriptor, posicion, bytesATruncar,sockId);
			descriptor->file_size = bytesATruncar;
			reemplazarEstructuraDeArchivo(osada_fs, header, *posicion, descriptor);
			pthread_rwlock_unlock(locks + (*posicion));

		}else {
			pthread_rwlock_unlock(locks + (*posicion));

		}
	}
	else
		out=ARCHIVO_INEXISTENTE;

	free(descriptor);
	free(resultado);
	free(posicion);
	return out;
}


/**
 * Crea un directorio en el path especificado.
 * Devuelve la posicion en el array de descriptores de archivos.
 * -2 si no hay entradas para mas archivos o directorios
 * -1 si hay otro error
 */

int16_t osada_crearDirectorio(unsigned char* path, FILE* osada_fs) {


	if(obtenerCantidadDeArchivos(osada_fs)>=2048){
		return NO_HAY_DESCRIPTORES;
	}

	osada_header* header=leerHeaderOsada(osada_fs);
	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));
	osada_file* f=leerPath(osada_fs, path, resultado, posicion);
	int16_t posicionNuevoDirectorio=-1;

	unsigned char* nombre = obtenerNombreDeArchivoDePath(path);

	if (*resultado == ARCHIVO_NO_PATH_VALIDO){
		posicionNuevoDirectorio=escribirDirectorioInterna(osada_fs, header, nombre, *posicion, DIRECTORY);
	}else{
		posicionNuevoDirectorio=PATH_INVALIDO;
	}
	free(resultado);
	free(f);
	free(nombre);
	free(posicion);
	return posicionNuevoDirectorio;

}

/**
 * Comprueba que existe el archivo.
 * Devuelve 0 si existe (y si la estructura de directorios es valida)
 * o -1 si no existe
 */

int32_t osada_existeArchivo(unsigned char* path, FILE* osada_fs) {



	osada_header* header = leerHeaderOsada(osada_fs);
	int32_t* resultado = malloc(sizeof(int32_t));

	leerPath(osada_fs, path, resultado, NULL);

	if (*resultado == 0)
		return 0;
	else
		return -1;
}

/**
 * Devuelve la lista de archivos y directorios de un directorio especificado en el parametro
 * Devuelve una estructura (separado por un caracter especial alt+20:
 *
 *  tipoArchivo nombre[17] | tipoArchivo nombre[17] ...
 *
 * Ademas en el parametro cantidad especifica cuantos archivos se encontraron
 */

t_list* osada_estructuraDirectorio(unsigned char* path,
		uint32_t* cantidad, FILE* osada_fs) {



	osada_header* header = leerHeaderOsada(osada_fs);
	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));

	osada_file* directorio = leerPath(osada_fs, path, resultado, posicion);
	t_list* listaDirectorios;

	t_list* listaElementos = list_create();
	directory_element* elemento = NULL;

	int i = 0;
	//El path es valido y es un directorio
	if (*resultado == 0 && directorio->state == DIRECTORY) {
		listaDirectorios = listarDirectorio(osada_fs, *posicion, header);
		while (i < list_size(listaDirectorios)) {
			osada_file* archivo = (osada_file*) list_get(listaDirectorios, i);
			elemento = malloc(sizeof(directory_element));
			if (archivo->state == DIRECTORY)
				elemento->tipoArchivo = DIRECTORIO;
			else
				elemento->tipoArchivo = ARCHIVO;

			strcpy(elemento->nombre, archivo->fname);
			list_add(listaElementos, elemento);
			i++;

		}

	}
	free(posicion);
	free(resultado);
	free(directorio);
	//list_clean_and_destroy_elements(listaDirectorios, )
	*cantidad = i;
	return listaElementos;
}

/**
* Devuelve el archivo leido indicado en el path o 
* NULL si no existe el archivo o hay algun error
*	bytesALeer: indica los bytes que debe leer
*	posicionInicial: Desde que byte en el archivo debe comenzar a leer
*	bytesLeidos: Devuelve cuantos bytes leyo
*
*/
unsigned char* osada_leerArchivo(unsigned char* path, uint32_t bytesALeer, uint32_t posicionInicial, uint32_t* bytesLeidos_p, FILE* osada_fs){

	//printf("Osada_leerArchivo: %s, desde: %d offset: %d\n", path, posicionInicial, bytesALeer);

    int32_t* resultado = malloc(sizeof(int32_t));
    int32_t* bytesLeidos = malloc(sizeof(uint32_t));
    unsigned char* archivo=NULL;
    osada_file* descriptor = leerPath(osada_fs, path, resultado, NULL);
	osada_header* header=leerHeaderOsada(osada_fs);
    if (*resultado==0 && descriptor->state!=DIRECTORY){
        archivo=obtenerArchivoInterna(osada_fs, header, descriptor, bytesALeer, posicionInicial, bytesLeidos);
    }else
    	*bytesLeidos=ARCHIVO_INEXISTENTE;
	if (bytesLeidos_p != NULL)
		*bytesLeidos_p = *bytesLeidos;
	free(bytesLeidos);
	free(resultado);
	free(descriptor);

	return archivo;
}
/**
 * Renombra el archivo indicado en el path, y le pone el nombre indicado en nuevoNombre.
 * Devuelve 1 si fue exitoso o
 * -1 si hubo un error
 */

int32_t osada_renombrarArchivo(unsigned char* viejoNombre, unsigned char* nuevoNombre , FILE* osada_fs){

	//printf("osada_renombrarArchivo: %s a %s\n", viejoNombre, nuevoNombre);

	osada_header* header = leerHeaderOsada(osada_fs);

	int32_t* resultadoViejo = malloc(sizeof(int32_t));
	uint16_t* posicionViejo = malloc(sizeof(uint16_t));

	int32_t* resultadoNuevo = malloc(sizeof(int32_t));
	uint16_t* posicionNuevo = malloc(sizeof(uint16_t));

	osada_file* descriptorViejo=leerPath(osada_fs, viejoNombre, resultadoViejo, posicionViejo);
	leerPath(osada_fs, nuevoNombre, resultadoNuevo, posicionNuevo);

	if (*resultadoViejo == ARCHIVO_EXISTE && strcmp(viejoNombre, "/")!=0 && *resultadoNuevo ==ARCHIVO_NO_PATH_VALIDO){
		strcpy(descriptorViejo->fname, obtenerNombreDeArchivoDePath(nuevoNombre));
		descriptorViejo->parent_directory = *posicionNuevo;
		reemplazarEstructuraDeArchivo(osada_fs, header, *posicionViejo, descriptorViejo);
	}else{
			free(descriptorViejo);
			free(resultadoViejo);
			free(resultadoNuevo);
			free(posicionViejo);
			free(posicionNuevo);
		return ARCHIVO_INEXISTENTE;
	}
	free(descriptorViejo);
	free(resultadoViejo);
	free(resultadoNuevo);
	free(posicionViejo);
	free(posicionNuevo);
	return 1;
}

/**
 * Elimina el archivo indicado en el path. Ademas se indica
 * hard = 0: Borra solo el directorio o archivo especificado (si tiene hijos, no borra nada)
 * o hard = cualquier otro numero: Si se especifico un directorio, borra tambien sus subdirectorios y archivos
 * Devuelve la cantidad de archivos eliminados o
 * -1 si hubo error
 */

int32_t osada_eliminarArchivo(unsigned char* path, uint32_t hard, FILE* osada_fs){

	//printf("osada_EliminarArchivo: %s\n", path);

	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));
	int32_t cantidad;
	osada_header* header = leerHeaderOsada(osada_fs);
	osada_file* archivos=obtenerTablaArchivos(osada_fs, header);
	osada_file* descriptor=leerPath(osada_fs, path, resultado, posicion);

	if (*resultado == ARCHIVO_EXISTE && strcmp(path, "/")!=0){
		cantidad= eliminarArchivoInterna(osada_fs, header, archivos, *posicion, descriptor, hard);
	}else{
		cantidad = ARCHIVO_INEXISTENTE;
	}
	free(resultado);
	free(posicion);
	free(archivos);
	free(descriptor);
	return cantidad;
}

/**
 * Actualiza la fecha de modificacion del archivo especificado.
 * Devuelve 1 en caso de exito
 * o -1 en caso de error
 */

int32_t osada_utimens(unsigned char* path, __time_t segundos, FILE* osada_fs){

	int32_t* resultado = malloc(sizeof(int32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));
	int32_t out;
	osada_header* header = leerHeaderOsada(osada_fs);
	osada_file* descriptor=leerPath(osada_fs, path, resultado, posicion);

	if (*resultado == ARCHIVO_EXISTE && strcmp(path, "/")!=0){
		descriptor->lastmod = (uint32_t) segundos;
		reemplazarEstructuraDeArchivo(osada_fs, header, *posicion, descriptor);
		out=1;
	}else
		out=-1;
	free(resultado);
	free(posicion);
	free(descriptor);
	return out;
}

directory_attr* osada_getAtributosArchivo(unsigned char* path, FILE* osada_fs){


	int32_t* resultado = malloc(sizeof(int32_t));
	osada_header* header = leerHeaderOsada(osada_fs);
	uint16_t* posicion = malloc(sizeof(uint16_t));
	osada_file* descriptor=leerPath(osada_fs, path, resultado, posicion);
	directory_attr* atributosSalida = malloc(sizeof(directory_attr));




	if (*resultado == ARCHIVO_EXISTE){
		atributosSalida->existe = true;
		if (descriptor->state==DIRECTORY){
			atributosSalida->tipoArchivo=DIRECTORIO;
			atributosSalida->size = descriptor->file_size;
			atributosSalida->lastmod = descriptor->lastmod;
		}else{
			atributosSalida->tipoArchivo=ARCHIVO;
			atributosSalida->size = descriptor->file_size;
			atributosSalida->lastmod = descriptor->lastmod;
		}
	}else {
		atributosSalida->existe = false;
	}

	free(resultado);
	free(descriptor);
	return atributosSalida;

}
