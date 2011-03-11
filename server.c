#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "login.h"
#include "chatroom.h"

#define DEFAULT_PORT 4485
#define MAX_PENDING 10
#define BUFFER_SIZE 100

static bool quit;

void interrupt(int signal_type);
void *handle_client(void *_sd);

int main() {
  
  long sd, client_sd;
  int port, result, return_code;
  struct sockaddr_in server_addr;
  pthread_t client_thread;
  struct sigaction handler;

  port = DEFAULT_PORT;
  quit = false;

  login_init();
  chatroom_init();

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

  while (quit == false) {

    client_sd = accept(sd, 0, 0);
    if (client_sd < 0) {
      perror("accept");
      quit = true;
      break;
    }

    /* Launch a separate thread to receive and display
       any incoming data. */
    return_code = pthread_create(&client_thread, 0, handle_client, (void *)client_sd);
    if (return_code != 0) {
      fprintf(stderr, "error: unable to create new thread. return code: %d\n", return_code);
      close(client_sd);
      quit = true;
      break;
    }
  }

  close(sd);
  return 0;
}

void *handle_client(void *_sd) {
  char receive_buffer[BUFFER_SIZE];
  char send_buffer[BUFFER_SIZE];
  char error_buffer[BUFFER_SIZE];
  char temp_buffer[BUFFER_SIZE];
  size_t bytes_received;
  long sd;
  char *saveptr;
  char *token;
  int result;


  sd = (long)_sd;
  saveptr = 0;

  while (quit == false) {
    saveptr = 0;
    memset(receive_buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sd, receive_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {

      /* Mega function */
      if (strncmp(receive_buffer, "LOGIN", 5) == 0) {

	/* TODO Handle case where no name is given */
	/* Is this needed? */
	strncpy(temp_buffer, receive_buffer, BUFFER_SIZE - 1);

	strtok_r(temp_buffer, " ", &saveptr);
	token = strtok_r(0, " ", &saveptr);

	result = login(token, sd, error_buffer, BUFFER_SIZE);
	if (result != sd) {
	  snprintf(send_buffer, BUFFER_SIZE - 1, "ERROR %s", error_buffer);

	} else {

	  strncpy(send_buffer, "OK", BUFFER_SIZE - 1);
	}
      
      } else if (strncmp(receive_buffer, "LOGOUT", 6) == 0) {
	break;

      } else if (strncmp(receive_buffer, "MSG", 3) == 0) {

      } else if (strncmp(receive_buffer, "JOIN", 4) == 0) {

	strncpy(temp_buffer, receive_buffer, BUFFER_SIZE - 1);

	strtok_r(temp_buffer, " ", &saveptr);
	token = strtok_r(0, " ", &saveptr);

	/* token should be the name of the chatroom
	   TODO verify that it starts with a pound sign */
	chatroom_join(sd, token);

      } else if (strncmp(receive_buffer, "PART", 4) == 0) {

      } else {
	strncpy(send_buffer, "ERROR unknown command", BUFFER_SIZE - 1);
      }

      send(sd, send_buffer, strlen(send_buffer), 0);
    }
  }

  logout(sd);
  close(sd);

  return 0;
}


/* Signal Interrupt handler for SIGINT. Sets the quit flag to true
   which gives time to cleanup before terminating. */
void interrupt(int signal_type) {
  quit = true;
}
