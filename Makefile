CC=gcc
CPP=g++
LIB=-lpthread
OPT=

default: bin/fio_gen 
	
bin/fio_gen: src/fio_gen.cpp
	${CPP} -DLINUX ${OPT} ${LIB} src/fio_gen.cpp -o bin/fio_gen  

clean:
	-rm -f bin/*	
