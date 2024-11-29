#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

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

    char response[2048];
    memset(response, 0, sizeof(response));

    FILE *file;
    char *file_name;
    long file_size = 0;

    long dir_size = 0;

    char *opcode;

    //FLAGS
    int getting = 0;
    int listing = 0;
    int putting = 0;

    while(1){
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));

        if(fgets(request, sizeof(request), stdin) != NULL){
            request[strcspn(request, "\n")] = '\0';

            send(s, request, sizeof(request), 0);
        }

        ssize_t bytes_r;
        while((bytes_r = recv(s, response, sizeof(response), 0)) > 0){
            //CHECK FLAGS
            if(getting == 1){
                file = fopen(file_name, "a");
                long file_bytes = 0;
                printf("GETTING FILE...\n");
                while(file_bytes != file_size){
                    sleep(1);
                    if(bytes_r < 0){
                        bytes_r = recv(s, response, sizeof(response), 0);
                        continue;
                    }
                    else if(bytes_r == 0){
                        break;
                    }
                    else{
                        fwrite(response, sizeof(char), bytes_r, file);
                        file_bytes += bytes_r;
                    }
                    memset(response, 0, sizeof(response));

                    bytes_r = recv(s, response, sizeof(response), 0);
                }
                printf("FILE SUCCESSFULLY DOWNLOADED\n");
                fclose(file);
                free(file_name);
                getting = 0;
                continue;
            }
            else if(listing == 1){
                long cnt = 0;
                while(cnt < dir_size){
                    if(bytes_r < 0){
                        bytes_r = recv(s, response, sizeof(response), 0);
                        continue;
                    }
                    else if(bytes_r == 0){
                        break;
                    }
                    else{
                        printf("%s", response);
                        memset(response, 0, sizeof(response));
                    }
                    bytes_r = recv(s, response, sizeof(response), 0);
                    
                    cnt++;
                }
                listing = 0;
                continue;
            }
            else if(putting == 1){
                //PUT FILE
            }

            //OPCODES
            printf("Server: %s\n", response);
            opcode = strtok(response, " ");
            if(opcode != NULL){
                if(strcmp(opcode, "GET") == 0){
                    file_name = strtok(NULL, " ");
                    file_size = strtol(strtok(NULL, " "), NULL, 10);
                    if(file_name != NULL){
                        file_name = strdup(file_name);
                        file = fopen(file_name, "w");
                        fclose(file);
                        getting = 1;
                    }
                }
                else if(strcmp(opcode, "LS") == 0){
                    dir_size = strtol(strtok(NULL, " "), NULL, 10) + 1;
                    listing = 1;
                }
            }

            memset(response, 0, sizeof(response));
        }
        if(bytes_r == 0){
            break;
        }
    }

    close(s);

    return 0;
}
