#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree */
#define BUFSIZE 4096     /* block transfer size */
#define FIRST_MESSAGE "Authorized! Send me your position."
#define SECOND_MESSAGE "Send speed and destination data."

struct UAV {
    int x; // current position
    int y; // current position
    double speed;
    int x_f; // x coordinate at final destination
    int y_f;  // y coordinate at final destination
} client_uav;

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int parse_server_message(char *message) {
    int client_response = -1; // Initialize to a default value, indicating failure
    

    if (strcmp(message,FIRST_MESSAGE) == 0)
    {
        // server requests a response of TYPE 1
        client_response = 1;
    }
    else if (strcmp(message,SECOND_MESSAGE) == 0)
    {
        // server requests a response of TYPE 2
        client_response = 2;
    }
    else
    {
        // Use sscanf to parse the message
        if(sscanf(message, "Go to (%d,%d)", &client_uav.x_f, &client_uav.y_f) > 0)
        {
            // server requests a response of TYPE 3
            client_response = 3;
        }
    }

    // if parsing failed or didn't find the expected format, return -1
    return client_response;
}

void wait_for_response(int s, char *buf){
    int bytes;
    while (1)
    {
        bytes = read(s, buf, BUFSIZE); /* read from server */
        if (bytes > 0)
        {
            buf[bytes] = '\0'; // Null-terminate the received message
            printf("Received from server: %s\n", buf);
            break;
        }
    }
}

void send_to_server(int s, char *message){
    write(s, message, strlen(message)); 
    printf("Sent to server: %s\n", message);
}

int main(int argc, char **argv)
{
    int bytes;
    char buf[BUFSIZE]; /* buffer for incoming file */
    struct hostent *h; /* info about server */
    struct sockaddr_in channel; /* holds IP address */
    char *client_number, *message;

    // Define the client initial parameters
    client_uav.x = 2;
    client_uav.y = 3;
    client_uav.speed = 0.1;
    client_uav.x_f = 10;
    client_uav.y_f = 15;
    
    if (argc != 3)
    {
        printf("Usage: client server-name client-id\n");
        exit(-1);
    }
    client_number = argv[2];
    
    while (1)
    {
      int client_response, c, s; // Create a new socket for each connection

      h = gethostbyname(argv[1]); /* look up host’s IP address */

      if (!h)
      {
      printf("gethostbyname failed to locate %s\n", argv[1]);
      exit(-1);
      }

      s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (s < 0)
      {
      printf("socket call failed\n");
      exit(-1);
      }

      memset(&channel, 0, sizeof(channel));
      channel.sin_family = AF_INET;
      memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
      channel.sin_port = htons(SERVER_PORT);

      c = connect(s, (struct sockaddr *)&channel, sizeof(channel));
      if (c < 0)
      {
      printf("connect failed\n");
      exit(-1);
      }

      // Set the socket to non-blocking mode
      int flags = fcntl(s, F_GETFL, 0);
      fcntl(s, F_SETFL, flags | O_NONBLOCK);

      char input;

      printf("Initializing connection, sending the first message.");
          

      // FIRST MESSAGE ---------------------------------------------------------------------
      asprintf(&message, "Client %d. Connection required.", atoi(client_number));
      send_to_server(s, message);

      // Wait for a response from the server
      wait_for_response(s, buf);

      client_response = parse_server_message(buf);
      if (client_response != 1)
      {
            printf("Client received an incorrect server's response, so the connection was aborted.\n");
            break;
      }

      // Send the position back to the server
      asprintf(&message, "Client %d. My current position is (%d,%d)", atoi(client_number), client_uav.x, client_uav.y);
      send_to_server(s, message);

      // SECOND OR THIRD MESSAGE ---------------------------------------------------------------------
      while (1)
      {
            wait_for_response(s, buf);
            client_response = parse_server_message(buf);

            if (client_response == 1)
            {
            printf("Client received an inappropriate request and give back no response.");
            }
            else if (client_response == 2)
            {
            asprintf(&message, "Client %d update: current speed of %.2f. Destination: (%d,%d).", atoi(client_number), client_uav.speed, client_uav.x_f, client_uav.y_f);
            send_to_server(s, message);
            }
            else if (client_response == 3)
            {
            asprintf(&message, "Client %d recognized new destination (%d,%d).", atoi(client_number), client_uav.x_f, client_uav.y_f);
            send_to_server(s, message);
            }
            else
            {
            printf("Connection failed and aborted with server.\n");
            break;
            }
      }


      close(s); // Close the socket after sending and receiving a message
    }

    return 0;
}