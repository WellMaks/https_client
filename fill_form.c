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

void send_request(int client, SSL *ssl, int use_https, const char *request, char *response, size_t response_size) {
    memset(response, 0, response_size); // Reset the response buffer

    if (use_https) {
        if (SSL_write(ssl, request, strlen(request)) < 0) {
            perror("SSL_write failed");
            exit(1);
        }

        int bytes = SSL_read(ssl, response, response_size - 1);
        response[bytes] = 0;
    } else {
        if (send(client, request, strlen(request), 0) < 0) {
            perror("send failed");
            exit(1);
        }

        if (recv(client, response, response_size - 1, 0) < 0) {
            perror("recv failed");
            exit(1);
        }
    }
}

int main(int argc, char *argv[]){
    if (argc != 2){
        puts("Parameters: <host> ");
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

    sprintf(request,
        "GET http://%s/%s HTTP/1.1\r\n"  
        "Host: %s\r\n"
        "\r\n",
        hostname, path, hostname);

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

    SSL *ssl = NULL;
    SSL_CTX *ctx = NULL;

    if (use_https) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        const SSL_METHOD *method = TLS_client_method();
        ctx = SSL_CTX_new(method);
        
        if (!ctx) {
            perror("SSL_CTX_new failed");
            exit(1);
        }
        
        ssl = SSL_new(ctx);
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

    char *csrfToken = NULL;
    char *cookies = NULL;
    char *line = strtok(response, "\r\n"); // tokenize that bitch

    while(line) {
        if (strncmp(line, "set-cookie: XSRF-TOKEN=", 23) == 0){
            csrfToken = strdup(line + 23); // duplicate that bitch
        } else if (strncmp(line, "set-cookie: ", 12) == 0){
            char* equalSignPos = strchr(line + 12, '=');
            if (equalSignPos != NULL) {
                *equalSignPos = 0;
                cookies = strdup(line + 12 );
            }
        }
        line = strtok(NULL, "\r\n");
    }

    printf("Response from the server: %s\n", response);
    printf("CSRF Token: %s\n", csrfToken);
    printf("Cookies: %s\n", cookies);

    free(csrfToken);
    free(cookies);

    char *params = "param1=value1&param2=value2";

    sprintf(request,
        "POST /%s HTTP/1.1\r\n"  
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %lu\r\n"
        "X-CSRF-Token: %s\r\n"
        "Cookie: %s\r\n"
        "\r\n"
        "%s",
        path, hostname, strlen(params), csrfToken, cookies, params);


    send_request(client, ssl, use_https, request, response, sizeof(response));
    printf("Response from the server: %s\n", response);

    if (use_https) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
    }
    
    close(client);

    return 0;
}