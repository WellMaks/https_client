#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    char *hostname;
    hostname = argv[1];
    const char *http_prefix = "http://";

    if (strstr(hostname, http_prefix) == hostname){
        hostname += strlen(http_prefix); 
    }

    printf("%s",hostname);
    return 0;
}


    // if(strcmp(argv[2], "GET") == 0){
    //     sprintf(request,
    //     "GET /%s HTTP/1.1\r\n"  
    //     "Host: %s\r\n"
    //     "\r\n",
    //     path, hostname);
    // }else if (strcmp(argv[2], "POST") == 0){
    //     post_data = "key=value&key2=value2";
    //     sprintf(request, "POST /%s HTTP/1.1\r\n"
    //             "Host: %s\r\n"
    //             "Content-Type: application/x-www-form-urlencoded\r\n"
    //             "Content-Length: %lu\r\n\r\n"
    //             "%s", path, hostname, strlen(post_data), post_data);
    // }else{
    //     printf("Invalid method\n");
    //     exit(1);
    // }