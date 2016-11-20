CC=gcc
FLAGS= -Wall -Wextra -pedantic -std=c11 -W -g
SERVER=dserver
FILES= main.h dserver.c dserver.h dhcpopt.c dhcpopt.h ippool.c ippool.h
all: $(OBJECTS)
	$(CC) $(FLAGS) $(FILES) -lm -o $(SERVER)

clean:
	rm $(SERVER) 

tar: 
	tar -cf xdurco00.tar $(FILES) README manual.pdf Makefile
