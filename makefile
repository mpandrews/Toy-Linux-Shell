CXX = gcc
CFLAGS = -std=gnu11 -Wall

.PHONY: clean
.PHONY: default

default : smallsh
	
smallsh : smallsh.o smallsh-u.o
	${CXX} ${CFLAGS} -o smallsh smallsh.o smallsh-u.o
smallsh.o : smallsh.c
	${CXX} ${CFLAGS} -c smallsh.c
smallsh-u.o: smallsh-u.c
	${CXX} ${CFLAGS} -c smallsh-u.c
clean:
	rm ./smallsh ./smallsh.o ./smallsh-u.o
