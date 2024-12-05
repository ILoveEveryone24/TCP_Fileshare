#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT 4444

int isInArray(char *str, char *arr[], int len){
    for(int i = 0; i < len; i++){
        if(strcmp(str, arr[i]) == 0){
            return 0;
        }
    }
    return -1;
}

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
    char command_res[1024];
    memset(command_res, 0, sizeof(command_res));

    struct dirent *dp;
    char *path = "./";
    DIR *dir = opendir(path);

    if(dir  == NULL){
        printf("Failed to open current directory");
        return -1;
    }

    long file_count = 0;
    while((dp = readdir(dir)) != NULL){
        file_count++;
    }

    char **files = malloc(file_count * sizeof(char *));
    if(files == NULL){
        printf("Failed to allocate memory");
        return -1;
    }
    rewinddir(dir);

    int i = 0;
    while((dp = readdir(dir)) != NULL){
        files[i] = strdup(dp->d_name);
        i++;
    }

    char *command_req;
    char *file_name;

    while(1){
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));
        recv(client_s, request, sizeof(request), 0);

        printf("Client: %s\n", request);

        command_req = strtok(request, " ");
        file_name = strtok(NULL, " ");
        printf("Comamnd_req: %s\n", command_req);
        printf("File_name: %s\n", file_name);

        if(strcmp(command_req, "ls") == 0){
            command = popen("ls -ago", "r");
            if(command == NULL){
                strcpy(response, "INVALID COMMAND\n");
                send(client_s, response, sizeof(response), 0);
                continue;
            }


            fseek(command, 0, SEEK_END);
            long command_length = ftell(command);
            fseek(command, 0, SEEK_SET);

            
            char *ls_dir = malloc(command_length);
            if(ls_dir == NULL){
                printf("Failed to allocate memory");
                strcpy(response, "FAILED TO RUN COMMAND\n");
                send(client_s, response, sizeof(response),0);
                pclose(command);
                continue;
            }
            size_t bytes_read = fread(ls_dir, sizeof(char), command_length, command);
            if(bytes_read != command_length){
                printf("Failed to read the command");
                strcpy(response, "FAILED TO READ THE COMMAND\n");
                send(client_s, response, sizeof(response), 0);
                free(ls_dir);
                pclose(command);
                continue;

            }

            sprintf(response, "LS %ld", command_length);
            send(client_s, response, sizeof(response), 0);
            size_t bytes_sent = send(client_s, ls_dir, command_length, 0);
            if(bytes_sent != command_length){
                printf("Failed to send whole command");
            }

            pclose(command);
        }
        else if(strcmp(command_req, "get") == 0 && file_name != NULL){
            if(isInArray(file_name, files, file_count) == 0){
                FILE *file = fopen(file_name, "r");
                if(file == NULL){
                    printf("Failed to open file");
                    strcpy(response, "INVALID FILE\n");
                    send(client_s, response, sizeof(response), 0);
                    continue;
                }

                fseek(file, 0, SEEK_END);
                long file_length = ftell(file);
                fseek(file, 0, SEEK_SET);
                printf("File length:%ld\n", file_length);

                char *file_content = malloc(file_length);
                if(file_content == NULL){
                    printf("Failed to allocate memory");
                    strcpy(response, "FAILED TO OPEN FILE\n");
                    send(client_s, response, sizeof(response), 0);
                    fclose(file);
                    continue;
                }

                size_t bytes_read = fread(file_content, sizeof(char), file_length, file);
                if(bytes_read != file_length){
                    printf("Failed to read the file");
                    strcpy(response, "FAILED TO READ FILE\n");
                    send(client_s, response, sizeof(response), 0);
                    free(file_content);
                    fclose(file);
                    continue;
                }

                sprintf(response, "GET %s %ld", file_name, file_length);
                send(client_s, response, sizeof(response), 0);
                printf("Sending file: %s\n", file_name);
                size_t bytes_sent = send(client_s, file_content, file_length, 0);
                printf("BYTES SENT:%ld\n", bytes_sent);
                if(bytes_sent != file_length){
                    printf("Failed to send whole file");
                }

                fclose(file);
                free(file_content);

            }
            else{
                strcpy(response, "INVALID FILE\n");
                send(client_s, response, sizeof(response), 0);
            }
        }
        else{
            strcpy(response, "INVALID COMMAND\n");
            send(client_s, response, sizeof(response), 0);
        }
    }

    free(files);

    close(client_s);
    close(s);

    return 0;
}
