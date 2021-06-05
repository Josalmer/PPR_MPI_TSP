all: bbpar bbseq

bbseq: bbseq.o libbbseq.o
	mpicxx -O3 bbseq.o libbbseq.o -o bbseq

bbseq.o: bbseq.cc
	mpicxx -O3 -c bbseq.cc

libbbseq.o: libbbseq.cc libbbseq.h
	mpicxx -O3 -c  libbbseq.cc 

bbpar: bbpar.o libbb.o
	mpicxx -O3 bbpar.o libbb.o -o bbpar
	
bbpar.o: bbpar.cc
	mpicxx -O3 -c bbpar.cc

libbb.o: libbb.cc libbb.h
	mpicxx -O3 -c  libbb.cc 

clean:
	/bin/rm -f *.o bbpar bbseq
