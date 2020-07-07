# GBS Shell Makefile

CC = gcc
CFLAGS  = -Wall -g
OBJ = gbsh.o

all: gbsh

gbsh: $(OBJ)
	$(CC) $(CFLAGS) -o gbsh $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
