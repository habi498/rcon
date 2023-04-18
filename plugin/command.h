#ifndef COMMAND_H
#define COMMAND_H
void OnRCONCommand(char* buffer, char* ip, char* reply, int replylen);
void OnRCONLoginFailed(char* IP);
#endif