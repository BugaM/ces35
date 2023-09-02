/* This is the server code */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree */
#define BUFSIZE 4096     /* block transfer size */
#define QUEUE_SIZE 10

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int parseClientMessage(const char *message) {
    int clientNumber = -1; // Initialize to a default value, indicating failure

    // Use sscanf to parse the message
    int result = sscanf(message, "Client %d. Connection required.", &clientNumber);

    if (result == 1) {
        // sscanf successfully parsed one integer, which is the client number
        return clientNumber;
    } else {
        // Parsing failed or didn't find the expected format
        return -1; // Indicate failure
    }
}

void wait_for_response(int sa, char *buf){
    int bytes;
    while (1){
        bytes = read(sa, buf, BUFSIZE); /* read from client */
        if (bytes > 0){
            buf[bytes] = '\0'; // Null-terminate the received message
            printf("Received from client: %s\n", buf);
            break;
        }
    }
}

void send_to_client(int sa, char *message){
    write(sa, message, strlen(message)); 
    printf("Sent to client: %s\n", message);
}

int main(int argc, char *argv[])
{
    int clientNumber, s, b, l, fd, sa, bytes, on = 1;
    char buf[BUFSIZE];  /* buffer for outgoing file */
    char *message;
    int x, y;
    struct sockaddr_in channel; /* holds IP address */

    /* Build address structure to bind to socket. */
    memset(&channel, 0, sizeof(channel));
    /* zero channel */
    channel.sin_family = AF_INET;
    channel.sin_addr.s_addr = htonl(INADDR_ANY);
    channel.sin_port = htons(SERVER_PORT);

    /* Passive open. Wait for connection. */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
    if (s < 0)
    {
        printf("socket call failed");
        exit(-1);
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    b = bind(s, (struct sockaddr *)&channel, sizeof(channel));
    if (b < 0)
    {
        printf("bind failed");
        exit(-1);
    }

    l = listen(s, QUEUE_SIZE); /* specify queue size */
    if (l < 0)
    {
        printf("listen failed");
        exit(-1);
    }

    /* Socket is now set up and bound. Wait for connection and process it. */
    while (1)
    {
        sa = accept(s, 0, 0); /* block for connection request */
        if (sa < 0)
        {
            printf("accept failed");
            exit(-1);
        }
        
        wait_for_response(sa, buf);

        clientNumber = parseClientMessage(buf);
        asprintf(&message, "Authorized! Send me your position.");

        send_to_client(sa, message);

        wait_for_response(sa, buf);

        while(1){
            printf("Type which message to send to client %d: ", clientNumber);
            char inp = getchar();
            clearInputBuffer();
            if (inp == '2'){
                asprintf(&message, "Send speed and destination data.");
                send_to_client(sa, message);
                wait_for_response(sa, buf);
            }
            else if (inp == '3'){
                printf("\nX value: ");
                scanf("%d", &x);
                clearInputBuffer();
                
                printf("\nY value: ");
                scanf("%d", &y);
                clearInputBuffer();
                
                asprintf(&message, "Go to (%d,%d)", x, y);
                send_to_client(sa, message);
                wait_for_response(sa, buf);
            }
            else if (inp == 'q' || inp == 'Q'){
                printf("Connection aborted with client %d\n", clientNumber);
                break;
            }
        }

        close(sa); /* close connection */
    }

    // Close the listening socket (this will never be reached in the infinite loop)
    close(s);
}
