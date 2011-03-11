#ifndef CHATROOM_H
#define CHATROOM_H

void chatroom_init(void);
void chatroom_join(int client_sd, char *room_name);
void chatroom_leave(int client_sd, char *room_name);

#endif /* CHATROOM_H */
