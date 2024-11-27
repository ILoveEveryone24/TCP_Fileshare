#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define PORT 4444
#define IP "127.0.0.1"

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        perror("Failed to create a socket");
        return -1;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &server_addr.sin_addr);

    printf("Connecting");

    int connected = connect(s, (struct sockaddr*)&server_addr, server_addr_len);
    while(connected < 0){
        if(connected < 0 && errno != EINPROGRESS && errno != ECONNREFUSED){
            perror("Failed to connect to the server");
            return -1;
        }
        sleep(1);

        connected = connect(s, (struct sockaddr*)&server_addr, server_addr_len);

        printf(".");
    }
    printf("\n");
    printf("CONNECTED");
    printf("\n");

    char request[1024];
    memset(request, 0, sizeof(request));

    char response[1024];
    memset(response, 0, sizeof(response));
    

    while(1){
        if(fgets(request, sizeof(request), stdin) != NULL){
            request[strcspn(request, "\n")] = '\0';

            send(s, request, sizeof(request), 0);
        }
        ssize_t bytes_r;
        while((bytes_r = recv(s, response, sizeof(response), 0)) > 0){
            printf("Server: %s", response);
        }
    }

    close(s);

    return 0;
}
