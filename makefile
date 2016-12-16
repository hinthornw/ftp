

INCLUDEC = -I./headers -I./clientFiles
INCLUDES = -I./headers -I./servFilesl
CF = ./clientFiles/


rc: client
	./client localhost

rs: server
	sudo ./server

server: server.o sservices.o
	gcc  -Wall $(INCLUDES) -o server server.o sservices.o
server.o: ./servFiles/server.c
	gcc -Wall $(INCLUDES) -c ./servFiles/server.c

sservices.o: ./servFiles/sservices.c
	gcc -Wall $(INCLUDES)  -c ./servFiles/sservices.c

client: client.o cservices.o
	gcc -Wall $(INCLUDEC)  -o client client.o cservices.o

client.o: ./clientFiles/client.c
	gcc -Wall $(INCLUDEC) -c ./clientFiles/client.c

cservices.o: ./clientFiles/cservices.c
	gcc -Wall $(INCLUDEC) -c ./clientFiles/cservices.c



clean:
	rm -f server client *.o