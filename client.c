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

void handle_getting(char *file_name, long file_size, int s){
    FILE *file = fopen(file_name, "a");
    long file_bytes = 0;
    int bytes_r;

    char response[2048];
    memset(response, 0, sizeof(response));

    printf("GETTING FILE...\n");
    bytes_r = recv(s, response, sizeof(response), 0);
    while(file_bytes != file_size){
        //ADD CHECK FOR NOT BEING ABLE TO RECEIVE FULL FILE
        sleep(1);
        if(bytes_r < 0){
            bytes_r = recv(s, response, sizeof(response), 0);
            continue;
        }
        else if(bytes_r == 0){
            fclose(file);
            free(file_name);
            printf("SERVER DISCONNECTED");
            return;
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
}

void handle_listing(long dir_size, int s){
    long ls_bytes = 0;
    int bytes_r;

    char response[2048];
    memset(response, 0, sizeof(response));

    printf("LISTING...\n");
    bytes_r = recv(s, response, sizeof(response), 0);
    while(ls_bytes != dir_size){
        //ADD CHECK FOR NOT BEING ABLE TO RECEIVE FULL LISTING
        if(bytes_r < 0){
            bytes_r = recv(s, response, sizeof(response), 0);
            continue;
        }
        else if(bytes_r == 0){
            break;
        }
        else if(bytes_r > 0){
            printf("RESPONSE");
            printf("%s", response);
            ls_bytes += bytes_r;
        }
        memset(response, 0, sizeof(response));
        bytes_r = recv(s, response, sizeof(response), 0);
        
    }
    printf("LISTING DONE\n");
}

void handle_putting(char *file_name, long file_length, int s){

    char *file_content = malloc(file_length);
    if(file_content == NULL){
        printf("Failed to allocate memory");
        free(file_name);
        //SEND TO SERVER THAT IT FAILED
        return;
    }
    FILE *file = fopen(file_name, "r");
    if(file == NULL){
        printf("Failed to open file");
        free(file_name);
        free(file_content);
        //SEND TO SERVER THAT IT FAILED
        return;
    }
    size_t bytes_read = fread(file_content, sizeof(char), file_length, file);
    if(bytes_read != file_length){
        printf("Failed to read the file");
        //SEND TO SERVER THAT IT FAILED
        free(file_name);
        free(file_content);
        fclose(file);
        return;
    }
    size_t bytes_sent = send(s, file_content, file_length, 0);
    if(bytes_sent != file_length){
        printf("Failed to send whole file");
    }

    free(file_name);
    free(file_content);
    fclose(file);
}

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

    while(1){
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));

        if(fgets(request, sizeof(request), stdin) != NULL){
            request[strcspn(request, "\n")] = '\0';

            send(s, request, sizeof(request), 0);
        }

        int bytes_r;
        while((bytes_r = recv(s, response, sizeof(response), 0)) > 0){
            printf("Server: %s\n", response);
            opcode = strtok(response, " ");
            if(opcode != NULL){
                //getting
                if(strcmp(opcode, "GET") == 0){
                    file_name = strtok(NULL, " ");
                    file_size = strtol(strtok(NULL, " "), NULL, 10);
                    if(file_name != NULL){
                        file_name = strdup(file_name);
                        file = fopen(file_name, "w");
                        fclose(file);

                        handle_getting(file_name, file_size, s);
                    }
                }
                //listing
                else if(strcmp(opcode, "LS") == 0){
                    dir_size = strtol(strtok(NULL, " "), NULL, 10);
                    handle_listing(dir_size, s);
                }
                //putting
                else if(strcmp(opcode, "PUT") == 0){
                    file_name = strtok(NULL, " ");
                    if(file_name != NULL){
                        file_name = strdup(file_name);
                        file = fopen(file_name, "r");
                        if(file != NULL){
                            fseek(file, 0, SEEK_END);
                            long file_length = ftell(file);
                            fclose(file);
                            char c_response[1024] ={0};
                            snprintf(c_response, sizeof(c_response), "%s %ld", file_name, file_length);
                            send(s, c_response, sizeof(c_response), 0);
                            handle_putting(file_name, file_length, s);
                        }
                        else{
                            printf("File does not exist\n");
                            free(file_name);
                        }
                    }
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
