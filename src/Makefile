# makefile for open source (LGPL) lasvalidate
#
#COPTS    = -g -Wall -Wno-deprecated -DDEBUG 
COPTS     = -O3 -Wall -Wno-deprecated -DNDEBUG 
#COMPILER  = CC
COMPILER  = g++
LINKER  = g++
#BITS     = -64

#LIBS     = -L/usr/lib64
#LIBS     = -L/usr/lib32
#INCLUDE  = -I/usr/include

LASLIBS     = -L../laslibrary/lib
LASINCLUDE  = -I../laslibrary/inc

all: lasvalidate

lasvalidate: lasvalidate.o xmlwriter.o
	${LINKER} ${BITS} ${COPTS} lasvalidate.o xmlwriter.o -llaslibrary -o $@ ${LIBS} ${LASLIBS} $(INCLUDE) $(LASINCLUDE)
	cp $@ ../bin

.cpp.o: 
	${COMPILER} ${BITS} -c ${COPTS} ${INCLUDE} $(LASINCLUDE) $< -o $@

.c.o: 
	${COMPILER} ${BITS} -c ${COPTS} ${INCLUDE} $(LASINCLUDE) $< -o $@

clean:
	rm -rf *.o
	rm -rf lasvalidate

clobber:
	rm -rf *.o
	rm -rf lasvalidate
	rm -rf *~
