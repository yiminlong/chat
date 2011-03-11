#ifndef USER_H
#define USER_H

#include <stdbool.h>

#define INVALID_USER -1

bool user_init(void);
int user_login(const char *client_name, int sd, 
	       char *error_message, int error_buffer_size);
void user_logout(int user_index);
void user_purge(int sd);

#endif /* USER_H */
