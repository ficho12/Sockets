#!/bin/sh
# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
# las ordenes est�n en un fichero que se pasa como tercer par�metro
#./servidor
#./client localhost TCP ordenes.txt &
#./client localhost TCP ordenes1.txt &
#./client localhost TCP ordenes2.txt &
./client localhost UDP ordenes.txt &
./client localhost UDP ordenes1.txt &
./client localhost UDP ordenes2.txt &