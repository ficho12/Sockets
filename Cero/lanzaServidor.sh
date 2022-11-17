# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
# las ordenes est�n en un fichero que se pasa como tercer par�metro
./servidortcp
./clientcp localhost ordenes.txt &
./clientcp localhost ordenes1.txt &
./clientcp localhost ordenes2.txt &
#./servidorudp
#./clientudp localhost ordenes.txt &
#./clientudp localhost ordenes1.txt &
#./clientudp localhost ordenes2.txt &

 