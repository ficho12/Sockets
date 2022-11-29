 #!/bin/sh
ps -ef | grep servidor | grep -v grep | awk '{print $2}' | xargs -r kill -9
make
rm logs2/*
cp -R logs/* logs2/
rm logs/*
