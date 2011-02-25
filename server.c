#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 4485
#define MAX_PENDING 10
#define BUFFER_SIZE 20

int main() {
  int sd, port, result;
  struct sockaddr_in server_addr;
  int client_sd, send_buffer_size;
  char buffer[BUFFER_SIZE];
  size_t client_size, bytes_sent;

  port = DEFAULT_PORT;

  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd < 0) {
    fprintf(stderr, "error: unable to open a socket\n");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port	      = htons(port);

  result = bind(sd, (struct sockaddr *)&server_addr, 
		sizeof(struct sockaddr_in));
  if (result < 0) {
    fprintf(stderr, "error: unable to bind\n");
    close(sd);
    exit(1);
  }

  result = listen(sd, MAX_PENDING);
  if (result < 0) {
    fprintf(stderr, "error: could not listen\n");
    close(sd);
    exit(1);
  }

  strncpy(buffer, "hello, world!\n", BUFFER_SIZE);
  send_buffer_size = strlen(buffer) + 1;

  while (1) {
    client_sd = accept(sd, 0, 0);
    if (client_sd < 0) {
      fprintf(stderr, "error: could not accept client connection\n");
      close(sd);
      exit(1);
    }

    bytes_sent = send(client_sd, buffer, send_buffer_size, 0);
    if (bytes_sent != send_buffer_size) {
      fprintf(stderr, "error: could not send full packet\n");
      close(client_sd);
      close(sd);
      exit(1);
    }

    close(client_sd);
  }
  close(sd);
  return 0;
}
