#include "webstash-server.h"

string *path;

int main( int argc, char *argv[] )
{

    passwd* pw = getpwuid(getuid());
    path = new string(pw->pw_dir);

    ifstream  config_file((*path)+path_config);

    struct basic_input input;
    input.argc = argc;
    input.argv = argv;

    if(config_file.is_open())
    {
        config_file.getline(input.port, PORT_LENGTH);
        config_file.getline(input.key, KEY_LENGTH);
        input.key_length = strnlen(input.key, KEY_LENGTH);
        input.port_length = strnlen(input.port, PORT_LENGTH);

        config_file.close();
    }
    else
    {
        cout << config_file_not_found << endl;
	return 0;
    }

    if(argc >1)
    {

        if(!strcmp(argv[1],config_string) || !strcmp(argv[1],config_string_short))
        {
            config(&input);
        }
        else
        {
            cout << help_text << endl;
        }

        return 0;
    }

    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int pid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5002;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    int yes=1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) { 
        perror("setsockopt"); 
        exit(1); 
    }  

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        /* Create child process */
        pid = fork();
        if (pid < 0)
        {
            close(sockfd);
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0)
        {
            /* This is the client process */
            close(sockfd);
            doprocessing(newsockfd);
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
    } /* end of while */
}

void doprocessing (int sock)
{
    int64_t function_id,number_of_files;

    read_function_header(sock,&function_id,&number_of_files);

    switch(function_id)
    {
    case UPLOAD:
        send_status(STATUS_FUNCTION_HEADER_OK,sock);

        for(int i = 0; i < number_of_files; i++)
        {
            process_upload_request(sock);
        }
        break;

    case DOWNLOAD:
        send_status(STATUS_FUNCTION_HEADER_OK,sock);

        for(int i = 0; i < number_of_files; i++)
        {
            process_download_request(sock);
        }
        break;

    case REMOVE:
        send_status(STATUS_FUNCTION_HEADER_OK,sock);

        for(int i = 0; i < number_of_files; i++)
        {
            process_remove_request(sock);
        }
        break;

    case SHOW:
        cout << "show" << endl;
        send_status(STATUS_FUNCTION_HEADER_OK,sock);

        process_show_request(sock);
        break;

    default:
        cout << "requested function "<<function_id<< " does not exist" << endl;
        send_status(STATUS_FUNCTION_HEADER_NO_SUCH_FUNCTION,sock);
    }

    close(sock);
}

void process_show_request(int sock)
{
    int link[2];

    pid_t pid;
    char buffer[BUFFER_SIZE];

    read_buffer(buffer,sock,true);


    if(pipe(link) == -1)
    {
        close(sock);
        cout << "pipe" <<endl;
        exit(EXIT_FAILURE);
    }

    if((pid = fork()) == -1)
    {
        close(sock);
        cout << "fork" <<endl;
        exit(EXIT_FAILURE);
    }

    if(pid == 0)
    {
        dup2(link[1],STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execl("/bin/ls", "ls" , ((*path)+path_stash).c_str() ,"-a","-1", (char *)0);
        exit(EXIT_SUCCESS);
    }
    else
    {
        close(link[1]);
        int nbytes;
        while((nbytes = read(link[0], buffer, sizeof(buffer))) >0)
        {
            send_buffer(buffer,nbytes,sock,true);
        }
        wait();
    }

}


void process_remove_request(int sock)
{
    char buffer[BUFFER_SIZE];

    read_buffer(buffer,sock,true);
    buffer[BUFFER_SIZE-1] = '\0';

    if(status[check_file_name(buffer,BUFFER_SIZE)][0] == status[STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED][0])
    {
	    cout << "deleting: bad file name "<<buffer<<endl;
        send_status(STATUS_DELETING_FAILED,sock);
        return;
    }

    remove(((*path)+path_stash+buffer).c_str());
    send_status(STATUS_DONE_DELETING,sock);
    cout << "deleted: "<<buffer<<endl;
}

void process_upload_request(int sock)
{
    int64_t file_length = 0;
    char buffer[BUFFER_SIZE];

    read_file_header(sock, buffer, &file_length);
    buffer[BUFFER_SIZE-1-sizeof(int64_t)] = '\0';

    cout << "client upload: "<< buffer << " of length " << file_length;

    send_status(check_file_name(buffer, BUFFER_SIZE),sock);

    ofstream  out_file(((*path)+path_stash+buffer).c_str());
    while(file_length > 0)
    {
        int size = read_buffer(buffer,sock,true);
        out_file.write(buffer,size);
        file_length -= size;
    }

    out_file.close();
    cout << " done" << endl;

    send_status(STATUS_FILE_HEADER_DONE_WRITING,sock);
}


void process_download_request(int sock)
{
    char buffer[BUFFER_SIZE];
    read_buffer(buffer,sock,true);
    buffer[BUFFER_SIZE-1] = '\0';

    cout << "client download: "<< buffer;

    if(status[check_file_name(buffer,BUFFER_SIZE)][0] == status[STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED][0])
    {
        send_status(STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED,sock);
        return;
    }

    std::ifstream is (((*path)+path_stash + buffer).c_str(), std::ifstream::binary);
    if (is.is_open())
    {
        // get length of file:
        is.seekg (0, is.end);
        int64_t length = is.tellg();
        is.seekg (0, is.beg);

        bzero(buffer,BUFFER_SIZE);
        buffer[0] = status[STATUS_FILE_HEADER_OK][0];
        mempcpy(buffer+1,&length, sizeof(int64_t));

        send_buffer(buffer,BUFFER_SIZE,sock,true);

        // TODO handle signal
        read_signal(sock);

        cout << length << endl;

        // send file
        while(length > 0)
        {
            int size = (BUFFER_SIZE > length)?length:BUFFER_SIZE;
            is.read (buffer,size);
            send_buffer(buffer,size,sock,true);
            length -= size;
        }
        cout << length << endl;
        is.close();

        cout << " done"<<endl;
    }else{
        send_status(STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED,sock);
        cout << " does not exist" << endl;
    }

}



int check_file_name(char *file_name, int max_size)
{
    for(uint64_t i = 0; i < strnlen(file_name, max_size); i++)
    {
        if(file_name[i] == '/')
        {
            return STATUS_FILE_HEADER_FILE_NAME_NOT_ALLOWED;
        }
    }

    return STATUS_FILE_HEADER_OK;
}

void config(struct basic_input *input)
{
    if(input->argc == 2){
        ifstream  file(((*path)+path_config).c_str());
        cout << file.rdbuf();
        file.close();
        return;
    }else if(input->argc % 2 == 1)
    {
        cout << help_text << endl;
        return;
    }

    for(int i = 2; i < input->argc; i+=2)
    {
        if(!strcmp(input->argv[i],config_key_string))
        {
            strncpy(input->key,input->argv[i+1],KEY_LENGTH);
        }
        else if(!strcmp(input->argv[i],config_port_string))
        {
            strncpy(input->port,input->argv[i+1],PORT_LENGTH);
        }
        else
        {
            cout << help_text << endl;
            return;
        }
    }

    ofstream  out_file(((*path)+path_config).c_str());

    out_file.write(input->port,strnlen(input->port,PORT_LENGTH-1));
    out_file.write("\n",1);
    out_file.write(input->key,strnlen(input->key,KEY_LENGTH-1));
    out_file.write("\n",1);

    out_file.close();

}

