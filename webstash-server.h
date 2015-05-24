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
#define TEMPORARY_KEY 8

#define STATUS_FUNCTION_HEADER_OK 0
#define STATUS_FUNCTION_HEADER_NO_SUCH_FUNCTION 1
#define STATUS_FILE_HEADER_OK 2
#define STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED 3
#define STATUS_FILE_HEADER_DONE_WRITING 4
#define STATUS_FILE_HEADER_FAILED_WRITING 5
#define STATUS_START_SENDING 6
#define STATUS_DONE_DELETING 7
#define STATUS_DELETING_FAILED 8

#define BUFFER_SIZE 256
#define KEY_LENGTH 256
#define PORT_LENGTH 50


#define path_config "/.webstash/.config-server"
#define path_stash "/.webstash/stash/"

using namespace std;

struct basic_input
{

    char key[KEY_LENGTH] = {0};
    int key_length;
    char port[PORT_LENGTH] = {0};
    int port_length;
    int argc;
    char **argv;
};


const char *status[] = {"0","1","2","3","4","5","6","7","8"};

const char *help_text = "Webstash is a service to easily stash files on a server.\nTo start the server just call webstash-server.\n\nAdditional options:\n\n-c [--config]: configure the server with parameters\n\t[none] show config\n\t-p set port\n\t-k set key\nconfigure file stored at ~/.webstash/.config-server.txt";
const char *config_file_not_found = "Config file not found";

const char *config_string = "--config";
const char *config_string_short = "-c";

const char *config_port_string = "-p";
const char *config_key_string = "-k";


void doprocessing (int sock);
void process_upload_request(int sock);
void process_download_request(int sock);
void process_remove_request(int sock);
void process_show_request(int sock);
void config(struct basic_input *input);

int check_file_name(char *file_name, int max_size);


int send_buffer(const char *buffer, int size, int sock, bool close_socket)
{
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


int read_buffer(char *buffer, int sock, bool close_socket)
{
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

void read_function_header(int sock, int64_t *function_id , int64_t *number_of_files)
{
    char buffer[BUFFER_SIZE];

    read_buffer(buffer,sock,true);

    mempcpy(function_id,buffer,sizeof(int64_t));
    mempcpy(number_of_files,buffer+sizeof(int64_t),sizeof(int64_t));
}

int send_status(int STATUS_IDENTIFIER, int sock)
{
    return send_buffer(status[STATUS_IDENTIFIER], 1, sock, true);
}


void read_file_header(int sock, char *file_name, int64_t* file_length)
{
    char buffer[BUFFER_SIZE];
    read_buffer(buffer,sock,true);
    mempcpy(file_length,buffer,sizeof(int64_t));
    mempcpy(file_name,buffer+sizeof(int64_t),BUFFER_SIZE-sizeof(int64_t));
}

int read_signal(int sockfd)
{
    char buffer[BUFFER_SIZE];

    read_buffer(buffer,sockfd,true);

    return buffer[0];
}


#endif // MAIN_H_INCLUDED
