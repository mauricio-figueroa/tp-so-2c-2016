#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "LectorOsada.h"
#include "../globales.h"
#include <pthread.h>
#include <protocolos/p_pokeDex.h>

int bitsEnChar(unsigned char c);
osada_header* header_singleton;
unsigned char* punteroBitmap_singleton;

/**
 * Recibe un archivo y devuelve el header del FS OSADA.
 * o NULL si hay error
 */
osada_header* leerHeaderOsada(FILE* fichero) {
	if (header_singleton==NULL){
		header_singleton = malloc(sizeof(osada_header));
		//@AUTHOR: fede TODO warning:  invalid read of size 4
		if(fichero == NULL){
			printf("fichero vacio\n");

		}
		fseek(fichero, 0, SEEK_SET);
		if (fread(header_singleton, sizeof(osada_header), 1, fichero) != 1)
			return NULL;
	}
	return header_singleton;
}

/**
 * Devuelve el espacio libre que tiene (en bloques)
 * o -1 Si hay error.
 */
uint32_t obtenerBloquesLibres(FILE* fichero, osada_header* header) {
	if (header == NULL)
		return -1;


	uint32_t tamanioBitMapEnBytes = header->bitmap_blocks * OSADA_BLOCK_SIZE;
	pthread_mutex_lock(&lockBitmap);
	unsigned char* bitMap = obtenerBitmap(fichero, header);

	int i;
	int resultado = 0;
	for (i = 0; i < tamanioBitMapEnBytes; i++) {
		resultado += bitsEnChar(bitMap[i]);
	}
	pthread_mutex_unlock(&lockBitmap);
	return header->fs_blocks - resultado;
}

/**
 * Recibe un char y devuelve la cantidad de bits 1 que contiene
 */
int bitsEnChar(unsigned char c) {
	int count = 0;
	while (c) {
		if (c % 2 == 1)
			count++;
		c = c >> 1;
	}
	return count;
}

