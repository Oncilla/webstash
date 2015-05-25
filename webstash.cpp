#include "webstash.h"

string *path;

int main(int argc, const char *argv[])
{
    if(argc <= 1){
        cout << string(help_text) << endl;
        return 0;
    }

    passwd* pw = getpwuid(getuid());
    path = new string(pw->pw_dir);

    ifstream  config_file((*path)+path_config);

    struct basic_input input;
    input.argc = argc;
    input.argv = argv;

    if(config_file.is_open()){
        config_file.getline(input.ip, IP_LENGTH);
        config_file.getline(input.port, PORT_LENGTH);
        config_file.getline(input.key, KEY_LENGTH);
        input.ip_length = strnlen(input.ip, IP_LENGTH);
        input.key_length = strnlen(input.key, KEY_LENGTH);
        input.port_length = strnlen(input.port, PORT_LENGTH);

        config_file.close();
    }else{
        cout << config_file_not_found << endl;
	return 0;
    }

    if(!strcmp(argv[1],config_string) || !strcmp(argv[1],config_string_short))
    {
        config(&input);
    }
    else if(!strcmp(argv[1],upload_string) || !strcmp(argv[1],upload_string_short))
    {
        request_upload(&input);
    }
    else if(!strcmp(argv[1],show_string) || !strcmp(argv[1],show_string_short))
    {
        request_show(&input);
    }
    else if(!strcmp(argv[1],download_string) || !strcmp(argv[1],download_string_short))
    {
        request_download(&input);
    }
    else if(!strcmp(argv[1],remove_string) || !strcmp(argv[1],remove_string_short))
    {
        request_remove(&input);
    }
    else{
        cout << string(help_text) << endl;
    }
    return 0;
}

void config(struct basic_input *input){

    if(input->argc == 2){
        ifstream  file((*path)+path_config);
        cout << file.rdbuf();
        file.close();
    }else if(input->argc % 2 == 1){
        cout << help_text << endl;
        return;
    }

    for(int i = 2; i < input->argc; i+=2){
        if(!strcmp(input->argv[i],config_ip_string)){
            strncpy(input->ip,input->argv[i+1],IP_LENGTH);
        }else if(!strcmp(input->argv[i],config_key_string)){
            strncpy(input->key,input->argv[i+1],KEY_LENGTH);
        }else if(!strcmp(input->argv[i],config_port_string)){
            strncpy(input->port,input->argv[i+1],PORT_LENGTH);
        }else{
            cout << help_text << endl;
            return;
        }
    }

    ofstream  out_file((*path)+path_config);

    out_file.write(input->ip,strnlen(input->ip,IP_LENGTH-1));
    out_file.write("\n",1);
    out_file.write(input->port,strnlen(input->port,PORT_LENGTH-1));
    out_file.write("\n",1);
    out_file.write(input->key,strnlen(input->key,KEY_LENGTH-1));
    out_file.write("\n",1);

    out_file.close();

}

void request_show(struct basic_input *input){
    char buffer[BUFFER_SIZE];
    int sockfd = setup_socket(input);

    send_function_header(SHOW, 0, sockfd);

    // TODO check signal
    read_signal(sockfd);

    send_status(STATUS_START_SENDING,sockfd);

    int n;
    while((n = read_buffer(buffer,sockfd,true)) > 0){
        printf("%.*s",n,buffer);
    }
    close(sockfd);
}


void remove_file(int sock,const char *file_name){
    send_file_header(file_name,sock);

    if(read_signal(sock) == status[STATUS_DELETING_FAILED][0]){
        cout << "deleting "<<file_name<<" failed"<<endl;
        return;
    }
    cout << "deleting "<<file_name<<" succeded"<<endl;
}

void request_upload(struct basic_input *input){
    int sockfd = setup_socket(input);

    send_function_header(UPLOAD,input->argc-2,sockfd);

    // TODO check signal
    read_signal(sockfd);

    for(int64_t i = 2; i < input->argc; i++){
        upload_file((const char*)input->argv[i], sockfd);
    }

    close(sockfd);
}


void upload_file(const char* file_name, int sock){

    std::ifstream is (file_name, std::ifstream::binary);
    if (is) {
        // get length of file:
        is.seekg (0, is.end);
        int64_t length = is.tellg();
        is.seekg (0, is.beg);

        send_file_header(file_name,sock,length);

        // TODO handle signal
        read_signal(sock);

        cout << "upload "<< file_name<< "of length "<< length;
        char buffer[BUFFER_SIZE];
        while(length > BUFFER_SIZE){
            is.read (buffer,BUFFER_SIZE);
            send_buffer(buffer, BUFFER_SIZE, sock, true);
            length -= BUFFER_SIZE;
        }

        if(length > 0){
            is.read (buffer,length);
            send_buffer(buffer, length, sock, true);
        }

        // TODO handle signal
        read_signal(sock);

        is.close();
        cout << " done"<<endl;
    }

}


void request_download(struct basic_input *input){
    int sockfd = setup_socket(input);

    send_function_header(DOWNLOAD,input->argc-2,sockfd);

    // TODO handle signal
    read_signal(sockfd);

    for(int i = 2; i < input->argc; i++){
        download_file(sockfd, input->argv[i]);
    }
}


void request_remove(struct basic_input *input){
    int sockfd = setup_socket(input);

    send_function_header(REMOVE,input->argc-2,sockfd);

    // TODO handle signal
    read_signal(sockfd);

    for(int i = 2; i < input->argc; i++){
        remove_file(sockfd, input->argv[i]);
    }



}


void download_file(int sock,const char *file_name){
    char buffer[BUFFER_SIZE];
    int64_t file_length = 0;

    send_file_header(file_name,sock);

    if(read_file_header(sock,&file_length) == status[STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED][0]){
        cout << "file name: " << file_name << " not allowed" << endl;
        return;
    }

    cout << "download: "<< file_name << " of length " << file_length;

    send_status(STATUS_START_SENDING,sock);

    ofstream  out_file(file_name);
    while(file_length > 0){

        int size = (file_length>BUFFER_SIZE)?BUFFER_SIZE:file_length;

        read_buffer(buffer,sock,true);
        out_file.write(buffer,size);

        file_length -= size;
    }

    out_file.close();
    cout << " done" << endl;
}
