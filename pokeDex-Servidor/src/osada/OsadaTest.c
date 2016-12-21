/**
 * Asegurarse antes de testear que el destino del filesystem sea un archivo OSADA valido, no importa que tengas datos o no.
 */
#include "OsadaTest.h"
#include "FuncionesOsada.h"
#include <time.h>




/**
 * Crea un directorio
 */

void testearEscribirDirectorio(){

	FILE* osada_fs = fopen("challenge2.bin", "rb+");

	unsigned char* nombre=malloc(23);
	strcpy(nombre, "directorio/papa");

	osada_crearDirectorio( nombre, osada_fs);

}

void truncarArchivoTest(){
	unsigned char* nombre=malloc(13);
	strcpy(nombre, "/truncame.txt");
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	osada_file* descriptor = leerPath(osada_fs, nombre, NULL, NULL);
	recorrerArchivo(nombre, 2, descriptor->file_size, 0);
	printf("El archivo tiene %d bytes \n", descriptor->file_size);

	osada_TruncarArchivo(nombre, 300);
}


/**
 * Escribe un archivo desde cero, la estructura de archivos, el contenido y los valores en la tabla de asignaciones.
 */

void testearEscrituraArchivoConDirectorios(){

	unsigned char* archivo1=malloc(129);
	unsigned char* archivo2=malloc(152);
	strcpy(archivo1, "Esto es un archivo que deberia ocupar mas de un bloque ya que en total suma 129 bytes que deberian grabarse en 2 bloques!!!!!!!!!");
	strcpy(archivo2, "Esto es un archivo que deberia ocupar mas de un bloque ya que en total suma 152 bytes que deberian grabarse en 3 bloques, aunque el ultimo no lo llene!");
	unsigned char* nombre1=malloc(18);
	strcpy(nombre1, "directorio/129.txt");
	unsigned char* nombre2=malloc(18);
	strcpy(nombre2, "directorio/152.txt");

	FILE* osada_fs = fopen("challenge2.bin", "rb+");

	osada_crearArchivo(nombre1, osada_fs);
	osada_editarArchivo(nombre1, 0, 129, archivo1,"test", osada_fs);
	//osada_escribirArchivo(nombre2, 152, archivo2);

	free(archivo1);
	free(archivo2);
	free(nombre1);
	free(nombre2);

}

/**
 * Testea la funcionalidad para ver si existe un archivo determinado
 */

void testearExisteArchivo(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	unsigned char* nombre1=malloc(23);
	strcpy(nombre1, "directorio1/Tercero.txt");

	osada_existeArchivo(nombre1, osada_fs);

}

/**
 * Recorre le filesystem, archivo por archivo, directorio por directorio. Muestra sus contenidos y toda su jerarquia de directorios.
 */

void recorrerFileSystem(){
	FILE* osada_fs = fopen("disco.bin", "rb");
	char* barra = string_new();
	string_append(&barra, "/");
	recorrerDirectorio(barra, osada_fs, 0);
}

/**
 * Recorre un directorio, todos sus sibdirectorios y sus archivos
 */

void recorrerDirectorio(unsigned char* path, FILE* fs, int nivel){
	uint32_t* resultado = malloc(sizeof(uint32_t));
	uint16_t* posicion = malloc(sizeof(uint16_t));
	leerPath(fs, path, resultado, posicion);
	int y;
	for (y = 0; y<nivel;y++)
		printf( "--");
	unsigned char* hola =obtenerNombreDeArchivoDePath(path);
	printf("%s \n", hola);
	fflush(stderr);
	osada_header* header = leerHeaderOsada(fs);
	t_list* lista = listarDirectorio(fs, *posicion, header);
	int i = 0;
	osada_file* archivo = (osada_file*) list_get(lista, i);
	while (archivo != NULL && *resultado == 0){
		char* nuevoPath = string_new();
		string_append(&nuevoPath, path);
		if (nivel!=0)
			string_append(&nuevoPath, "/");
		string_append(&nuevoPath, archivo->fname);
		if (archivo->state == DIRECTORY){
			recorrerDirectorio(nuevoPath, fs, nivel+1);
		}else if(archivo->state == REGULAR){
			printf("\n");
			for (y = 0; y<nivel+1;y++)
					printf("--");
				printf("%s (%d bytes) \n", obtenerNombreDeArchivoDePath(nuevoPath), archivo->file_size);
				fflush(stderr);
			//recorrerArchivo(nuevoPath, nivel+1, archivo->file_size, 0);
		}
		i++;
		archivo = (osada_file*) list_get(lista, i);
	}
}

/**
 * Recorre un archivo y lo muestra por pantalla
 */

