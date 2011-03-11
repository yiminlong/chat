#include "user.h"
#include <stdio.h>
#include <string.h>
#include <semaphore.h>


#define MAX_NAME_LENGTH 20
#define MAX_USERS	2


/* Represents a single connected user with an associated username. */
struct user {
  int sd;			/* The socket descriptor that the user is connected to. */
  bool logged_in;		/* Whether this structure is an active user. */
  char name[MAX_NAME_LENGTH + 1];	/* The name of the user. */
};

/* For now use a statically allocated table of users. */
static struct user users[MAX_USERS];

/* A semaphore is used to synchronize access to the users table. */
static sem_t users_sema;

/* Initializes the user table. Returns true on success, false otherwise. */
bool user_init() {
  int i;

  /* Make sure that the users table is empty. */
  for (i = 0; i < MAX_USERS; ++i) {
    users[i].logged_in = false;
    users[i].sd = -1;
    users[i].name[0] = 0;
  }

  /* Initialize the semaphore for locking the table. */
  if (sem_init(&users_sema, 0, 1) == -1) {
    perror("sem_init");
    return false;
  }

  return true;
}


int user_login(const char *client_name, int sd, 
		char *error_message, int error_buffer_size) {

  int i;

  /* Check that the username is not too long. */
  if (strlen(client_name) > MAX_NAME_LENGTH) {
    snprintf(error_message, error_buffer_size, "username too long (max: %d chars)", MAX_NAME_LENGTH);
    return INVALID_USER;
  }

  /* TODO: other input validation on username (one word, any special chars?) */
  /* TODO check for multiple logins by same name or SD */

  /* Lock the table before searching it. */
  sem_wait(&users_sema);

  /* Find a free spot in the users table. */
  for (i = 0; i < MAX_USERS; ++i) {
    if (!users[i].logged_in) {
      break;
    }
  }

  /* If no free slots available, return an error. */
  if (i >= MAX_USERS) {
    sem_post(&users_sema);
    strncpy(error_message, "chat server full", error_buffer_size - 1);
    return INVALID_USER;
  }

  /* Copy the user info into the open slot. */
  users[i].logged_in = true;
  users[i].sd = sd;
  strncpy(users[i].name, client_name, MAX_NAME_LENGTH);

  /* Release the lock on the table. */
  sem_post(&users_sema);

  /* Return the index into the user table as the user ID. */
  return i;
}


/* Logs the given user out. */
void user_logout(int user_index) {

  /* If the index is outside the range of the table, do nothing. */
  if (user_index < 0 || user_index >= MAX_USERS) {
    return;
  }

  /* Lock the table before making any changes. */
  sem_wait(&users_sema);

  users[user_index].logged_in = false;
  users[user_index].sd = -1;
  users[user_index].name[0] = 0;
  
  /* Release the lock on the table. */
  sem_post(&users_sema);
}


/* Removes any entries in the user table with the given socket descriptor. */
void user_purge(int sd) {

  int i;

  /* Lock the table before making any changes. */
  sem_wait(&users_sema);

  /* Search for any users that match the given SD and mark them
     as logged out. */
  for (i = 0; i < MAX_USERS; ++i) {
    if (users[i].sd == sd) {
      users[i].logged_in = false;
      users[i].sd = -1;
      users[i].name[0] = 0;
    }
  }
  
  /* Release the lock on the table. */
  sem_post(&users_sema);
}
