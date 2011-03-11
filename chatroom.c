#include <stdbool.h>

#include "chatroom.h"

#define MAX_ROOMS		100
#define MAX_CHAT_NAME_LENGTH	100
#define MAX_CLIENTS_PER_ROOM	100

struct chatroom {
  bool in_use;
  char name[MAX_CHAT_NAME_LENGTH];
  int client_sds[MAX_CLIENTS_PER_ROOM];
};

static struct chatroom rooms[MAX_ROOMS];


void chatroom_init(void) {

}

void chatroom_join(int client_sd, char *room_name) {

}


void chatroom_leave(int client_sd, char *room_name) {

}
