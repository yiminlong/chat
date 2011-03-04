#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 4485
#define MAX_PENDING 10
#define BUFFER_SIZE 100

#define TRUE 1
#define FALSE 0

static int quit;

void interrupt(int signal_type);
void *handle_client(void *_sd);
void parse_input(const char *input_buffer, int input_buffer_size,
		 char *output_buffer, int output_buffer_size);

int main() {
  
  long sd, client_sd;
  int port, result, return_code;
  struct sockaddr_in server_addr;
  pthread_t client_thread;
  struct sigaction handler;

  port = DEFAULT_PORT;
  quit = FALSE;

  sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd < 0) {
    perror("socket");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port	      = htons(port);

  result = bind(sd, (struct sockaddr *)&server_addr, 
		sizeof(struct sockaddr_in));
  if (result < 0) {
    perror("bind");
    close(sd);
    exit(1);
  }

  result = listen(sd, MAX_PENDING);
  if (result < 0) {
    perror("listen");
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

  while (quit == FALSE) {

    client_sd = accept(sd, 0, 0);
    if (client_sd < 0) {
      perror("accept");
      quit = TRUE;
      break;
    }

    /* Launch a separate thread to receive and display
       any incoming data. */
    return_code = pthread_create(&client_thread, 0, handle_client, (void *)client_sd);
    if (return_code != 0) {
      fprintf(stderr, "error: unable to create new thread. return code: %d\n", return_code);
      close(client_sd);
      quit = TRUE;
      break;
    }
  }

  close(sd);
  return 0;
}

void *handle_client(void *_sd) {
  char receive_buffer[BUFFER_SIZE];
  char send_buffer[BUFFER_SIZE];
  size_t bytes_received;
  long sd;

  sd = (long)_sd;

  while (quit == FALSE) {
    memset(receive_buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sd, receive_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
      parse_input(receive_buffer, BUFFER_SIZE, send_buffer, BUFFER_SIZE);
      send(sd, send_buffer, strlen(send_buffer), 0);
    }
  }

  close(sd);

  return 0;
}

void parse_input(const char *input_buffer, int input_buffer_size,
		 char *output_buffer, int output_buffer_size) {

  if (strncmp(input_buffer, "LOGIN", 5) == 0) {
    strncpy(output_buffer, "OK", output_buffer_size - 1);
  } else {
    strncpy(output_buffer, "ERROR unknown command", output_buffer_size - 1);
  }
}


/* Signal Interrupt handler for SIGINT. Sets the quit flag to true
   which gives time to cleanup before terminating. */
void interrupt(int signal_type) {
  quit = TRUE;
}
