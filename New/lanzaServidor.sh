#!/bin/sh
# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
# las ordenes est�n en un fichero que se pasa como tercer par�metro
#./servidor
./clientcp localhost TCP ordenes.txt &
./clientcp localhost TCP ordenes1.txt &
./clientcp localhost TCP ordenes2.txt &
#./clientcp localhost UDP ordenes.txt &
#./clientcp localhost UDP ordenes1.txt &
#./clientcp localhost UDP ordenes2.txt &