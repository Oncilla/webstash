#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pwd.h>



#define UPLOAD 0
#define DOWNLOAD 1
#define SHOW 2
#define REMOVE 3
#define CONFIG_STATS 4
#define CONFIG_IP 5
#define CONFIG_PORT 6
#define CONFIG_KEY 7

#define STATUS_FUNCTION_HEADER_OK 0
#define STATUS_FUNCTION_HEADER_NO_SUCH_FUNCTION 1
#define STATUS_FILE_HEADER_OK 2
#define STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED 3
#define STATUS_FILE_HEADER_DONE_WRITING 4
#define STATUS_FILE_HEADER_FAILED_WRITING 5
#define STATUS_START_SENDING 6
#define STATUS_DONE_DELETING 7
#define STATUS_DELETING_FAILED 8

const char *status[] = {"0","1","2","3","4","5","6","7","8"};


#define IP_LENGTH 50
#define KEY_LENGTH 256
#define PORT_LENGTH 50
#define BUFFER_SIZE 256
#define path_config "/.webstash/.config"


using namespace std;

struct basic_input{

    char ip[IP_LENGTH] = {0};
    int ip_length;
    char key[KEY_LENGTH] = {0};
    int key_length;
    char port[PORT_LENGTH] = {0};
    int port_length;
    int argc;
    const char **argv;


};



const char *help_text = "Webstash is a service to easily stash files on a server.\nOptions:\n\n-c [--config]: configure the server with parameters\n\t[non] show config\n\t-i set ip\n\t-p set port\n\t-k set key\nconfigure file stored at ~/.webstash/.config.txt\n\n-u [--upload]: upload files to stash\n\n-d [--download]: download files from stash\n\n-s [--show]: show files in stash\n\n-r [--remove]: remove files from stash";
const char *config_file_not_found = "Config file not found";

const char *upload_string = "--upload";
const char *upload_string_short = "-u";

const char *download_string = "--download";
const char *download_string_short = "-d";

const char *show_string = "--show";
const char *show_string_short = "-s";

const char *remove_string = "--remove";
const char *remove_string_short = "-r";

const char *config_string = "--config";
const char *config_string_short = "-c";

const char *config_ip_string = "-i";
const char *config_port_string = "-p";
const char *config_key_string = "-k";

void request_download(struct basic_input *input);
void download_file(int sock,const char *file_name);
void request_remove(struct basic_input *input);
void remove_file(int sock,const char *file_name);
void request_upload(struct basic_input *input);
void upload_file(const char* file_name, int sock);
void request_show(struct basic_input *input);
void config(struct basic_input *input);



int setup_socket(struct basic_input *input){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    if (sockfd < 0)
    {
      perror("ERROR on binding");
      exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_aton(input->ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(atoi(input->port));

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        perror("ERROR connecting");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}


int send_buffer(const char *buffer, int size, int sock, bool close_socket){
    int n = write(sock,buffer,size);
    if (n < 0)
    {
        if(close_socket)
            close(sock);
        perror("ERROR writing to socket");
        exit(1);
    }
    return n;
}


int read_buffer(char *buffer, int sock, bool close_socket){
    int n = read(sock, buffer, BUFFER_SIZE);
    if (n < 0)
    {
        if(close_socket)
            close(sock);
        perror("ERROR on binding");
        exit(1);
    }
    return n;
}

int send_function_header(int64_t function_id, int64_t number_of_files, int sockfd){
    char buffer[BUFFER_SIZE];

    mempcpy(buffer,&function_id, sizeof(int64_t));
    mempcpy(buffer+sizeof(int64_t),&number_of_files, sizeof(uint64_t));

    return send_buffer(buffer,2*sizeof(int64_t), sockfd, true);
}

int read_signal(int sockfd)
{
    char buffer[BUFFER_SIZE];

    read_buffer(buffer,sockfd,true);

    return buffer[0];
}

int send_status(int STATUS_IDENTIFIER, int sock){
    return send_buffer(status[STATUS_IDENTIFIER], 1, sock, true);
}

int send_file_header(const char *file_name, int sock){
    char buffer[BUFFER_SIZE];

    bzero(buffer,BUFFER_SIZE);
    mempcpy(buffer,file_name, BUFFER_SIZE);
    buffer[BUFFER_SIZE-1] = '\0';
    return send_buffer(buffer,BUFFER_SIZE,sock,true);

}

int send_file_header(const char *file_name, int sock, int64_t file_length){
    char buffer[BUFFER_SIZE];

    bzero(buffer,BUFFER_SIZE);
    mempcpy(buffer,&file_length, sizeof(int64_t));
    mempcpy(buffer+sizeof(int64_t),file_name,strnlen(file_name,BUFFER_SIZE-sizeof(uint64_t)));
    buffer[BUFFER_SIZE-1] = '\0';
    return send_buffer(buffer,BUFFER_SIZE,sock,true);

}


int read_file_header(int sock, int64_t *file_length){
    char buffer[BUFFER_SIZE];
    read_buffer(buffer,sock,true);
    mempcpy(file_length,buffer+1,sizeof(int64_t));
    return buffer[0];

}



#endif // MAIN_H_INCLUDED