void recorrerArchivo(char* path, int nivel, int cantidadALeer, int posicionInicial){

	FILE* osada_fs = fopen("challenge2.bin", "rb+");

	uint32_t* bytesLeidos = malloc(sizeof(uint32_t));
	int y;
	unsigned char* archivo = osada_leerArchivo(path, cantidadALeer, posicionInicial, bytesLeidos, osada_fs);

	int i;
	for (i=0;i<*bytesLeidos;i++)
		printf("%c", archivo[i]);
	printf("\n");
	fflush(stderr);

}

/**
 * Lee un archivo de a pedacitos, con un bucle utilizando tramos de largo arbitrarios.
 */

void leerArchivosDeATramosRandom(char* path){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	int32_t* resultado = malloc(sizeof(int32_t));
	osada_file* archivo = leerPath(osada_fs, path, resultado, NULL);


	int bytesALeer = archivo->file_size;
	int inicio = 0;
	int r;

	while (inicio<archivo->file_size){
		srand(time(NULL));
		r = rand() % 30;
		if (inicio+r>archivo->file_size)
			r = archivo->file_size-inicio;
		recorrerArchivo(path, 2, r, inicio);
		inicio+=r;
	}
}

/**
 * Borra el directorio especificado, sin eliminar su contenido
 *
 */

void borrarDirectorio(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	unsigned char* nombre=malloc(23);
	strcpy(nombre, "directorio/archivo.txt");
	osada_eliminarArchivo(nombre, 0, osada_fs);
}

/**
 * Edita el archivo especificado y le mete una cadena por el medio
 */

void editarArchivo(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	unsigned char* nombre=malloc(18);
	strcpy(nombre, "directorio/128.txt");
	unsigned char* archivo=malloc(30);
	strcpy(archivo, "Estos treinta bytes deberian!!");
	osada_editarArchivo(nombre, 60, 30, archivo,"test", osada_fs);
}

/**
 * Edita un archivo mas alla del largo del archivo
 */

void editarArchivoAMasDeSuContenido(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	unsigned char* nombre=malloc(18);
	strcpy(nombre, "directorio/129.txt");
	unsigned char* archivo=malloc(331);
	strcpy(archivo, "Este archivo ahora va a ocupar muchos de 128 bytes para probar la funcionalidad de que pida mas bloques. Tengo que seguir escribiendo para que ocupe mas lugar, igual me parece que ya pase los 128 bytes, asi que voy a dejar de escribir en un rato. Aca hay gente comiendo panchos. El clima esta muy calido, pero aca dentro esta lindo");
	osada_editarArchivo(nombre, 60, 331, archivo,"test", osada_fs);
	free(nombre);
	free(archivo);
}

/**
 * Escribe nuevamente el archivo, de a tramos de un largo arbitrario
 */

void editarArchivoTramosRandom(){

	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	int tamanioArchivo = 2654;

	unsigned char* nombre=malloc(34);
	strcpy(nombre, "directorio/subdirectorio/large.txt");
	unsigned char* archivo= osada_leerArchivo(nombre, tamanioArchivo, 0, NULL, osada_fs);

	int inicio = 0;
	int r ;

	while (inicio<tamanioArchivo){
		srand(time(NULL));
		r = rand() % 5;
		if (inicio+r>tamanioArchivo)
			r = tamanioArchivo-inicio;
		osada_editarArchivo(nombre, inicio, r, archivo+inicio,"test", osada_fs);
		inicio+=r;
	}
}

/**
 * Renombra un archivo especificado
 */

void renombrarArchivoTest(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	unsigned char* nombre1=malloc(18);
	strcpy(nombre1, "/directorio/129.txt");
	unsigned char* nombre2=malloc(18);
	strcpy(nombre2, "/directorio/128.txt");

	osada_renombrarArchivo(nombre1, nombre2, osada_fs);
}

void probarMapaDeBits(){
	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	osada_header* header = leerHeaderOsada(osada_fs);
	while (1){
		int32_t inicioBloques = obtenerInicioTablaBloques(header);
		int32_t proximoLibre = obtenerPrimerBloqueLibre(osada_fs, header);
		editarBloqueEnBitmap(osada_fs, header, proximoLibre, 1,0);
	}
}

void llenarFileSystem(){

	FILE* osada_fs = fopen("challenge2.bin", "rb+");
	osada_crearArchivo("/archGrande", osada_fs);


	int i;
	for(i=0;i<4019712;i= i+6400){
		osada_editarArchivo("/archGrande",i,6400,"hola","test", osada_fs);

	}


}



void llenarTablaDeArchivosDeCheto(){
	int i;
	FILE* osada_fs = fopen("disco.bin", "rb+");
	for(i=0;i<2049;i++){
		char str[10];
		sprintf(str,"/%d.txt",i);

		int result = osada_crearArchivo(str, osada_fs);
		if(result == -2){
			printf("TABLA DE ARCHIVOS LLENA!!");
			break;
		}
	}


}

