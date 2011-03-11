all: client server

client: client.c
	gcc -Wall -pthread -o client client.c 

server: server.c login.c chatroom.c
	gcc -Wall -pthread -o server server.c login.c chatroom.c

clean:
	-rm server client