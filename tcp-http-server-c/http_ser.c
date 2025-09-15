#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// Error handling
void error(const char *str) { perror(str); exit(1); }

// Mutex for file access
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Get MIME type
const char *get_file_type(const char *file_path) {
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) return "text/html";
        if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(ext, ".png") == 0) return "image/png";
        if (strcmp(ext, ".gif") == 0) return "image/gif";
        if (strcmp(ext, ".css") == 0) return "text/css";
        if (strcmp(ext, ".js") == 0) return "application/javascript";
    }
    return "application/octet-stream";
}

// Send file for GET
void serve_file(int newsockfd, const char *file_path) {
    FILE *file_pointer;

    pthread_mutex_lock(&file_mutex); // lock for read
    file_pointer = fopen(file_path, "rb");
    pthread_mutex_unlock(&file_mutex);

    if (!file_pointer) { 
        const char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<h1>File Not Found</h1>";
        write(newsockfd, response, strlen(response));
        return;
    }

    fseek(file_pointer, 0, SEEK_END);
    long file_length = ftell(file_pointer);
    rewind(file_pointer);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
        get_file_type(file_path), file_length);
    write(newsockfd, header, strlen(header));

    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), file_pointer)) > 0) 
        write(newsockfd, buffer, n);

    fclose(file_pointer);
}

// Handle client
void *handle_client(void *arg) {
    int newsockfd = *(int *)arg;
    free(arg);

    char buf[30000]; 
    bzero(buf, sizeof(buf));

    int n = read(newsockfd, buf, sizeof(buf) - 1);
    if (n <= 0) { close(newsockfd); pthread_exit(NULL); }
    buf[n] = '\0';

    // Print request headers only (up to first blank line)
    char *end_headers = strstr(buf, "\r\n\r\n");
    if (end_headers) *end_headers = '\0';
    printf("Received request:\n%s\n\n", buf);
    fflush(stdout);

    char method[10], path[100], version[10];
    sscanf(buf, "%s %s %s", method, path, version);
    printf("Method: %s\nPath: %s\nVersion: %s\n\n", method, path, version);
    fflush(stdout);

    // Prevent directory traversal
    if (strstr(path, "..")) {
        const char *resp = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<h1>400 Bad Request</h1>";
        write(newsockfd, resp, strlen(resp));
        close(newsockfd);
        pthread_exit(NULL);
    }

    char file_path[200] = "server_files";
    if (strcmp(path, "/") == 0) strcpy(path, "/index.html");
    strcat(file_path, path);

    if (strcmp(method, "GET") == 0) serve_file(newsockfd, file_path);

    else if (strcmp(method, "POST") == 0) {
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            pthread_mutex_lock(&file_mutex);
            FILE *fp = fopen(file_path, "w");
            if (fp) { fprintf(fp, "%s", body); fclose(fp);
                pthread_mutex_unlock(&file_mutex);
                const char *resp = "HTTP/1.1 201 Created\r\nContent-Type: text/html\r\nContent-Length: 20\r\n\r\n<h1>File Created</h1>";
                write(newsockfd, resp, strlen(resp));
            } else {
                pthread_mutex_unlock(&file_mutex);
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: 35\r\n\r\n<h1>Failed to save the file</h1>";
                write(newsockfd, resp, strlen(resp));
            }
        }
    }

    else if (strcmp(method, "DELETE") == 0) {
        pthread_mutex_lock(&file_mutex);
        if (remove(file_path) == 0) { pthread_mutex_unlock(&file_mutex);
            const char *resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 20\r\n\r\n<h1>File Deleted</h1>";
            write(newsockfd, resp, strlen(resp));
        } else { pthread_mutex_unlock(&file_mutex);
            const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<h1>File Not Found</h1>";
            write(newsockfd, resp, strlen(resp));
        }
    }

    else {
        const char *resp = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: 23\r\n\r\n<h1>405 Method Not Allowed</h1>";
        write(newsockfd, resp, strlen(resp));
    }

    close(newsockfd);
    pthread_exit(NULL);
}

// Main server
int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Port not provided\n"); exit(1); }
    int port_no = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");

    struct sockaddr_in serv_addr; 
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY; 
    serv_addr.sin_port = htons(port_no);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) error("Binding error");

    listen(sockfd, 50); // backlog 50
    printf("Server listening on port %d...\n", port_no);
    fflush(stdout);

    struct sockaddr_in cli_addr; 
    socklen_t clilen = sizeof(cli_addr);

    while (1) {
        int *newsockfd = malloc(sizeof(int));
        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*newsockfd < 0) { perror("Error accepting"); free(newsockfd); continue; }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, newsockfd);
        pthread_detach(tid);
    }

    close(sockfd);
    pthread_mutex_destroy(&file_mutex);
    return 0;
}
