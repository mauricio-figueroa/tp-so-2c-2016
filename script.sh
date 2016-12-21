#!/bin/bash
cd /home/utnso/Escritorio

git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install

cd /home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-/rocketLibrary/Debug
make clean
make all

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-/rocketLibrary/Debug

cd /home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-
cd pokeDex-Servidor/Debug
make clean
make all

cd /home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-/pokeDex-Cliente/Debug
make clean
make all

cd /home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-/Entrenador/Debug
sudo apt-get update
sudo apt-get install ncurses-dev

cd /home/utnso/Escritorio
git clone https://github.com/sisoputnfrba/so-nivel-gui-library
cd so-nivel-gui-library
make && make install

cd /home/utnso/Escritorio
git clone https://github.com/sisoputnfrba/so-pkmn-utils
cd so-pkmn-utils/src
make all

cd /home/utnso/Escritorio/tp-2016-2c-Equipo-Rocket-




