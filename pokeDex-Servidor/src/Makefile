CC := gcc
CFLAGS :=  -Wall


OBJS :=  pokedex.o  globales.o  conexiones.o  lectorOsada.o funcionesOsada.o tests.o
HDRS := globales.h conexiones.h FuncionesOsada.h LectorOsada.h OsadaTest.h




#ALL
all:  pokedexServidor


#pokedexServidor
pokedexServidor:  $(OBJS)
    gcc  -o pokedex $(OBJS)


pokedex.o : PokeDex.c  PokeDex.h
  $(CC) -c $(CFLAGS) PokeDex.c



globales.o : globales.c  globales.h
        $(CC) -c $(CFLAGS) globales.c


conexiones.o: conexionesPokeServidor.c conexiones.h
          $(CC) -c $(CFLAGS) conexionesPokeServidor.c


lectorOsada.o : LectorOsada.h LectorOsada.c
		  $(CC) -c $(CFLAGS) ./osada/LectorOsada.c


funcionesOsada.o : FuncionesOsada.h FuncionesOsada.c
		  $(CC) -c $(CFLAGS) ./osada/FuncionesOsada.c


tests.o : OsadaTest.h OsadaTest.c
		  $(CC) -c $(CFLAGS) ./osada/OsadaTest.c