all: client server

client: client.c
	gcc -pthread -o client client.c

server: server.c
	gcc -pthread -o server server.c