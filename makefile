CXX = gcc
CFLAGS = -Wall

.PHONY: clean

default : smallsh.o smallsh-u.o
	${CXX} ${CFLAGS} -o smallsh smallsh.o smallsh-u.o
smallsh.o : smallsh.c
	${CXX} ${CFLAGS} -c smallsh.c
smallsh-u.o: smallsh-u.c
	${CXX} ${CFLAGS} -c smallsh-u.c
clean:
	rm ./smallsh ./smallsh.o ./smallsh-u.o
