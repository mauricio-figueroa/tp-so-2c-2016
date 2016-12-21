# Equipo Rocket


Ash /home/utnso/Escritorio/mnt/pokedex                        
${workspace_loc:rocketLibrary/Debug}                        
/home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-/rocketLibrary/Debug                        
Azul /home/utnso/Escritorio/mnt/pokedex


Signals

$ kill -s VALUE PID

Para obtener el PID del mapa 'ps -ef | grep Mapa'


   Señal       		  	     value 
SIGTERM     		15                 //usada para hacerle ganar una vida al entrenador
SIGUSR1   			10  // usada para hacerle perder una vida al entrenador
SIGUSR2  			12  //es la que usamos para avisarle el mapa que recargue su archivo de metadata

Ej-
kill -s 31 16564  -------> es el pid del proceso Mapa


Mapa: 0 si es FIFO, 1 RR, 2 SRDF


./PokeDex-Cliente -d -s ./mnt --ip "192.168.3.72" --puerto "4000"