 ## Ejecutar el servidor en 2º plano y ejecutar sh lanzaServidor.sh
#Cuando se quiera matar el servidor ejecutar sh mataProceso.sh
# mataProceso.sh mata al Servidor, compila, y pasa los logs a logs2
#ps -ef | grep servidor | grep -v grep | awk '{print $2}' | xargs -r kill -9 #no funciona en nogal
killall servidor -u i4292873
killall cliente -u i4292873
make
mkdir logs2
rm logs2/*
cp -R logs/* logs2/
rm logs/*
sh lanzaServidor.sh