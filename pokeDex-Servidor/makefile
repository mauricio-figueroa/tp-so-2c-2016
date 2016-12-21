all: PokeServer

PokeServer: pokeServer.o
			gcc -c ./src/PokeDex.c
			gcc -c ./src/conexionesPokeServidor.c
			gcc -c ./src/globales.c
			gcc -o PokeServer PokeServer.o pokeServer.o -lcommons -lpthread
			rm -R pokeServer.o
			rm -R PokeServer.o

clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) CPU
	-@echo ' '
	
			