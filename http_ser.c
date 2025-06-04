#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//error function
void error(const char *str) {
    perror(str);
    exit(1);
}

// determine the file type based on extension
const char *get_file_type(const char *file_path) {
    const char *ext = strrchr(file_path, '.');
    if (ext != NULL) {
        // if we have any valid file type 
        if (strcmp(ext, ".html") == 0) return "text/html";
        if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(ext, ".png") == 0) return "image/png";
        if (strcmp(ext, ".gif") == 0) return "image/gif";
        if (strcmp(ext, ".css") == 0) return "text/css";
        if (strcmp(ext, ".js") == 0) return "application/javascript";
    }
    return "application/octet-stream";  // unknown type ka default type
}

// serve the requested file
void serve_file(int newsockfd, const char *file_path) {
    FILE *file_pointer = fopen(file_path, "rb"); 
    if(file_pointer == NULL) {
        const char *response = "HTTP/1.1 404 Not Found\r\n" "Content-Type: text/html\r\n" "Content-Length: 22\r\n" "\r\n" "<h1>File Not Found</h1>";
        write(newsockfd, response, strlen(response));
        return;
    }

    // if file found, determine type first
    const char *file_type = get_file_type(file_path);

    // response header
    char header[256];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\n" "Content-Type: %s\r\n" "\r\n", file_type);
    write(newsockfd, header, strlen(header));

    // response content
    char file_buffer[1024];
    size_t file_size;
    while((file_size = fread(file_buffer, 1, sizeof(file_buffer), file_pointer)) > 0) {
        write(newsockfd, file_buffer, file_size);
    }
    fclose(file_pointer);
}

int main(int argc, char *argv[]) {
    if(argc < 2){
        fprintf(stderr, "port number not provided. program terminated.\n");
        exit(1);
    }

    // TCP Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) error("Error opening socket.\n");

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));

    int port_no = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) error("Binding error.\n");

    listen(sockfd, 3);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // keep listening for client requests and serving files one after another
    while(1) {
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd < 0) error("Error accepting.\n");

        char buffer[30000];
        bzero(buffer, 30000);

        int n = read(newsockfd, buffer, 29999);
        if (n < 0) error("Error reading from socket.");
        buffer[n] = '\0';
        printf("Received request:\n%s\n", buffer);

        char method[10], path[100], version[10];
        sscanf(buffer, "%s %s %s", method, path, version);
        printf("Method: %s\nPath: %s\nVersion: %s\n", method, path, version);

        if (strcmp(method, "GET") == 0) {
            // if path is /, serve index.html
            if(strcmp(path, "/") == 0) strcpy(path, "/index.html");

            //add directory to path
            char file_path[100] = "server_files";
            strcat(file_path, path);

            serve_file(newsockfd, file_path);
        } 
        else {
            const char *response =
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 23\r\n"
                "\r\n"
                "<h1>405 Method Not Allowed</h1>";
            write(newsockfd, response, strlen(response));
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
