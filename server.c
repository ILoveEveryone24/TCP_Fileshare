#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define PORT 4444

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        perror("Failed to create the socket");
        return -1;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

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

    FILE *command;
    char command_r[1024];
    memset(command_r, 0, sizeof(command_r));

    while(1){
        recv(client_s, request, sizeof(request), 0);

        printf("Client: %s\n", request);

        if(strcmp(request, "ls") == 0){
            command = popen("ls -ago", "r");
            if(command == NULL){
                strcpy(response, "INVALID COMMAND\n");
                send(client_s, response, sizeof(response), 0);
            }
            while(fgets(command_r, sizeof(command_r), command) != NULL){
                int bytes_sent = send(client_s, command_r, sizeof(command_r), 0);
            }
            pclose(command);
        }
        else{
            strcpy(response, "INVALID COMMAND\n");
            send(client_s, response, sizeof(response), 0);
        }
    }

    close(client_s);
    close(s);

    return 0;
}
