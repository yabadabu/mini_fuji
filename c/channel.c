#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>   // for fcntl()
#include <unistd.h>     // for close()
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>  // for inet_addr(), struct sockaddr_in

#include "channel.h"

static int set_non_blocking(int sockfd) {
  // Get the current file descriptor flags
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) {
      perror("fcntl(F_GETFL)");
      return -1;
  }

  // Set the socket to non-blocking mode
  flags |= O_NONBLOCK;
  if (fcntl(sockfd, F_SETFL, flags) == -1) {
      perror("fcntl(F_SETFL)");
      return -1;
  }

  return 0;
}

void ch_close( channel_t* ch ) {
  close( ch->fd );
  ch->fd = -1;
}

int ch_open( channel_t* ch, const char* conn_info ) {
  assert( ch );
  assert( conn_info );
  
  if( strncmp( conn_info, "tcp:", 4 ) != 0 ) {
    printf( "We only support tcp now.\n");
    return -1;
  }
  const char* str_ip = conn_info + 4;
  const char* str_port = strchr( str_ip, ':' );
  if( !str_port ) {
    printf( "Failed to identify port number\n");
    return -1;
  }

  char ip[32];
  memset( ip, 0x00, sizeof(ip));
  strncpy( ip, str_ip, str_port - str_ip );
  int port = atoi( str_port + 1 );
  printf( "IP: %s\n", ip );
  printf( "Port: %d\n", port );

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return -1;
  }

  // Step 3: Configure the server address
  struct sockaddr_in remote_addr;
  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);  // Connect to port 8080 (adjust as needed)
  
  // Convert and set the server IP address (e.g., "192.168.1.1")
  if (inet_pton(AF_INET, ip, &remote_addr.sin_addr) <= 0) {
      perror("inet_pton");
      close(sockfd);
      return -2;
  }

  if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
    if (errno != EINPROGRESS) {
      // EINPROGRESS indicates that the connection is in progress in non-blocking mode
      perror("connect");
      close(sockfd);
      return -3;
    } else {
      printf("Connecting...\n");
    }
  } else {
      // The connection was successful immediately (which is rare in non-blocking mode)
      printf("Connected to the server!\n");
  }

  if (set_non_blocking(sockfd) < 0) {
    close(sockfd);
    return -1;
  }

  printf("Socket created\n");
  ch->fd = sockfd;

  return 0;
}

int ch_write( channel_t* ch, const void* data, uint32_t bytes_to_write ) {
  return 0;
}

/*

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>  // for select()

// Function to set the socket to non-blocking mode
int set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

// Function to write N bytes of data
int write_data(int sockfd, const char *buffer, int length) {
    int total_bytes_written = 0;

    while (total_bytes_written < length) {
        // Use select() to wait for the socket to be ready for writing
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &write_fds);

        int ret = select(sockfd + 1, NULL, &write_fds, NULL, NULL);
        if (ret < 0) {
            perror("select");
            return -1;
        } else if (ret > 0 && FD_ISSET(sockfd, &write_fds)) {
            int bytes_written = write(sockfd, buffer + total_bytes_written, length - total_bytes_written);
            if (bytes_written < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;  // Socket not ready, retry
                } else {
                    perror("write");
                    return -1;
                }
            }
            total_bytes_written += bytes_written;
        }
    }

    return total_bytes_written;
}

// Function to read up to N bytes of data
int read_data(int sockfd, char *buffer, int length) {
    int total_bytes_read = 0;

    while (total_bytes_read < length) {
        // Use select() to wait for the socket to be ready for reading
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        int ret = select(sockfd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            return -1;
        } else if (ret > 0 && FD_ISSET(sockfd, &read_fds)) {
            int bytes_read = read(sockfd, buffer + total_bytes_read, length - total_bytes_read);
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;  // Socket not ready, retry
                } else {
                    perror("read");
                    return -1;
                }
            } else if (bytes_read == 0) {
                // Connection closed
                break;
            }
            total_bytes_read += bytes_read;
        }
    }

    return total_bytes_read;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set non-blocking mode
    if (set_non_blocking(sockfd) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "192.168.1.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server (non-blocking)
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("connect");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    // Use select() to wait until the socket is ready to write (i.e., connected)
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);
    if (select(sockfd + 1, NULL, &write_fds, NULL, NULL) > 0) {
        if (FD_ISSET(sockfd, &write_fds)) {
            // The socket is now connected

            // Writing data
            const char *message = "Hello, server!";
            if (write_data(sockfd, message, strlen(message)) < 0) {
                fprintf(stderr, "Failed to send data.\n");
            }

            // Reading data (for example, up to 1024 bytes)
            char buffer[1024];
            int bytes_received = read_data(sockfd, buffer, sizeof(buffer));
            if (bytes_received > 0) {
                printf("Received data: %.*s\n", bytes_received, buffer);
            } else {
                printf("No data received or connection closed.\n");
            }
        }
    }

    // Close the socket when done
    close(sockfd);
    return 0;
}


*/



