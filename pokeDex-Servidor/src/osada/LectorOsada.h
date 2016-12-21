#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <commons/collections/list.h>

#ifndef OSADA_LECTOROSADA_H_

#define OSADA_LECTOROSADA_H_
#define OSADA_BLOCK_SIZE 64
#define OSADA_TABLA_DE_ARCHIVOS 1024
#define OSADA_PADDING 40
#define OSADA_FILENAME_LENGTH 17

typedef unsigned char osada_block[OSADA_BLOCK_SIZE];
typedef uint32_t osada_block_pointer;
typedef uint16_t osada_file_pointer;

#pragma pack(push, 1)

typedef struct {
	unsigned char magic_number[7]; // OSADAFS
	uint8_t version;
	uint32_t fs_blocks; // cantidad total de bloques
	uint32_t bitmap_blocks; // tamanio del bitmap (en bloques)
	osada_block_pointer inicio_tabla_asignaciones; // inicio tabla de asignaciones (bloque numero...)
	uint32_t data_blocks; // tamnio de datos (en bloques)
	unsigned char padding[40]; // relleno
} osada_header;

//Static_assert( sizeof(osada_header) == 64, "osada_header size does not match osada_block size");

typedef enum __attribute__((packed)) {
    DELETED = '\0',
    REGULAR = '\1',
    DIRECTORY = '\2',
} osada_file_state;

//Static_assert( sizeof(osada_file_state) == 1, "osada_file_state is not a char type");

typedef struct {
	osada_file_state state;
	unsigned char fname[OSADA_FILENAME_LENGTH]; //Nombre de archivo, si es menor de 17 caracteres, se rellena con 0
	uint32_t file_size;
	osada_file_pointer parent_directory;
	uint32_t lastmod;
	osada_block_pointer first_block;
} osada_file;

typedef enum resultados{
	ARCHIVO_EXISTE=0,
	ARCHIVO_NO_PATH_VALIDO=1,
	PATH_INVALIDO=2,
} resultados;

//_Static_assert( sizeof(osada_file) == (sizeof(osada_block) / 2.0), "osada_file size does not half osada_block size");


osada_file* obtenerEstructuraDeArchivo(FILE* fs, osada_header* header, osada_file_pointer posicionPadre, osada_file_pointer* posicion_a_devolver, unsigned char* nombre);
osada_header* crearHeaderOsadaPrueba();
unsigned char* obtenerBitmap(FILE* fs, osada_header* header);
uint32_t* obtenerTablaAsignaciones(FILE* fs, osada_header* header);
osada_file* obtenerTablaArchivos(FILE* fs, osada_header* header);
uint32_t obtenerInicioTablaBloques(osada_header* header);
uint32_t obtenerTamanioAsignaciones(osada_header* header);
int32_t editarBloqueEnBitmap(FILE* fs, osada_header* header, uint32_t nBloque_bq, int ocupado, int muteado);
uint32_t asignarSiguienteBloque(FILE* fs, osada_header* header, osada_block_pointer posicion, osada_block_pointer valor, osada_block_pointer* asignaciones);
int16_t escribirDescriptor(FILE* fs, osada_header* header, osada_file* descriptor);
osada_header* leerHeaderOsada(FILE* fichero);
osada_file* leerPath(FILE* osada_fs, unsigned char* path, int32_t* _resultado, osada_file_pointer* posicion);
t_list* listarDirectorio(FILE* fs, osada_file_pointer ubicacionDirectorio, osada_header* header);
int32_t reemplazarEstructuraDeArchivo(FILE* fs, osada_header* header, osada_file_pointer posicion, osada_file* nuevoDescriptor);
int32_t eliminarArchivoInterna(FILE* fs, osada_header* header, osada_file* archivos, osada_file_pointer posicion, osada_file* nuevoDescriptor, uint32_t hard);
unsigned char* obtenerNombreDeArchivoDePath(unsigned char* path);
int32_t editarArchivoInterna(FILE* fs, osada_header* header, osada_file* descriptor, osada_file_pointer posicion, int32_t bytesAEscribir, int32_t posicionInicial, unsigned char* edicion,char* sockId);
int32_t truncarArchivoInterna(FILE* fs, osada_header* header, osada_file* descriptor, uint32_t offset);
int32_t obtenerCantidadDeArchivos(FILE * fs);
uint32_t obtenerSegundosFechaActual();
char* getStringFromNombreOsada(unsigned char nombre[OSADA_FILENAME_LENGTH]);
#pragma pack(pop)


#endif /* OSADA_LECTOROSADA_H_ */
