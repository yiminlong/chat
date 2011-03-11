#include <stdio.h>
#include <semaphore.h>
#include <string.h>

#include "chatroom.h"
#include "user.h"

#define MAX_ROOMS		20
#define MAX_ROOMNAME_LENGTH	20
#define MAX_USERS_PER_ROOM	20

struct chatroom {
  bool in_use;
  char name[MAX_ROOMNAME_LENGTH + 1];
  int user_indices[MAX_USERS_PER_ROOM];
};

static struct chatroom rooms[MAX_ROOMS];

/* Semaphore used to synchronize access to the chatrooms table. */
static sem_t rooms_sema;


/* Initializes any data structures for the chat rooms. */
bool chatroom_init(void) {

  int i, j;

  /* Initialize the semaphore to allow one thread to access the table
     at a time. */
  if (sem_init(&rooms_sema, 0, 1) == -1) {
    perror("sem_init");
    return false;
  }

  /* For each room, mark it not in use and set all of the 
     user indices to -1. */
  for (i = 0; i < MAX_ROOMS; ++i) {
    rooms[i].in_use = false;
    rooms[i].name[0] = 0;
    
    for (j = 0; j < MAX_USERS_PER_ROOM; ++j) {
      rooms[i].user_indices[j] = -1;
    }
  }

  return true;
}


/* Adds a user to an existing chatroom or creates a new one if it doesn't exist.
   Returns false on error and fills the error_message buffer with a description
   of the problem. */
bool chatroom_join(int user_index, const char *room_name,
		   char *error_message, int error_buffer_size) {

  int i, j;

  /* Verify that the room name is valid (not null and less than max. */
  if (room_name == 0 || strlen(room_name) > MAX_ROOMNAME_LENGTH) {
    snprintf(error_message, error_buffer_size, 
	     "must specify a room name (max: %d chars)", MAX_ROOMNAME_LENGTH);
    return false;
  }

  /* Check that the user index is valid. */
  if (user_index == INVALID_USER) {
    strncpy(error_message, "user not logged in", error_buffer_size);
    error_message[error_buffer_size - 1] = 0;
    return false;
  }

  /* Lock the chatrooms table. */
  sem_wait(&rooms_sema);

  /* Search through the table to see if a room with the given name already exists. */
  for (i = 0; i < MAX_ROOMS; ++i) {
    if (strcmp(rooms[i].name, room_name) == 0) {

      /* The room was found, now add this user. */
      for (j = 0; j < MAX_USERS_PER_ROOM; ++j) {

	/* TODO: check if user was already added. */
	if (rooms[i].user_indices[j] == INVALID_USER) {
	  rooms[i].user_indices[j] = user_index;
	  break;
	}
      }
      break;
    }
  }

  /* If the room wasn't found, create it. */
  if (i == MAX_ROOMS) {
    
    /* Find a room that isn't in use. */
    for (i = 0; i < MAX_ROOMS; ++i) {
      if (!rooms[i].in_use) {

	rooms[i].in_use = true;
	strcpy(rooms[i].name, room_name);
	
	/* Clear out any existing users, and then add this
	   user as the first entry. */
	for (j = 0; j < MAX_USERS_PER_ROOM; ++j) {
	  rooms[i].user_indices[j] = INVALID_USER;
	}
	rooms[i].user_indices[0] = user_index;

	break;
      }
    }
  }

  /* Check if a room was found. */
  if (i == MAX_ROOMS) {
    strncpy(error_message, "no space available to create chatroom", error_buffer_size);
    error_message[error_buffer_size - 1] = 0;

    sem_post(&rooms_sema);
    return false;
  }

  /* Release the lock. */
  sem_post(&rooms_sema);

  return true;
}


/* Removes a user from the given chatroom (if it exists). */
void chatroom_leave(int user_index, const char *room_name) {
  
  int i, j;

  /* Lock the chatrooms table. */
  sem_wait(&rooms_sema);

  /* Find the room name in the chatrooms table. */
  for (i = 0; i < MAX_ROOMS; ++i) {

    if (strcmp(rooms[i].name, room_name) == 0) {
      
      /* Room found, now remove the user index from the table. */
      for (j = 0; j < MAX_USERS_PER_ROOM; ++j) {
	if (rooms[i].user_indices[j] == user_index) {
	  rooms[i].user_indices[j] = INVALID_USER;
	}
      }
    }
  }
  
  /* Release the lock. */
  sem_post(&rooms_sema);
}
