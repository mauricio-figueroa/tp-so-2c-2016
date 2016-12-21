#include <protocolos/p_pokeDex.h>
#include "LectorOsada.h"
#include <commons/collections/list.h>
#ifndef OSADA_FUNCIONESOSADA_H_
#define OSADA_FUNCIONESOSADA_H_


//File system compartido para todas las funciones
unsigned char* osada_leerArchivo(unsigned char* path, uint32_t bytesALeer, uint32_t posicionInicial, uint32_t* bytesLeidos_p, FILE* osada_fs);
int16_t osada_crearArchivo(unsigned char* path, FILE* osada_fs);
int16_t osada_crearDirectorio(unsigned char nombre[OSADA_FILENAME_LENGTH], FILE* osada_fs);
int32_t osada_existeArchivo(unsigned char* path, FILE* osada_fs);
t_list* osada_estructuraDirectorio(unsigned char* path, uint32_t* cantidad, FILE* osada_fs);
int32_t osada_renombrarArchivo(unsigned char* path, unsigned char nuevoNombre[OSADA_FILENAME_LENGTH] , FILE* osada_fs);
int32_t osada_eliminarArchivo(unsigned char* path, uint32_t hard, FILE* osada_fs);
directory_attr* osada_getAtributosArchivo(unsigned char* path, FILE* osada_fs);
int32_t osada_editarArchivo(unsigned char* path, int32_t inicioBytes, int32_t bytesAEscribir, unsigned char* archivo,char* sockId, FILE* osada_fs);

#endif /* OSADA_FUNCIONESOSADA_H_ */
