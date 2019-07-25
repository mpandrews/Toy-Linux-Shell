CXX = gcc
CFLAGS = -std=gnu11 -Wall

.PHONY: clean
.PHONY: default

default : smallsh
	
smallsh : smallsh.o smallsh-u.o smallsh-sig.o
	${CXX} ${CFLAGS} -o smallsh smallsh.o smallsh-u.o smallsh-sig.o
smallsh.o : smallsh.c
	${CXX} ${CFLAGS} -c smallsh.c
smallsh-u.o : smallsh-u.c smallsh-u.h
	${CXX} ${CFLAGS} -c smallsh-u.c
smallsh-sig.o : smallsh-sig.c smallsh-sig.h
	${CXX} ${CFLAGS} -c smallsh-sig.c
clean:
	rm ./smallsh ./smallsh.o ./smallsh-u.o ./smallsh-sig.o
