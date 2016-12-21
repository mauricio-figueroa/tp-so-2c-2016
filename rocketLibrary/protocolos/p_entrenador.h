#ifndef PROTOCOLOS_P_ENTRENADOR_H_
#define PROTOCOLOS_P_ENTRENADOR_H_

typedef struct punto {
	int puntox;
	int puntoy;
} punto;

typedef enum {
	CONOCER_POKENEST = 1,
	MOVER_POSICION = 2,
	CAPTURAR = 3,
	TERMINE_OBJETIVOS = 4
} t_mensajes_entrenador;

typedef enum {
	IZQUIERDA = 1,
	DERECHA = 2,
	ABAJO = 3,
	ARRIBA = 4,
	SIN_DIRECCION = 5,
	SOBRE_POKENEST = 6
} direccionMovimiento;

#endif /* PROTOCOLOS_P_ENTRENADOR_H_ */
