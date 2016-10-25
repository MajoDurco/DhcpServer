CC=gcc
FLAGS= -Wall -Wextra -pedantic -std=c11 -W -g
SERVER=dserver
FILES= main.h dserver.c dserver.h dhcpopt.c dhcpopt.h ippool.c ippool.h
all: $(OBJECTS)
	$(CC) $(FLAGS) $(FILES) -lm -o $(SERVER)

clean:
	rm $(SERVER) 

tar: 
	tar -czf xdurco00.tar.gz $(SERVER_FILE) #TODO
