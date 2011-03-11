#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_PORT	4485
#define BUFFER_SIZE	100

static bool quit;
void *display_responses(void *_sd);
void interrupt(int signal_type);

int main() {
  long sd;
  int port, result, return_code;
  pthread_t receive_thread;
  char send_buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr;
  struct sigaction handler;
  const char *server = "127.0.0.1";

  quit = false;
  port = DEFAULT_PORT;

  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd < 0) {
    perror("socket");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server);
  server_addr.sin_port	      = htons(port);

  result = connect(sd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
  if (result < 0) {
    perror("connect");
    close(sd);
    exit(1);
  }

  /* Set up the interrupt handler to capture SIGINT so we have a chance to cleanup. */
  handler.sa_handler = interrupt;
  result = sigfillset(&handler.sa_mask);
  if (result < 0) {
    perror("sigfillset");
    close(sd);
    exit(1);
  }
  handler.sa_flags = 0;

  result = sigaction(SIGINT, &handler, 0);
  if (result < 0) {
    perror("sigaction");
    close(sd);
    exit(1);
  }

  /* Launch a separate thread to receive and display
     any incoming data. */
  return_code = pthread_create(&receive_thread, 0, display_responses, (void *)sd);
  if (return_code != 0) {
    fprintf(stderr, "error: unable to create new thread. return code: %d\n", return_code);
    close(sd);
    exit(1);
  }

  /* Read input from standard input and send it to the server. */
  memset(send_buffer, 0, BUFFER_SIZE);

  while (quit == false) {
    /* Place fgets() last in the while loop since it is a blocking function.
       If a signal interrupts it, it will check the quit flag before sending
       its buffer. This prevents it from sending the last buffer twice when
       it is terminating. */
    if (strlen(send_buffer) > 0) {
      send(sd, send_buffer, strlen(send_buffer), 0);
    }

    fgets(send_buffer, BUFFER_SIZE, stdin);
  }

  close(sd);
  
  return 0;
}

/* Listens for messages on a connected socket and 
   writes them to the standard output. */
void *display_responses(void *_sd) {
  char receive_buffer[BUFFER_SIZE];
  size_t bytes_received;
  long sd = (long)_sd;

  while (quit == false) {
    memset(receive_buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sd, receive_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
      printf("%s\n", receive_buffer);
    }
  }

  return 0;
}

/* Signal interrupt handler for SIGINT. Sets the quit flag to true
   which gives time to cleanup before terminating. */
void interrupt(int signal_type) {
  quit = true;
}
