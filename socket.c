#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


// #define PROXY_HOST "134.209.29.120"
// #define PROXY_PORT 8080

int main(int argc, char *argv[]){
    if (argc != 3){
        puts("Parameters: <host> <method>");
        exit(1);
    }

    char *hostname;
    int port;

    hostname = argv[1];
    port = 80;
 
    const char *http_prefix = "http://";
    const char *https_prefix = "https://";
    int use_https = 0;

    if (strstr(hostname, https_prefix) == hostname){
        hostname += strlen(https_prefix);
        use_https = 1;
        port = 443;
    } else if (strstr(hostname, http_prefix) == hostname){
        hostname += strlen(http_prefix);
    } else {
        puts("URL should start with http:// or https://");
        exit(1);
    }

    char *path = strchr(hostname, '/');  
    if (path != NULL) {
        *path = '\0'; 
        path++;  
    } else {
        path = "";  
    }

    char request[512];
    char *post_data;

    if(strcmp(argv[2], "GET") != 0 && strcmp(argv[2], "POST") != 0){exit(1);}
    sprintf(request,
        "%s http://%s/%s HTTP/1.1\r\n"  
        "Host: %s\r\n"
        "\r\n",
        argv[2], hostname, path, hostname);


    struct hostent *host = gethostbyname(hostname);
    if (host == NULL) {
        perror("gethostbyname failed");
        exit(1);
    }

    int client;
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in remote_address;
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(port);
    memcpy(&(remote_address.sin_addr), host->h_addr_list[0], host->h_length);

    if (connect(client, (struct sockaddr*) &remote_address, sizeof(remote_address)) < 0) {
        perror("connection failed");
        exit(1);
    }

    char response[4096] = {0};

if (use_https) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        perror("SSL_CTX_new failed");
        exit(1);
    }
    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);

    if (SSL_connect(ssl) == -1) {
        ERR_print_errors_fp(stderr);
    }
    
    if (SSL_write(ssl, request, strlen(request)) < 0) {
        perror("SSL_write failed");
        exit(1);
    }
    
    int bytes = SSL_read(ssl, response, sizeof(response) - 1);
    response[bytes] = 0;

    SSL_free(ssl);
    SSL_CTX_free(ctx);
} else {
    if (send(client, request, strlen(request), 0) < 0) {
        perror("send failed");
        exit(1);
    }
    
    if (recv(client, response, sizeof(response) - 1, 0) < 0) {
        perror("recv failed");
        exit(1);
    }
}
    printf("Response from the server: %s\n", response);
    close(client);

    return 0;
}