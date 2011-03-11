#include "login.h"
#include <string.h>

#define BAD_SD -1
#define MAX_NAME_LENGTH 100
#define MAX_LOGINS 2

struct login {
  int sd;
  bool used;
  char name[MAX_NAME_LENGTH + 1];
};

static struct login logins[MAX_LOGINS];

bool login_init() {
  memset(logins, 0, sizeof(struct login) * MAX_LOGINS);
  return true;
}

int login(const char *client_name, int sd, char *error_message, int error_buffer_size) {
  int i; 

  /* TODO Add locks to this table. */
  /* TODO check for multiple logins by same SD */

  /* If the name is too long, return an error instead of truncating it. */
  if (strlen(client_name) > MAX_NAME_LENGTH) {
    strncpy(error_message, "login name too long", error_buffer_size - 1);
    return BAD_SD;
  }


  /* Find a free spot in the table of logins. */
  for (i = 0; i < MAX_LOGINS; ++i) {
    if (!logins[i].used) {
      break;
    }
  }

  /* No free slots available. */
  if (i >= MAX_LOGINS) {
    strncpy(error_message, "no room available", error_buffer_size - 1);
    return BAD_SD;
  }

  /* Copy the client info into the open slot. */
  logins[i].used = true;
  logins[i].sd = sd;
  strncpy(logins[i].name, client_name, MAX_NAME_LENGTH);

  return sd;
}

void logout(int sd) {

  /* TODO Add locks to this table. */

  int i;

  /* Try to find the given SD in the logins table. */
  for (i = 0; i < MAX_LOGINS; ++i) {
    if (logins[i].sd == sd) {
      break;
    }
  }

  /* SD wasn't found in the table, no need to do anything. */
  if (i >= MAX_LOGINS) {
    return;
  }
  
  logins[i].used = false;
  logins[i].sd = BAD_SD;
}
