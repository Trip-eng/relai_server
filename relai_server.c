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
struct WS_User *users;
int numClientSockets = 0;
struct sockaddr_in broadcastAddr;
FILE *f;

//CODE
void error(char *msg)
{
    printf("%s\n", msg);
    
    //exit(1);
}
void logStr(char *str)
{
    //printf("%s\n", str);
    fprintf(f, "%s\n", str);

}
void logStr2(char *str1, char *str2)
{
    //no log because of daemon
    //printf("%s %s\n", str1, str2);
    fprintf(f, "%s %s\n", str1, str2);
}

int main(int argc, char *argv[])
{
    f = fopen("/var/log/relai_server.log", "a+");
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
	
    initRelai();    
    //listen for incomming connections
    listenSocketServer(argc, argv);
    return 0;
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

struct WS_User* getUserWithSocket(int socket)
{
    struct WS_User *user = users;
    while (user != NULL) 
    {
        if(user->socket == socket) return user;
        user = user->next;
    }
}

struct WS_User* getUserWithNext(struct WS_User *u)
{
    struct WS_User *user = users;
    while (user != NULL) 
    {
        if(user->next == u) return user;
        user = user->next;
    }
}

void closeSocket(int socket)
{
    //remove socket

    struct WS_User *user = getUserWithSocket(socket);
    if(user == NULL) return;
    struct WS_User *parent = getUserWithNext(user);
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
    logStr2("Disconect", user->ip);
    free(user);
    numClientSockets--;
}

void receivedText(char *outBuffer, struct WS_User *user)
{
        json_object *json = json_tokener_parse((char*)outBuffer);
	char *relai;
	char *state;
	json_object_object_foreach(json, key, val)
	{
		const char *v = json_object_get_string(val);
		if (strcmp(key, "relai") == 0) {
			relai = malloc(strlen(v)+1);
			strcpy(relai, v);
			relai[strlen(v)] = '\0';
		} else if (strcmp(key, "state") == 0) {
			state = malloc(strlen(v)+1);
			strcpy(state, v);
			state[strlen(v)] = '\0';
		}
	}
	json_object_put (json);
	char *ip = user->ip;
	
	int relaiInt = atoi(relai);
	if(strcmp(state, "on") == 0)
	{
		setRelaiState(relaiInt, 1);
	}
	else if(strcmp(state, "off") == 0)
	{
		setRelaiState(relaiInt, 0);
	}
	
	sendToAll(outBuffer);

	free(relai);
	free(state);
        json_object_put (json);
}

//MAIN
void* clientMain(void *arg)
{
    
    char buffer[MAXBUFLEN];
    int n;
    int socket = *(int*)arg;

    struct WS_User *user = getUserWithSocket(socket);
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
                logStr("ERROR reading / parsing frame failed : ");
                continue;
            };
            if (n < 0) logStr("ERROR reading socket failed");
	    //printf("recive text ");
	    //printf((char*)outBuffer);
            //json_object *json = json_tokener_parse((char*)outBuffer);

	    receivedText((char*)outBuffer,user);

            //json_object_put (json);
            free(outBuffer);
        }else if(opcode == 10)
        {
            //pong!
            //logStr("PONG! received");
        }else if(opcode == 9)
        {
            //ping!
            //logStr("PING! received");
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

        //handshake
        handshake(clientSock);

        struct WS_User *user = malloc(sizeof(struct WS_User));
        if(user == NULL)
        {
            logStr("error allocating user\n");
            continue;
        }
        if(users == NULL) users = user;
        else 
        {
            struct WS_User *last = getUserWithNext(NULL);
            last->next = user;
        }
        
        user->socket = clientSock;
        strcpy(user->ip, inet_ntoa(cli_addr.sin_addr));
        user->ip[39] = '\0';
        user->next = NULL;

        logStr2("Connect", user->ip);

        numClientSockets++;
	
        //new thread for every client
        pthread_t pth;
        pthread_create(&pth,NULL, clientMain, &clientSock);
	
	for (int i = 0; i < 10; i++)
	{
            writeFrame(clientSock,getJsonString(i,getRelaiState(i)));
	}
    }
    
    logStr("Closing server");
    return 0; 
}

char* getJsonString(int relai,char state)
{
	json_object *json = json_object_new_object();
        json_object_object_add(json, "relai", json_object_new_int(relai));
	if(state == 0)        
		json_object_object_add(json, "state", json_object_new_string("off"));
	else if (state == 1)
        	json_object_object_add(json, "state", json_object_new_string("on"));
    //const char *s = json_object_to_json_string(json);
    return json_object_to_json_string(json);
}    


void sendToAll(char *s)
{
        struct WS_User *user = users;
        while (user != NULL) 
        {
            int error;
            socklen_t len = sizeof(error);
            if(getsockopt(user->socket, SOL_SOCKET, SO_ERROR, &error, &len) == 0) writeFrame(user->socket, s);
            user = user->next;
        }
}