//Cantidad de 1s en un int
uint32_t bitsEnUint32(uint32_t i) {
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

/**
 * Recibe un file system, el header del mismo y devuelve el numero de bloque del primer bloque de datos libres
 * que encuentre. (El primer bloque es el numero 0)
 * -1 si no encuentra o hay un error
 **/

int32_t obtenerPrimerBloqueLibre(FILE* fichero, osada_header* header) {
	if (header == NULL)
		return -1;

	int tamanioBitMapEnBytes = header->bitmap_blocks * OSADA_BLOCK_SIZE;
	int i;
	int pos;


	pthread_mutex_lock(&lockBitmap);
	unsigned char* punteroBitmap = obtenerBitmap(fichero, header);
	if (punteroBitmap == NULL) {
		pthread_mutex_unlock(&lockBitmap);
		return -1;
	}
	for (i = 0; i < tamanioBitMapEnBytes; i++) {
		for (pos = 0; pos < 8; pos++) {

			if (!((punteroBitmap[i] >> (7 - pos)) % 2)) {
				osada_block_pointer resultado = i * 8 + pos;

				//Le da el numero de bloque desde donde empiezan los bloques de datos

				editarBloqueEnBitmap(fichero,header,resultado - obtenerInicioTablaBloques(header),1,1);
				pthread_mutex_unlock(&lockBitmap);
				return resultado - obtenerInicioTablaBloques(header);
			}
		}
		pos++;
	}
	pthread_mutex_unlock(&lockBitmap);

	return -1;
}




/**
 * Recibe el file system, el header del mismo y el nombre del archivo del que se quiere recuperar su estructura. SIN PATH
 * Devuelve:
 * 1) El osada file encontrado o NULL si no lo encontro
 */

osada_file* obtenerEstructuraDeArchivo(FILE* fs, osada_header* header, osada_file_pointer posicionPadre, osada_file_pointer* posicion_a_devolver, unsigned char* nombre) {
	osada_file* archivos = obtenerTablaArchivos(fs, header);
	osada_file_pointer i;

	for (i = 0; i < 2048; i++) {
		osada_file archivo = archivos[i];
		pthread_rwlock_rdlock(locks + i);
		char* string = getStringFromNombreOsada(archivo.fname);
		if (strcmp(nombre, string) == 0 && archivo.state!=DELETED && archivo.parent_directory==posicionPadre) {
			osada_file* archivoADevolver = malloc(sizeof(osada_file));
			memcpy(archivoADevolver, (const void*) &archivo,
					sizeof(osada_file));
			free(archivos);
			*posicion_a_devolver = i;
			pthread_rwlock_unlock(locks + i);
			free(string);
			return archivoADevolver;
		}
		free(string);

		pthread_rwlock_unlock(locks + i);
	}
	free(archivos);
	return NULL;
}

/**
 * Escribe el descriptor de archivo indicado por parametro en la posicion indicada por parametro
 * Devuelve 1 si escribio correctamente o
 * -1 si hubo un error
 */

int32_t reemplazarEstructuraDeArchivo(FILE* fs, osada_header* header, osada_file_pointer posicion, osada_file* nuevoDescriptor){

	int salida;
	fseek(fs, (1 + header->bitmap_blocks) * OSADA_BLOCK_SIZE + posicion * sizeof(osada_file), SEEK_SET);
	if (fwrite(nuevoDescriptor, sizeof(osada_file), 1, fs) == 1)
		salida=1;
	else
		salida=-1;
	fflush(fs);
	return salida;


}
/*
* Elimina el archivo indicado en el path. Ademas se indica
* hard = 0: Borra solo el directorio o archivo especificado (si tiene hijos, no borra nada)
* o hard = cualquier otro numero: Si se especifico un directorio, borra tambien sus subdirectorios y archivos
* Devuelve la cantidad de archivos eliminados o
* -1 si hubo error
*/

int32_t eliminarArchivoInterna(FILE* fs, osada_header* header, osada_file* archivos, osada_file_pointer posicion, osada_file* nuevoDescriptor, uint32_t hard){

	osada_file_pointer i;
	int32_t contador=0;
	int32_t contadorHijo=0;
	for (i = 0; i < 2048; i++) {
		if (archivos[i].parent_directory==posicion && archivos[i].state!=DELETED) {
			if (hard){
				if (eliminarArchivo(fs, header, archivos, i, &archivos[i], hard)!=-1)
					contador+=contadorHijo;
			}else
				return DIRECTORIO_NO_VACIO;
		}
	}
	if (nuevoDescriptor->state==REGULAR)
		marcarLiberadosLosBloquesDeBitmap(fs, header, nuevoDescriptor);
	nuevoDescriptor->state=DELETED;
	if (reemplazarEstructuraDeArchivo(fs, header, posicion, nuevoDescriptor)!=-1)
		return contador+1;
	else
		return -1;
}

/**
 * Devuelve la posicion en bytes dentro del filesystem donde se encuentra la primera estructura de archivos
 * libre.
 * -1 Si no encuentra.
 *
 **/

/**
 * Marca liberados los bloques en el bitmap del archivo indicado por parametro
 */

void marcarLiberadosLosBloquesDeBitmap(FILE* fs, osada_header* header, osada_file* descriptor){

	osada_block_pointer* asignaciones = obtenerTablaAsignaciones(fs, header);
	osada_block_pointer bloqueALeer_bq = descriptor->first_block;

	pthread_mutex_lock(&lockBitmap);
	while (bloqueALeer_bq != UINT32_MAX) {

		editarBloqueEnBitmap(fs, header, bloqueALeer_bq, 0,1);
		bloqueALeer_bq = asignaciones[bloqueALeer_bq];
	}
	pthread_mutex_unlock(&lockBitmap);

	free(asignaciones);
}

//sincronizado
int16_t obtenerEstructuraDeArchivoLibre(FILE* archivo, osada_header* header) {
	osada_file* archivos = obtenerTablaArchivos(archivo, header);
	//TODO debe ser thread safe
	int i;
	if(ultimaEntradaLibreDeTabla >= 2047){
		ultimaEntradaLibreDeTabla = 0;
	}
	i = ultimaEntradaLibreDeTabla;

	do{
		pthread_mutex_lock(&obtenerEntradaArchivoLibreMutex);
		if (archivos[i].state == DELETED && entradasLibresOtorgadas[i] == 0) {
				free(archivos);
				ultimaEntradaLibreDeTabla = i + 1;
				entradasLibresOtorgadas[i] = 1;
				pthread_mutex_unlock(&obtenerEntradaArchivoLibreMutex);
				return i;
			}
		i++;
		if(i == 2048){
			i = 0;
		}
		if (i == ultimaEntradaLibreDeTabla)
			break;
		pthread_mutex_unlock(&obtenerEntradaArchivoLibreMutex);

	}while(i != ultimaEntradaLibreDeTabla);


	free(archivos);
	return -1;
}

/**
 * Recibe el file system, el header del mismo, el nombre del archivo que se quiere recuperar,
 * los bytes a leer y el puntero donde devuelve el archivo leido.
 * Devuelve la cadena leida, y en el parametro bytesLeidos, la cantidad de bytes leidos
 */

unsigned char* obtenerArchivoInterna(FILE* fs, osada_header* header, osada_file* descriptor, int32_t bytesALeer, int32_t posicionInicial, uint32_t* byteLeidos_p) {

	osada_block_pointer* asignaciones = obtenerTablaAsignaciones(fs, header);
	uint32_t bytesLeidos = 0;

	unsigned char* bloqueBuffer = malloc(OSADA_BLOCK_SIZE);
	unsigned char* punteroBloqueBuffer;
	unsigned char* archivo = malloc(bytesALeer);
	unsigned char* punteroArchivo = archivo;


	//FEDE, fuse esta pidiendo mas bytes de lo que pesa el archivo
	if((bytesALeer + posicionInicial) > descriptor->file_size ){

		bytesALeer = descriptor->file_size-posicionInicial;
	}

	if (bloqueBuffer == NULL || archivo == NULL) {
		free(bloqueBuffer);
		free(asignaciones);
		free(archivo);
		return NULL;
	}

	osada_block_pointer bloqueALeer_bq = descriptor->first_block;

	while (bytesALeer > 0 || bloqueALeer_bq == UINT32_MAX) {

		if (posicionInicial < OSADA_BLOCK_SIZE) {
			
			
			fseek(fs, bloqueALeer_bq * OSADA_BLOCK_SIZE + obtenerInicioTablaBloques(header) * OSADA_BLOCK_SIZE,
					SEEK_SET);
			fread(bloqueBuffer, OSADA_BLOCK_SIZE, 1, fs);
			punteroBloqueBuffer = bloqueBuffer;

			if (posicionInicial != 0 )
				punteroBloqueBuffer += posicionInicial;

			if (bytesALeer + posicionInicial>= OSADA_BLOCK_SIZE) {
				memmove(punteroArchivo, (const void*) punteroBloqueBuffer, OSADA_BLOCK_SIZE - posicionInicial);
				punteroArchivo += OSADA_BLOCK_SIZE - posicionInicial;
				bytesALeer -= OSADA_BLOCK_SIZE - posicionInicial;
				bytesLeidos+=OSADA_BLOCK_SIZE-posicionInicial;
			} else {
				memmove(punteroArchivo, (const void*) punteroBloqueBuffer, bytesALeer);
				punteroArchivo += bytesALeer;
				bytesLeidos+=bytesALeer;
				bytesALeer = 0;
			}
			posicionInicial=0;
		}else{
			posicionInicial -= OSADA_BLOCK_SIZE;
		}

		bloqueALeer_bq = asignaciones[bloqueALeer_bq];

	}
	free(asignaciones);
	free(bloqueBuffer);
	if (byteLeidos_p != NULL)
		*byteLeidos_p = bytesLeidos;
	return archivo;
}

/**
 * Edita el archivo desde el byte especificado y la cantidad especificada.
 * Devuelve bytes escritos o
 * -1 si hubo error
 * la sincronizacion se realiza desde la funcion superior editarArchivo ya que tambien incluye un mutex sobre la funcion que altera la entrada
 * de la tabla de paginas
 */

int32_t editarArchivoInterna(FILE* fs, osada_header* header, osada_file* descriptor, uint16_t posicion, int32_t bytesAEscribir, int32_t posicionInicial, unsigned char* edicion,char* sockId) {

	//Lo que se pide escribir es mas grande que el archivo y mas grande que el espacio disponible en el filesystem
	if (bytesAEscribir > descriptor->file_size - posicionInicial + obtenerBloquesLibres(fs, header) * OSADA_BLOCK_SIZE)
		return NO_HAY_ESPACIO;

	osada_block_pointer* asignaciones = obtenerTablaAsignaciones(fs, header);
	int32_t bytesEscritos = 0;

	unsigned char* punteroArchivo = edicion;


	osada_block_pointer bloqueAEditar_bq = descriptor->first_block;
	//El archivo no tenia primer bloque (estaba creado con 0 bytes)
	if (bloqueAEditar_bq == UINT32_MAX){
		bloqueAEditar_bq = obtenerPrimerBloqueLibre(fs, header);
		printf("------------\nEl pokedex %s recibio el bloque numero %d\n------",sockId,bloqueAEditar_bq);
		descriptor->first_block = bloqueAEditar_bq;
		asignarSiguienteBloque(fs, header, bloqueAEditar_bq, UINT32_MAX, asignaciones);
	}

	osada_block_pointer siguiente_bloque_bq;

	while (bytesAEscribir > 0) {
		if (posicionInicial < OSADA_BLOCK_SIZE){
			fseek(fs, bloqueAEditar_bq * OSADA_BLOCK_SIZE + obtenerInicioTablaBloques(header) * OSADA_BLOCK_SIZE + posicionInicial,
					SEEK_SET);

			if (bytesAEscribir + posicionInicial>= OSADA_BLOCK_SIZE) {
				fwrite(punteroArchivo, OSADA_BLOCK_SIZE-posicionInicial, 1, fs);
				punteroArchivo += OSADA_BLOCK_SIZE - posicionInicial;
				bytesAEscribir -= OSADA_BLOCK_SIZE - posicionInicial;
				bytesEscritos+=OSADA_BLOCK_SIZE-posicionInicial;
			} else {
				fwrite(punteroArchivo, bytesAEscribir, 1, fs);
				punteroArchivo += bytesAEscribir;
				bytesEscritos+=bytesAEscribir;
				bytesAEscribir = 0;
			}
			posicionInicial=0;
		}else{
			posicionInicial -= OSADA_BLOCK_SIZE;
		}


		siguiente_bloque_bq = asignaciones[bloqueAEditar_bq];

		//Se acabo el archivo, tengo que seguir escribiendo: necesito pedir mas espacio
		if (siguiente_bloque_bq == UINT32_MAX && bytesAEscribir > 0){
			siguiente_bloque_bq = obtenerPrimerBloqueLibre(fs, header);
			printf("------------\nEl pokedex %s recibio el bloque numero %d\n------",sockId,bloqueAEditar_bq);
			asignarSiguienteBloque(fs, header, bloqueAEditar_bq, siguiente_bloque_bq, asignaciones);
			asignarSiguienteBloque(fs, header, siguiente_bloque_bq, UINT32_MAX, asignaciones);

			//Para probar los bloques asignados
			FILE* archivo;
			if (strcmp(descriptor->fname, "sincro1.jpg")==0)
				archivo = fopen("logsincro1.log", "a");
			else
				archivo = fopen("logsincro2.log", "a");

			fprintf(archivo, "%d, ", siguiente_bloque_bq);
			fflush(archivo);
			fclose(archivo);
		}
		bloqueAEditar_bq = siguiente_bloque_bq;

	}
	free(asignaciones);
	fflush(fs);

	return bytesEscritos;
}


/**
 * Trunca el archivo a la cantidad especificada.
 * Devuelve 1 si pudo y
 * -1 si hubo error
 * la sincronizacion se realiza desde la funcion superior editarArchivo ya que tambien incluye un mutex sobre la funcion que altera la entrada
 * de la tabla de paginas
 */

int32_t truncarArchivoInterna(FILE* fs, osada_header* header, osada_file* descriptor, uint32_t offset) {

	//Lo que se pide truncar es mas grande que el archivo y mas grande que el espacio disponible en el filesystem
	if (offset > descriptor->file_size + obtenerBloquesLibres(fs, header) * OSADA_BLOCK_SIZE || offset<0)
		return NO_HAY_ESPACIO;

	osada_block_pointer* asignaciones = obtenerTablaAsignaciones(fs, header);
	osada_block_pointer bloqueAEditar_bq;
	int32_t inicio = offset;
	osada_block_pointer suguiente_bloque_bq=descriptor->first_block;

	//El archivo no tenia ningun bloque asignado
	if (suguiente_bloque_bq==UINT32_MAX && inicio!=0){
		suguiente_bloque_bq = obtenerPrimerBloqueLibre(fs, header);
		descriptor->first_block=suguiente_bloque_bq;
		asignarSiguienteBloque(fs, header, suguiente_bloque_bq, UINT32_MAX, asignaciones);
	}

	while (inicio > 0) {

		if (suguiente_bloque_bq == UINT32_MAX){
			suguiente_bloque_bq = obtenerPrimerBloqueLibre(fs, header);
			asignarSiguienteBloque(fs, header, bloqueAEditar_bq, suguiente_bloque_bq, asignaciones);
			asignarSiguienteBloque(fs, header, suguiente_bloque_bq, UINT32_MAX, asignaciones);
		}

		inicio -= OSADA_BLOCK_SIZE;
		bloqueAEditar_bq=suguiente_bloque_bq;
		suguiente_bloque_bq = asignaciones[suguiente_bloque_bq];
	}

	if (suguiente_bloque_bq != descriptor->first_block)
		asignarSiguienteBloque(fs, header, bloqueAEditar_bq, UINT32_MAX, asignaciones);
	else
		descriptor->first_block=UINT32_MAX;

	pthread_mutex_lock(&lockBitmap);
	while(suguiente_bloque_bq!=UINT32_MAX){

		editarBloqueEnBitmap(fs, header, suguiente_bloque_bq, 0,1);
		suguiente_bloque_bq = asignaciones[suguiente_bloque_bq];

	}
	pthread_mutex_unlock(&lockBitmap);

	free(asignaciones);
	fflush(fs);

	return 1;
}

/**
 * Rellena de espacios un archivo cuando se pide truncarlo mas de su espacio inicialmente asignado.
 * El espacio nuevo contendra espacios.
 */


int32_t rellenarConEspacios(FILE* osada_fs, osada_header* header, osada_file* descriptor, int16_t posicion, int32_t bytesATruncar,char* sockId){
	int largo=bytesATruncar-descriptor->file_size;
	char* edicion = string_repeat('\0', largo);
	int32_t res=editarArchivoInterna(osada_fs, header, descriptor, posicion, bytesATruncar-descriptor->file_size, descriptor->file_size, edicion,sockId);
	free(edicion);
	return res;
}


/**
 * Escribe un directorio o archivo en la tabla de archivos (especificado en el ultimo parametro, 1: archivo 2: directorio
 * Devuelve la posicion donde se grabo en la tabla de archivos o
 * -1 si hay error
 */

int16_t escribirDirectorioInterna(FILE* fs, osada_header* header,
		unsigned char* nombre, osada_file_pointer parentDirectoy, int estructura) {
	osada_file* nuevoDescriptor = malloc(sizeof(osada_file));
	if (nuevoDescriptor == NULL)
		return -1;
	if (estructura == REGULAR || estructura == DIRECTORY)
		nuevoDescriptor->state = estructura;
	else
		return -1;
	if (strlen(nombre)!=17)
		strcpy(&nuevoDescriptor->fname, nombre);
	else
		memcpy(&nuevoDescriptor->fname, nombre, 17);
	nuevoDescriptor->parent_directory = parentDirectoy;
	nuevoDescriptor->file_size = 0;
	time_t horaActual = time( NULL);
	nuevoDescriptor->lastmod = (uint32_t) horaActual;
	nuevoDescriptor->first_block = UINT32_MAX;
	int16_t out= escribirDescriptor(fs, header, nuevoDescriptor);
	free(nuevoDescriptor);
	return out;
}

/**
 * Devuelve una lista de osada_file* que se encuentran en un directorio especificado
 * o NULL si no encuentra ninguna
 */

t_list* listarDirectorio(FILE* fs, osada_file_pointer ubicacionDirectorio, osada_header* header){
	osada_file* archivos = obtenerTablaArchivos(fs, header);

	t_list* archivosADevolver = list_create();

	osada_file_pointer i;
	osada_file_pointer encontrados=0;
	for (i = 0; i < 2048; i++) {
		osada_file archivo = archivos[i];
		if (archivo.parent_directory==ubicacionDirectorio && archivo.state!=DELETED) {
			encontrados++;
			//archivosADevolver= (osada_file**) realloc(archivosADevolver, sizeof(osada_file*) * encontrados);
			osada_file* nuevoArchivo = malloc(sizeof(osada_file));
			memcpy(nuevoArchivo, &archivo, sizeof(osada_file));
			list_add(archivosADevolver, nuevoArchivo);
		}
	}
	free(archivos);
	return archivosADevolver;
}

/**
 *  Escribe el descriptor del archivo.
 *  Devuelve la posicion en la tabla de archivos donde se grabo
 *  -1 si hay error
 */

int16_t escribirDescriptor(FILE* fs, osada_header* header,
		osada_file* descriptor) {
	osada_file_pointer posicion = obtenerEstructuraDeArchivoLibre(fs, header);

	if (posicion == -1)
		return posicion;

	pthread_rwlock_wrlock(locks + posicion);
	osada_block_pointer inicioTablaArchivos_b = (header->bitmap_blocks + 1)
			* OSADA_BLOCK_SIZE;
	//Posiciona el puntero de archivo para escribir el nuevo descriptor
	if (fseek(fs, inicioTablaArchivos_b + posicion * sizeof(osada_file),
			SEEK_SET)) {
		free(descriptor);
		entradasLibresOtorgadas[posicion] = 0;
		pthread_rwlock_unlock(locks + posicion);
		return -1;
	}
	//Escribe el nuevo descriptor en la tabla de archivos
	if (fwrite(descriptor, sizeof(osada_file), 1, fs) != 1) {
		free(descriptor);
		entradasLibresOtorgadas[posicion] = 0;
		pthread_rwlock_unlock(locks + posicion);
		return -1;
	}
	fflush(fs);
	entradasLibresOtorgadas[posicion] = 0;
	pthread_rwlock_unlock(locks + posicion);
	return posicion;
}

/**
 * Recibe 1 si ya fue muteado el lockBitmap, si no un 0
 * Marca el bloque indicado como ocupado o desocupado segun el parametro ocupado.
 * Devuelve el byte modificado en caso satisfactorio,
 * -1 si hay error
 *
 **/

int32_t editarBloqueEnBitmap(FILE* fs, osada_header* header, osada_block_pointer nBloque_bq, int ocupado,int muteado) {
	//Empieza a contar desde los bloques de datos
	nBloque_bq += obtenerInicioTablaBloques(header);
		uint32_t numeroByte = nBloque_bq / 8;
		uint32_t numeroBit = nBloque_bq - numeroByte * 8;

		if(muteado == 0){
				pthread_mutex_lock(&lockBitmap);
		}

		unsigned char* bitmap = obtenerBitmap(fs, header);
			unsigned char byteAModificar = bitmap[numeroByte];
			unsigned char factor = 128;
			factor = factor >> numeroBit;


			if (ocupado)
				byteAModificar = byteAModificar | factor;
			else
				byteAModificar = byteAModificar & ~factor;


			if (fseek(fs, OSADA_BLOCK_SIZE + numeroByte, SEEK_SET)) {
				if(muteado == 0) pthread_mutex_unlock(&lockBitmap);
				return -1;
			}


			if (fwrite((const void*) &byteAModificar, sizeof(unsigned char), 1, fs) != 1) {
				if(muteado == 0)pthread_mutex_unlock(&lockBitmap);
				return -1;
			}
			*(bitmap+numeroByte) = byteAModificar;
			fflush(fs);
			if(muteado == 0)pthread_mutex_unlock(&lockBitmap);
			return 0;






}
/**
 * Asigna a la posicion indicada, el bloque indicado en el valor
 * Devuelve 0 en caso satisfactorio, -1 si hubo error
 */

uint32_t asignarSiguienteBloque(FILE* fs, osada_header* header, osada_block_pointer posicion, osada_block_pointer valor, osada_block_pointer* asignaciones) {

	pthread_mutex_lock(&mutexAsignacion);
	if (fseek(fs, header->inicio_tabla_asignaciones * OSADA_BLOCK_SIZE + posicion * sizeof(osada_block_pointer), SEEK_SET)){

		pthread_mutex_unlock(&mutexAsignacion);
		return -1;
	}

	if (fwrite((const void*) &valor, sizeof(osada_block_pointer), 1, fs) != 1){
		pthread_mutex_unlock(&mutexAsignacion);
		return -1;
	}
	fflush(fs);
	asignaciones[posicion] = valor;
	pthread_mutex_unlock(&mutexAsignacion);
	return 0;
}

/**
 * Devuelve un puntero con la informacion de la estructura bitmap
 **/

unsigned char* obtenerBitmap(FILE* fs, osada_header* header) {

	if (punteroBitmap_singleton==NULL){

		uint32_t tamanioBitMapEnBytes = header->bitmap_blocks * OSADA_BLOCK_SIZE;
		punteroBitmap_singleton = malloc(tamanioBitMapEnBytes);

		if (fseek(fs, OSADA_BLOCK_SIZE, SEEK_SET) != 0) {
			free(punteroBitmap_singleton);
			return NULL;
		}

		if (fread((void*) punteroBitmap_singleton, sizeof(unsigned char),
				tamanioBitMapEnBytes, fs) != tamanioBitMapEnBytes) {
			free(punteroBitmap_singleton);
			return NULL;
		}
	}
	return punteroBitmap_singleton;
}

/**
 * Devuelve un puntero con la informacion de la tabla de asignaciones
 **/

uint32_t* obtenerTablaAsignaciones(FILE* fs, osada_header* header) {
	osada_block_pointer* asignaciones = malloc(sizeof(uint32_t) * header->data_blocks);
	if (asignaciones == NULL)
		return NULL;
	fseek(fs, header->inicio_tabla_asignaciones * OSADA_BLOCK_SIZE, SEEK_SET);
	fread(asignaciones, sizeof(uint32_t), header->data_blocks, fs);
	return asignaciones;
}

/**
 * Devuelve un puntero con la informacion de la tabla de archivos
 **/

osada_file* obtenerTablaArchivos(FILE* fs, osada_header* header) {
	osada_file* archivos = malloc(sizeof(osada_file) * 2048);
	fseek(fs, (1 + header->bitmap_blocks) * OSADA_BLOCK_SIZE, SEEK_SET);
	fread(archivos, sizeof(osada_file), 2048, fs);
	return archivos;
}

/**
 * Devuelve el bloque donde inicia los bloques dentro del file system
 *
 */

osada_block_pointer obtenerInicioTablaBloques(osada_header* header) {
	return header->inicio_tabla_asignaciones + obtenerTamanioAsignaciones(header);
}

/**
 * Devuelve el tamanio en bloques de la tabla de asignaciones
 */

osada_block_pointer obtenerTamanioAsignaciones(osada_header* header) {
	osada_block_pointer ret = ((header->fs_blocks - header->bitmap_blocks -1 - OSADA_TABLA_DE_ARCHIVOS) * sizeof(uint32_t)) / OSADA_BLOCK_SIZE;
	if (((header->fs_blocks - header->bitmap_blocks -1 - OSADA_TABLA_DE_ARCHIVOS) * sizeof(uint32_t)) / OSADA_BLOCK_SIZE % OSADA_BLOCK_SIZE)
		ret++;
	return ret;
}

/**
 * Devuelve la estructura del archivo/directorio encontrado
 * NULL si no lo encontró
 * Adicionalmente devuelve en la variable resultado:
 * 0 si el path es valido y el archivo/directorio existe
 * 1 si el path es valido pero el ultimo archivo/directorio no existe
 * 2 si el path es invalido
 */


osada_file* leerPath(FILE* osada_fs, unsigned char* path, int32_t* _resultado, osada_file_pointer* posicion) {
	osada_file_pointer parentDirectory = 0xFFFF;
	osada_file_pointer* posicionArrayArchivos = malloc(sizeof(uint16_t));

	osada_header* header = leerHeaderOsada(osada_fs);
	//Por defecto, el archivo existe
	int32_t resultado=ARCHIVO_EXISTE;

	if (posicionArrayArchivos == NULL)
		return -1;

	osada_file* directory = malloc(sizeof(osada_file));
	directory->state=DIRECTORY;
	uint16_t posicionBarra = 0xFFFF;
	strcpy(&directory->fname, "/");
	if (posicion != NULL)
		*posicion = posicionBarra;
	directory->parent_directory = posicionBarra;
	*posicionArrayArchivos = posicionBarra;

	if (strcmp(path, "/") == 0){
		if (_resultado!=NULL)
			*_resultado = resultado;
		return directory;
	}

	unsigned char** partes = string_split(path, "/");

	int indicePartes=-1;
	int ultimaEstructura=0;


	while (1) {

		if (ultimaEstructura){
			resultado=PATH_INVALIDO;
			break;
		}

		if (directory==NULL ){
			ultimaEstructura=1;
			resultado=ARCHIVO_NO_PATH_VALIDO;
		}else if (directory->state==REGULAR){
			ultimaEstructura=1;
		}

		indicePartes++;

		if (partes[indicePartes] == NULL)
			break;

		parentDirectory = *posicionArrayArchivos;

		directory = obtenerEstructuraDeArchivo(osada_fs, header, parentDirectory,
				posicionArrayArchivos, partes[indicePartes]);

	}

	if (_resultado!=NULL)
		*_resultado = resultado;
	if (posicion!=NULL)
		*posicion = *posicionArrayArchivos;
	free(posicionArrayArchivos);
	return directory;
}

unsigned char* obtenerNombreDeArchivoDePath(unsigned char* path){

	if (strcmp(path, "/")==0)
		return path;

	unsigned char** partes = string_split(path, "/");

	int indicePartes = 0;

	while (partes[indicePartes] != NULL) {
		indicePartes++;
	}
	return partes[indicePartes-1];
}


int32_t obtenerCantidadDeArchivos(FILE * fs){
 osada_header* header =	leerHeaderOsada(fs);
 osada_file* archivos =	obtenerTablaArchivos(fs,header);
 int i;
 int contador=0;
 for(i=0;i<2048;i++){
	 osada_file archivo = archivos[i];
	 if(archivo.state == REGULAR || archivo.state == DIRECTORY ){
		 contador ++;
	 }
 }
 free(archivos);
 return contador;

}

uint32_t obtenerSegundosFechaActual(){
	return (uint32_t) time(NULL);
}

char* getStringFromNombreOsada(unsigned char nombre[OSADA_FILENAME_LENGTH]){
	int i;
	for (i=0;i<7;i++){
		if (nombre[i] == '\0'){
			char* string = string_new();
			string_append(&string, nombre);
			return string;
		}
	}
	char* string = malloc(18);
	memcpy(string, nombre, 17);
	char caracter = '\0';
	memcpy(string+17, &caracter, 1);
	return string;

}
