#ifndef LOGIN_H
#define LOGIN_H

#include <stdbool.h>

bool login_init();
int login(const char *client_name, int sd, char *error_message, int error_buffer_size);
void logout(int sd);

#endif /* LOGIN_H */
