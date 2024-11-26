#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define PORT 4444

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        perror("Failed to create the socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(s, (struct sockaddr*)&server_addr, server_addr_len) < 0){
        perror("Failed to bind the socket");
        return -1;
    }

    printf("Listening...");
    if(listen(s, 5) < 0){
        perror("Failed to listen");
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf("Waiting for a client");
    int client_s = accept(s, (struct sockaddr*)&client_addr, &client_addr_len);
    while(client_s < 0){
        if(errno != EAGAIN && errno != EWOULDBLOCK){
            perror("Failed to accept a socket");
            return -1;
        }
        client_s = accept(s, (struct sockaddr*)&client_addr, &client_addr_len);
        printf(".");
        sleep(1);
    }
    printf("\nCLIENT CONNECTED\n");

    char request[1024];
    memset(request, 0, sizeof(request));

    char response[1024];
    memset(response, 0, sizeof(response));

    while(1){
        recv(client_s, request, sizeof(request), 0);

        printf("Client: %s\n", request);

        if(strcmp(request, "ls") == 0){
            strcpy(response, "DIRECTORY");
            send(client_s, response, sizeof(response), 0);
        }
        else{
            strcpy(response, "INVALID COMMAND");
            send(client_s, response, sizeof(response), 0);
        }
    }

    close(client_s);
    close(s);

    return 0;
}
