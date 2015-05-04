CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS = -lpthread
default: h2o clean

h2o: h2o.o $(LFLAGS)

h2o.o: h2o.c

clean:
	rm *.o

zip:
	zip xsroka00.zip *.h *.c Makefile