# Makefile
# MATH 4777 Project

CC=mpicc
SRC=main.c central.c node.c
INC=central.h node.h univ.h container.h
OBJ=main.o central.o node.o container.o
TARGET=fsch
CFLAGS=-O0 -Wall -Werror -pedantic -std=c99 -g -pthread

all : $(OBJ) $(INC)
	$(CC) $(OBJ) -o $(TARGET)

serial : CC=gcc
serial : main_serial.o container.o container.h
	$(CC) main_serial.o container.o -o fsch_serial

main.o : main.c
	$(CC) $(CFLAGS) -c main.c

central.o : central.c central.h
	$(CC) $(CFLAGS) -c central.c

node.o : node.c node.h
	$(CC) $(CFLAGS) -c node.c

container.o : container.c container.h
	$(CC) $(CFLAGS) -c container.c

main_serial.o : main_serial.c
	$(CC) $(CFLAGS) -c main_serial.c

clean :
	rm -rf $(OBJ) main_serial.o $(TARGET) fsch_serial

