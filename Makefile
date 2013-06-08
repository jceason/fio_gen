CC=gcc
CPP=g++
LIB=-lpthread
OPT=

default: bin/fio_gen 

bin: 
	mkdir bin	

bin/fio_gen: src/fio_gen.cpp bin
	${CPP} -DLINUX ${OPT} src/fio_gen.cpp -o bin/fio_gen ${LIB} 

clean:
	-rm -f bin/*	
