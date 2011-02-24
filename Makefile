all: client server

client: client.c
	gcc -o client client.c

server: server.c
	gcc -o server server.c