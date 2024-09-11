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

static const char* broadcast_ip = "255.255.255.255";

static int set_non_blocking_socket(int sockfd) {
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

int ch_broadcast( channel_t* ch, const void* msg, uint32_t msg_size ) {
  assert( ch->is_udp );

  struct sockaddr_in broadcast_addr;
  memset(&broadcast_addr, 0, sizeof(broadcast_addr));
  broadcast_addr.sin_family = AF_INET;
  broadcast_addr.sin_port = htons( ch->port );
  
  // Convert the broadcast IP address string to binary format
  if (inet_pton(AF_INET, broadcast_ip, &broadcast_addr.sin_addr) <= 0) {
      perror("inet_pton");
      ch_close( ch );
      return -1;
  }

  // Do the actual broadcast
  int rc = sendto(ch->fd, msg, msg_size, 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
  if( rc < 0) {
      perror("sendto");
      ch_close( ch );
      return -1;
  }

  return rc;
}

static void ch_clean( channel_t* ch ) {
  // Clear to some sane values
  ch->fd = 0;
  ch->is_udp = false;
  ch->is_broadcast = false;
  ch->is_server = false;
  ch->port = 0;
}

int ch_create( channel_t* ch, const char* conn_info ) {
  assert( ch );
  assert( conn_info );

  // Clear to some sane values
  ch_clean( ch );

  const char* str_ip = strchr( conn_info, ':' ) + 1;
  const char* str_port = strchr( str_ip, ':' );
  if( !str_port ) {
    printf( "Failed to identify port number\n");
    return -2;
  }

  char ip[32];
  memset( ip, 0x00, sizeof(ip));
  strncpy( ip, str_ip, str_port - str_ip );
  int port = atoi( str_port + 1 );
  printf( "IP: %s\n", ip );
  printf( "Port: %d\n", port );

  int sockfd = 0;
  if( strncmp( conn_info, "udp:", 4 ) == 0 ) {
    ch->is_udp = true;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
      perror("socket");
      return -3;
    }

    // Enable the broadcast option
    bool is_broadcast = strcmp( ip, broadcast_ip ) == 0;
    if( is_broadcast ) {
      int broadcast_permission = 1;
      if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0) {
          perror("setsockopt (SO_BROADCAST)");
          close(sockfd);
          return -10;
      }

      struct sockaddr_in broadcast_addr;    
      memset(&broadcast_addr, 0, sizeof(broadcast_addr));
      broadcast_addr.sin_family = AF_INET;
      broadcast_addr.sin_port = htons(port);
      if (inet_pton(AF_INET, ip, &broadcast_addr.sin_addr) <= 0) {
          perror("inet_pton");
          close(sockfd);
          return -11;
      }
    }

  }

  else if( strncmp( conn_info, "tcp", 3 ) == 0 ) {
    ch->is_server = conn_info[3] == '_';  // tcp_server

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("socket");
      return -3;
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
        return -4;
    }

    if( ch->is_server ) {

      if (bind(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("bind");
        return -7;
      }

      if (listen(sockfd, 5) < 0) {
        perror("listen");
        return -7;
      }

    } else {

      if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
        if (errno != EINPROGRESS) {
          // EINPROGRESS indicates that the connection is in progress in non-blocking mode
          perror("connect");
          close(sockfd);
          return -5;
        } else {
          printf("Connecting...\n");
        }
      } else {
          // The connection was successful. (in blocking situations)
      }
    }

    if (set_non_blocking_socket(sockfd) < 0) {
      close(sockfd);
      return -6;
    }

  }

  ch->port = port;
  ch->fd = sockfd;
  return 0;
}

int ch_read( channel_t* ch, uint8_t *out_buffer, uint32_t max_length ) {
  int sockfd = ch->fd;
  uint32_t total_bytes_read = 0;
  while (total_bytes_read < max_length) {

      // Use select() to wait for the socket to be ready for reading
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(sockfd, &read_fds);
      struct timeval tv = {0, 0};

      int ret = select(sockfd + 1, &read_fds, NULL, NULL, &tv);
      if (ret < 0) {
          perror("select");
          return -1;

      } else if (ret > 0 && FD_ISSET(sockfd, &read_fds)) {
          int bytes_read = read(sockfd, out_buffer + total_bytes_read, max_length - total_bytes_read);
          if (bytes_read < 0) {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                  break;
              } else {
                  perror("read");
                  return -1;
              }
          } else if (bytes_read == 0) {
              break;
          }
          total_bytes_read += bytes_read;

      // Nothing to read. timeout
      } else if( ret == 0 ) {
          break;
      }
  }

  return (int)total_bytes_read;
}

int ch_write( channel_t* ch, const void* buffer, uint32_t length ) {
  int sockfd = ch->fd;
  int total_bytes_written = 0;

  while (total_bytes_written < (int)length) {

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
                  break;

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

void ch_wait( int usecs ) {
  struct timeval tv = {usecs / 1000000, usecs % 1000000};
  select(0, NULL, NULL, NULL, &tv);
}

bool ch_accept( channel_t* server, channel_t* out_new_client, int usecs ) {

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(server->fd, &fds);

  struct timeval tv = {usecs / 1000000, usecs % 1000000};
  printf( "Waiting for accept: %d:%d\n", tv.tv_sec, tv.tv_usec);

  int ret = select(server->fd + 1, &fds, NULL, NULL, 0 ) ; //&tv);
  if (ret < 0) {
    perror("select");
    return false;
  }

  if( ret == 0 ) {
    printf( "No one is connecting...\n" );
    return false;
  }

  struct sockaddr_storage sa;
  socklen_t sa_sz = sizeof(sa);
  int new_fd = accept( server->fd, (struct sockaddr*)&sa, (socklen_t*) &sa_sz );
  if( new_fd < 0 )
    return false;

  set_non_blocking_socket( new_fd );

  ch_clean( out_new_client );

  out_new_client->fd = new_fd;
  return true;
}

