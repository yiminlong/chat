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

#include "user.h"
#include "chatroom.h"

#define DEFAULT_PORT 4485
#define MAX_PENDING 10
#define BUFFER_SIZE 100

static bool quit;

void interrupt(int signal_type);
void *handle_client(void *_sd);
bool parse_input(char *input_buffer, int input_buffer_size,
		 char *output_buffer, int output_buffer_size,
		 int *user_index, int sd);


int main() {
  
  long sd, client_sd;
  int port, result, return_code;
  struct sockaddr_in server_addr;
  pthread_t client_thread;
  struct sigaction handler;

  if (!user_init()) {
    fprintf(stderr, "unable to initialize user tables\n");
    exit(1);
  }

  if (!chatroom_init()) {
    fprintf(stderr, "unable to initialize chatroom tables\n");
    exit(1);
  }

  port = DEFAULT_PORT;
  quit = false;

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
  size_t bytes_received;
  int user_index;
  long sd;

  sd = (long)_sd;
  user_index = INVALID_USER;

  while (quit == false) {

    memset(receive_buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sd, receive_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {

      if (!parse_input(receive_buffer, BUFFER_SIZE, 
		       send_buffer, BUFFER_SIZE, &user_index, sd)) {
	break;
      }
      send(sd, send_buffer, strlen(send_buffer), 0);
    }
  }

  user_purge(sd); /* for now, until I add a check to prevent multiple users from same SD */
  close(sd);

  return 0;
}


/* Validates incoming messsages from users and takes the appropriate action, and then
   fills the output buffer to send back to the user. 

   Returns false if the user is requesting to logoff. */
bool parse_input(char *input_buffer, int input_buffer_size,
		 char *output_buffer, int output_buffer_size,
		 int *user_index, int sd) {

  char error_buffer[BUFFER_SIZE];
  char *token, *saveptr;
  int result;
  
  if (strncmp(input_buffer, "LOGIN", 5) == 0) {

    /* Check to see if they have already logged in and have
       been assigned a user index. */
    if (*user_index != INVALID_USER) {
      strncpy(output_buffer, "ERROR already logged in", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* Split the input into tokens separated by a space. The first
       token is guaranteed to exist (should be LOGIN), but need to
       verify that a second token was supplied as a username. If the
       username has a space in it, only the first word will be used. */
    token = strtok_r(input_buffer, " ", &saveptr);
    token = strtok_r(0, " ", &saveptr);

    if (token == 0) {
      strncpy(output_buffer, "ERROR need to specify username", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    result = user_login(token, sd, error_buffer, BUFFER_SIZE);
    if (result == INVALID_USER) {
      snprintf(output_buffer, BUFFER_SIZE, "ERROR %s", error_buffer);
      return true;
    }

    /* Save the user index. */
    *user_index = result;

  } else if (strncmp(receive_buffer, "MSG", 3) == 0) {

    /* TODO */

  } else if (strncmp(input_buffer, "JOIN", 4) == 0) {

    /* Check if the user is logged in to perform this action. */
    if (*user_index == INVALID_USER) {
      strncpy(output_buffer, "ERROR not logged in", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* Split the input to get the chatroom name. As with the login, 
       the first token is guaranteed to exist but need to
       verify that a second token was supplied. If the
       room name has a space in it, only the first word will be used. */
    token = strtok_r(input_buffer, " ", &saveptr);
    token = strtok_r(0, " ", &saveptr);

    if (token == 0) {
      strncpy(output_buffer, "ERROR need to specify room name", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* The chat rooms need to start with the # character. */
    if (token[0] != '#') {
      strncpy(output_buffer, "ERROR room name must start with # character", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* Attempt to join the chat room. */
    if (!chatroom_join(*user_index, token, error_buffer, BUFFER_SIZE)) {
      snprintf(output_buffer, BUFFER_SIZE, "ERROR %s", error_buffer);
      return true;
    }

  } else if (strncmp(input_buffer, "PART", 4) == 0) {

    /* Check if the user is logged in to perform this action. */
    if (*user_index == INVALID_USER) {
      strncpy(output_buffer, "ERROR not logged in", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* Split the input to get the chatroom name. As with the login, 
       the first token is guaranteed to exist but need to
       verify that a second token was supplied. If the
       room name has a space in it, only the first word will be used. */
    token = strtok_r(input_buffer, " ", &saveptr);
    token = strtok_r(0, " ", &saveptr);

    if (token == 0) {
      strncpy(output_buffer, "ERROR need to specify room name", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* The chat rooms need to start with the # character. */
    if (token[0] != '#') {
      strncpy(output_buffer, "ERROR room name must start with # character", output_buffer_size);
      output_buffer[output_buffer_size - 1] = 0;
      return true;
    }

    /* Leave the chat room. */
    chatroom_leave(*user_index, token);

  } else if (strncmp(input_buffer, "LOGOUT", 6) == 0) {

    user_logout(*user_index);
    return false;

  }

  strncpy(output_buffer, "OK", output_buffer_size);
  output_buffer[output_buffer_size - 1] = 0;

  return true;


}




/* Signal Interrupt handler for SIGINT. Sets the quit flag to true
   which gives time to cleanup before terminating. */
void interrupt(int signal_type) {
  quit = true;
}
