#ifndef CHATROOM_H
#define CHATROOM_H

#include <stdbool.h>

bool chatroom_init(void);
bool chatroom_join(int user_index, const char *room_name,
		   char *error_message, int error_buffer_size);
void chatroom_leave(int user_index, const char *room_name);

#endif /* CHATROOM_H */
