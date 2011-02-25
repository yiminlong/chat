#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_PORT 4485
#define BUFFER_SIZE 100

int main() {
  int sd, port, result;
  struct sockaddr_in server_addr;
  const char *server = "localhost";

  port = DEFAULT_PORT;

  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd < 0) {
    fprintf(stderr, "error: unable to open a socket\n");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port	      = htons(port);

  result = connect(sd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
  if (result < 0) {
    fprintf(stderr, "error: unable to connect to server\n");
    close(sd);
    exit(1);
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_received;

  bytes_received = recv(sd, buffer, BUFFER_SIZE, 0);
  if (bytes_received > 0) {
    printf("received: %s\n", buffer);
  }

  close(sd);
  
  return 0;
}
