#ifndef RELAI_SERVER_H
#define RELAI_SERVER_H

struct BCC_User
{
	char ip[40];
	int socket;
	struct BCC_User *next;
};

char *packFrame(char *msg);
void closeSocket(int socket);

//methods
void error(char *msg);
void sendToAll(char *s);


//MAIN
int listenSocketServer(int argc, char *argv[]);
void* clientMain(void *arg);


#endif