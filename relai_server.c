#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <json/json.h>

#include "sha1.h"
#include "base64.h"
#include "ws_protocol.h"
#include "relai_server.h"

#include "relai_gpio.h"

#define CLIENTPORT 45600
#define MAXBUFLEN 5120
#define MAXCLIENTS 30

//globals
struct BCC_User *users;
int numClientSockets = 0;
struct sockaddr_in broadcastAddr;

void receivedText(json_object *jobj,struct BCC_User *user);
//CODE

void error(char *msg)
{
    printf("%s\n", msg);
    perror(&msg[0]);
    //exit(1);
}
void logStr(char *str)
{
    //no log because of daemon
    printf("%s\n", str);
}

int main(int argc, char *argv[])
{
    /*
    //daemon
    pid_t pid, sid;
    pid = fork();
    if (pid != 0) {
        exit(EXIT_FAILURE);
    }
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);	
    }
    */
	
    initRelai();    
    //listen for incomming connections
    printf("started\n");
    listenSocketServer(argc, argv);
    return 1;
}

char *packFrame(char *msg)
{
    int len = 1+strlen(msg)+1+4;
    int pos = 2;
    if(strlen(msg) >= 126 && strlen(msg) <= 65535) {len += 3; pos += 2;}

    char *frame = (char*)malloc(len+1);
    frame[0] = (char)129;

    if(strlen(msg) < 125) 
    {
        frame[1] = (int)strlen(msg);
    }
    else if(strlen(msg) >= 126 && strlen(msg) <= 65535)
    {
        frame[1] = (char)126;
        frame[2] = (char)((strlen(msg) >> 8) & 255);
        frame[3] = (char)(strlen(msg) & 255);
    }
    strcpy(frame+pos, msg);

    frame[len-3] = '\r';
    frame[len-2] = '\n';
    frame[len-1] = '\0';
    return frame;
}

void writeFrame(int sock, char *s)
{
    char *s2;
    s2 = packFrame(s);
    write(sock, s2, strlen(s2));
    free(s2);
}

struct BCC_User* getUserWithSocket(int socket)
{
    struct BCC_User *user = users;
    while (user != NULL) 
    {
        if(user->socket == socket) return user;
        user = user->next;
    }
}

struct BCC_User* getUserWithNext(struct BCC_User *u)
{
    struct BCC_User *user = users;
    while (user != NULL) 
    {
        if(user->next == u) return user;
        user = user->next;
    }
}

void closeSocket(int socket)
{
    //remove socket

    struct BCC_User *user = getUserWithSocket(socket);
    if(user == NULL) return;
    struct BCC_User *parent = getUserWithNext(user);
    if(parent != NULL)
    {
        if(user->next != NULL) parent->next = user->next;
        else parent->next = NULL;
    }else 
    {
        if(user->next != NULL)  users = users->next;
        else users = NULL;
    }
    close(socket);
    logStr("socket closed");
    free(user);
    numClientSockets--;
}

void receivedText(json_object *json, struct BCC_User *user)
{
	char *relais;
	char *state;
	json_object_object_foreach(json, key, val)
	{
		const char *v = json_object_get_string(val);
		if (strcmp(key, "relais") == 0) {
			relais = malloc(strlen(v)+1);
			strcpy(relais, v);
			relais[strlen(v)] = '\0';
		} else if (strcmp(key, "state") == 0) {
			state = malloc(strlen(v)+1);
			strcpy(state, v);
			state[strlen(v)] = '\0';
		}
	}
	json_object_put (json);
	char *ip = user->ip;
	
	int relai = atoi(relais);
	if(strcmp(state, "on") == 0)
	{
		setRelaiState(relai, 1);
	}
	else if(strcmp(state, "off") == 0)
	{
		setRelaiState(relai, 0);
	}
	
	//sendToAll(x)

	free(relais);
	free(state);
}

//MAIN
void* clientMain(void *arg)
{
    
    char buffer[MAXBUFLEN];
    int n;
    int socket = *(int*)arg;

    struct BCC_User *user = getUserWithSocket(socket);
    while(1)
    {
        memset((char *) &buffer, '\0', MAXBUFLEN);
        n = read(socket,&buffer,MAXBUFLEN-1);
        if(n == 0 || (int)(buffer[0] & 0x0F) == 0)
        {
            closeSocket(socket);
            break;
        }
        int opcode = (int)(buffer[0] & 0x0F);
        //text
        if(opcode == 1)
        {
            int len = 0;
            uint8_t *outBuffer = parse((const uint8_t*)buffer, n, &len);
            if(outBuffer == NULL) 
            {
                logStr("ERROR reading / parsing frame failed");
                continue;
            };
            if (n < 0) logStr("ERROR reading socket failed");
	    //printf("recive text ");
	    //printf((char*)outBuffer);
            json_object *json = json_tokener_parse((char*)outBuffer);

	    receivedText(json,user);

            json_object_put (json);
            free(outBuffer);
        }else if(opcode == 10)
        {
            //pong!
            logStr("PONG! received");
        }else if(opcode == 9)
        {
            //ping!
            logStr("PING! received");
            char *pckt = packFrame("PING");
            pckt[0] = (char)0b10001001;
            write(socket, pckt, strlen(pckt));
            free(pckt);
        }else if(opcode == 8)
        {
            //close
            write(socket, buffer, (size_t)n);
            closeSocket(socket);
            break;
        }else 
        {
            //wtf is thaat?
        }
    }
    return NULL;
}

int listenSocketServer(int argc, char *argv[])
{
    int serverSock, clientSock;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    //init socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) logStr("ERROR socket creation failed");
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(CLIENTPORT);

    //bind and listen
    int reuse = -1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) logStr("ERROR port reusing failed");
    if (bind(serverSock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  logStr("ERROR socket binding failed");
    listen(serverSock,5);
    socklen_t clilen = sizeof(cli_addr);

    //accepting new clients
    while(1)
    {
        clientSock = accept(serverSock, (struct sockaddr *) &cli_addr, &clilen);
        if(numClientSockets >= MAXCLIENTS) continue;
        if (clientSock < 0)  
        {
            logStr("ERROR accepting new socket failed");
            continue;
        }
        logStr("connected");

        //handshake
        handshake(clientSock);

        struct BCC_User *user = malloc(sizeof(struct BCC_User));
        if(user == NULL)
        {
            logStr("error allocating user\n");
            continue;
        }
        if(users == NULL) users = user;
        else 
        {
            struct BCC_User *last = getUserWithNext(NULL);
            last->next = user;
        }
        
        user->socket = clientSock;
        strcpy(user->ip, inet_ntoa(cli_addr.sin_addr));
        user->ip[39] = '\0';
        user->next = NULL;

        numClientSockets++;
        //new thread for every client
        pthread_t pth;
        pthread_create(&pth,NULL, clientMain, &clientSock);
    }
    
    logStr("closing server\n");
    return 0; 
}

/*
	json_object *json = json_object_new_object();
        json_object_object_add(json, "nick", jsostartwhileeeeeeeeeg(bccmsg->nick));
        json_object_object_add(json, "ip", json_object_new_string(bccmsg->ipAddr));
        json_object_object_add(json, "msg", json_object_new_string(bccmsg->msg));

        const char *s = json_object_to_json_string(json);
	...
        json_object_put (json);*/

void sendToAll(char *s)
{
        struct BCC_User *user = users;
        while (user != NULL) 
        {
            int error;
            socklen_t len = sizeof(error);
            if(getsockopt(user->socket, SOL_SOCKET, SO_ERROR, &error, &len) == 0) writeFrame(user->socket, s);
            user = user->next;
        }
}
