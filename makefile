

INCLUDEC = -I./headers -I./clientFiles
INCLUDES = -I./headers -I./servFiles
CF = ./clientFiles/
FLAGS = -Wall -Wextra
vpath %.h headers
vpath %.c clientFiles
vpath %.c servFiles

rc: client
	./client localhost

rs: server
	sudo ./server

server: server.o sservices.o sservices.h
	gcc  $(FLAGS) $(INCLUDES) -o server server.o sservices.o
server.o: ./servFiles/server.c
	gcc $(FLAGS) $(INCLUDES) -c ./servFiles/server.c

sservices.o: ./servFiles/sservices.c
	gcc $(FLAGS) $(INCLUDES)  -c ./servFiles/sservices.c

client: client.o cservices.o cservices.h
	gcc $(FLAGS) $(INCLUDEC)  -o client client.o cservices.o

client.o: ./clientFiles/client.c
	gcc $(FLAGS) $(INCLUDEC) -c ./clientFiles/client.c

cservices.o: ./clientFiles/cservices.c
	gcc $(FLAGS) $(INCLUDEC) -c ./clientFiles/cservices.c



clean:
	rm -f server client *.o
	