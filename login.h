#ifndef LOGIN_H
#define LOGIN_H

int login(const char *client_name, int sd, char *error_message, int error_buffer_size);
void logout(int sd);

#endif /* LOGIN_H */
