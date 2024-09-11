#include <assert.h>
#include <string.h>
#include <stdio.h>      // printf
#include <stdlib.h>

#ifdef _WIN32

#define sys_close               closesocket
#define CH_ERR_WOULD_BLOCK      WSAEWOULDBLOCK
#define CH_ERR_CONN_IN_PROGRESS WSAEWOULDBLOCK

#pragma comment(lib, "Ws2_32.lib")

#else

#include <errno.h>
#include <fcntl.h>   // for fcntl()
#include <unistd.h>     // for close()
#include <netinet/tcp.h>   // For TCP_NODELAY
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>  // for inet_addr(), struct sockaddr_in
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define sys_close               close
#define CH_ERR_WOULD_BLOCK      EWOULDBLOCK
#define CH_ERR_CONN_IN_PROGRESS EINPROGRESS

#endif

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

/*
     bool setNonBlocking( TSocket sock ) {
        // set non-blocking
#if defined(O_NONBLOCK)
        int flags = fcntl(sock.s, F_GETFL, 0);
        if (flags == -1)
          flags = 0;
        auto rc = fcntl(sock.s, F_SETFL, flags | O_NONBLOCK);
#else
        u_long iMode = 1;
        auto rc = ioctlsocket(sock.s, FIONBIO, &iMode);
#endif
        if (rc != 0)
          dbg("Failed to set socket %d as non-blocking\n", sock.s);
        return rc == 0;
      }
      */

  return 0;
}

static void set_tcp_no_delay( int sockfd ) {
  int flag = 1;
  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
    perror("setsockopt(TCP_NODELAY) failed");
  }
}

static void set_reuse_addr( int sockfd ) {
  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror("setsockopt");
  }
}

static bool set_udp_broadcast( int sockfd ) {
  int broadcast_permission = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0) {
    perror("setsockopt (SO_BROADCAST)");
    return false;
  }
  return true;
}

void ch_close( channel_t* ch ) {
  sys_close( ch->fd );
  ch->fd = -1;
}

static bool ch_make_address( struct sockaddr_in* addr, const char* ip, int port ) {
  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons( port );
  if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
      perror("inet_pton");
      return false;
  }
  return true;
}

int ch_broadcast( channel_t* ch, const void* msg, uint32_t msg_size ) {
  assert( ch->is_udp );

  struct sockaddr_in addr;
  if( !ch_make_address( &addr, broadcast_ip, ch->port ) ) {
      ch_close( ch );
      return -1;
  }

  // Do the actual broadcast
  int rc = sendto(ch->fd, msg, msg_size, 0, (struct sockaddr*)&addr, sizeof(addr));
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

// ("127.0.0.1", port, AF_INET )
// ("::", port, AF_INET6 )

// tcp:192.168.1.27:8080    -> tcp client connection to server at port 192.168.1.27:8080
// tcp_server:0.0.0.0:4800  -> tcp server at port 4800
// udp:0.0.0.0:4700         -> Broadcast udp to port 4700
bool ch_create( channel_t* ch, const char* conn_info ) {
  assert( ch );
  assert( conn_info );

  // Clear to some sane values
  ch_clean( ch );

  const char* str_ip = strchr( conn_info, ':' ) + 1;
  const char* str_port = strchr( str_ip, ':' );
  if( !str_port ) {
    errno = ECANCELED;
    perror( "Failed to identify port number in conn_info");
    return false;
  }

  char ip[32];
  memset( ip, 0x00, sizeof(ip));
  strncpy( ip, str_ip, str_port - str_ip );
  str_port += 1;
  int port = atoi( str_port );
  //printf( "IP: %s\n", ip );
  //printf( "Port: %d\n", port );

  int sockfd = 0;
  if( strncmp( conn_info, "udp:", 4 ) == 0 ) {
    ch->is_udp = true;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
      perror("socket");
      return false;
    }

    // Enable the broadcast option
    bool is_broadcast = strcmp( ip, broadcast_ip ) == 0;
    if( is_broadcast && !set_udp_broadcast( sockfd ) )
      close(sockfd);
      return false;
    }

  }
  else if( strncmp( conn_info, "tcp_server:", 11 ) == 0 ) {
    ch->is_server = true;

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // No effect if bindaddr != NULL

    if (getaddrinfo(ip, str_port, &hints, &servinfo) != 0) {
      perror( "getaddrinfo" );
      return false;
    }

    for ( p = servinfo; p != NULL; p = p->ai_next) {

      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        continue;

      set_reuse_addr( sockfd );

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
        perror( "bind failed");
        close(sockfd);
        continue;
      }
      
      if (listen(sockfd, 5) < 0) {
        perror( "listen failed" );
        close(sockfd);
        continue;
      }

      break;
    }

    freeaddrinfo(servinfo);
    if (!p)
      return false;

  }
  else if( strncmp( conn_info, "tcp", 3 ) == 0 ) {

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("socket");
      return false;
    }

    struct sockaddr_in remote_addr;
    if( !ch_make_address( &remote_addr, ip, port )) {
      close(sockfd);
      return false;
    }

    if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
      if (errno != EINPROGRESS) {
        // EINPROGRESS indicates that the connection is in progress in non-blocking mode
        perror("connect");
        close(sockfd);
        return false;

      } else {
        //printf("Connecting...\n");
      }
    }

    set_tcp_no_delay( sockfd );
  }

  if( !ch->is_udp ) {
    if (set_non_blocking_socket(sockfd) < 0) {
      close(sockfd);
      return false;
    }
  }


  ch->port = port;
  ch->fd = sockfd;
  return true;
}

int ch_read( channel_t* ch, void *out_buffer, uint32_t max_length ) {
  uint8_t* obuf = (uint8_t*) out_buffer;
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
          int bytes_read = read(sockfd, obuf, max_length - total_bytes_read);
          if (bytes_read < 0) {
              if (errno == EAGAIN || errno == CH_ERR_WOULD_BLOCK ) {
                  break;
              } else {
                  perror("read");
                  return -1;
              }
          } else if (bytes_read == 0) {
              break;
          }
          total_bytes_read += bytes_read;
          obuf += bytes_read;

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
              if (errno == EAGAIN || errno == CH_ERR_WOULD_BLOCK ) {
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

  int ret = select(server->fd + 1, &fds, NULL, NULL, &tv);
  if (ret < 0)
    return false;

  if( ret == 0 )
    return false;

  struct sockaddr_storage sa;
  socklen_t sa_sz = sizeof(sa);
  int new_fd = accept( server->fd, (struct sockaddr*)&sa, (socklen_t*) &sa_sz );
  if( new_fd < 0 )
    return false;

  set_non_blocking_socket( new_fd );
  set_tcp_no_delay( new_fd );

  ch_clean( out_new_client );

  out_new_client->fd = new_fd;
  return true;
}




/*

get local ip in windows


#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int main() {
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
    if (pAddresses == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses, &outBufLen);
    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            printf("Adapter name: %s\n", pCurrAddresses->AdapterName);
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
            while (pUnicast != NULL) {
                SOCKADDR *sa = pUnicast->Address.lpSockaddr;
                if (sa->sa_family == AF_INET) {
                    char str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), str, INET_ADDRSTRLEN);
                    printf("IPv4 Address: %s\n", str);
                }
                pUnicast = pUnicast->Next;
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("GetAdaptersAddresses failed with error: %lu\n", dwRetVal);
    }

    free(pAddresses);
    return 0;
}

get local ip in unix

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Loop through the list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
                printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, host);
            }
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}


*/