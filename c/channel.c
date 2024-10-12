#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <string.h>
#include <stdio.h>      // perror
#include <stdlib.h>
#include "dbg.h"

// --------------------------------------------------------------------
#ifdef _WIN32

#define sys_close               closesocket
#define sys_error_code          WSAGetLastError()
#define CH_ERR_WOULD_BLOCK      WSAEWOULDBLOCK
#define CH_ERR_CONN_IN_PROGRESS WSAEWOULDBLOCK

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#define socklen_t int

// --------------------------------------------------------------------
#else

#include <errno.h>
#include <fcntl.h>      // for fcntl()
#include <unistd.h>     // for close()
#include <netinet/tcp.h>   // For TCP_NODELAY
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <sys/types.h>
#include <sys/select.h>
//#include <sys/uio.h>
#include <arpa/inet.h>  // for inet_addr(), struct sockaddr_in
#include <netdb.h>      // getaddrinfo
#include <ifaddrs.h>    // getifaddrs

#define sys_close               close
#define sys_error_code          errno
#define CH_ERR_WOULD_BLOCK      EWOULDBLOCK
#define CH_ERR_CONN_IN_PROGRESS EINPROGRESS

#endif

// --------------------------------------------------------------------
#include "channel.h"

static const char* broadcast_ip = "255.255.255.255";
#define invalid_socket_id            (~0)

static int set_non_blocking_socket(socket_t sockfd) {

#if defined(O_NONBLOCK)

  // Get the current file descriptor flags
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) {
      dbg( DbgError, "fcntl(F_GETFL)\n");
      return -1;
  }

  // Set the socket to non-blocking mode
  flags |= O_NONBLOCK;
  if (fcntl(sockfd, F_SETFL, flags) == -1) {
      dbg( DbgError, "fcntl(F_SETFL)\n");
      return -1;
  }

#else

  u_long iMode = 1;
  auto rc = ioctlsocket(sockfd, FIONBIO, &iMode);
  if (rc)
    return false;

#endif

  return 0;
}

static void set_tcp_no_delay(socket_t sockfd ) {
  int flag = 1;
  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(int)) < 0) {
    dbg( DbgError, "setsockopt(TCP_NODELAY) failed %d\n", sys_error_code);
  }
}

static void set_reuse_addr(socket_t sockfd ) {
  int optval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) < 0) {
    dbg( DbgError, "setsockopt failed %d\n", sys_error_code);
  }
}

static bool set_udp_broadcast(socket_t sockfd ) {
  int broadcast_permission = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast_permission, sizeof(broadcast_permission)) < 0) {
    dbg( DbgError, "setsockopt (SO_BROADCAST) %d\n", sys_error_code);
    return false;
  }
  return true;
}

void ch_close( channel_t* ch ) {
  sys_close( ch->fd );
  ch->fd = invalid_socket_id;
}

static bool ch_make_address( struct sockaddr_in* addr, const char* ip, int port ) {
  memset(addr, 0, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons( port );
  if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    dbg( DbgInfo, "ch_make_address.inet_pton failed %s:%d -> %d\n", ip, port, sys_error_code);
    return false;
  }
  return true;
}

int ch_broadcast( channel_t* ch, const void* msg, uint32_t msg_size, const char* broadcast_addr ) {
  assert( ch->is_udp );

  if( !ch->is_broadcast ) {

    if( !set_udp_broadcast( ch->fd ) ) {
      sys_close(ch->fd);
      return false;
    }
    //if( is_broadcast )
    dbg( DbgInfo, "UDP set_udp_broadcast OK\n");
    ch->is_broadcast = true;
  }

  struct sockaddr_in addr;
  if( !broadcast_addr )
    broadcast_addr = broadcast_ip;
  dbg( DbgInfo, "ch_broadcast on fd %d to %s\n", ch->fd, broadcast_addr ); 
  if( !ch_make_address( &addr, broadcast_addr, ch->port ) )
    return -1;

  // Do the actual broadcast
  int rc = sendto(ch->fd, msg, msg_size, 0, (struct sockaddr*)&addr, sizeof(addr));
  if( rc < 0) {
    dbg( DbgError, "ch_broadcast.sendto(%d bytes) ch->port:%d => %d (Err:%d)\n", msg_size, ch->port, rc, sys_error_code);
    ch_close( ch );
    return -1;
  }

  return rc;
}

void ch_clean( channel_t* ch ) {
  // Clear to some sane values
  ch->fd = invalid_socket_id;
  ch->is_udp = false;
  ch->is_broadcast = false;
  ch->is_server = false;
  ch->port = 0;
}

// ("127.0.0.1", port, AF_INET )
// ("::", port, AF_INET6 )

// tcp:192.168.1.27    8080 -> tcp client connection to server at port 192.168.1.27:8080
// tcp_server:0.0.0.0  4800 -> tcp server at port 4800
// udp:0.0.0.0         4700 -> Broadcast udp to port 4700
bool ch_create( channel_t* ch, const char* conn_info, int port ) {
  
  dbg( DbgInfo, "ch_create %s port:%d\n", conn_info, port);

  static bool global_initialization = false;
  if (!global_initialization) {

#ifdef _WIN32

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int wsaerr = WSAStartup(wVersionRequested, &wsaData);
    assert(wsaerr == 0);

#endif

    global_initialization = true;
  }


  assert( ch );
  assert( conn_info );

  // Clear to some sane values
  ch_clean( ch );

  const char* ip = strchr( conn_info, ':' ) + 1;

  //printf( "IP: %s\n", ip );
  //printf( "Port: %d\n", port );

  socket_t sockfd = -1;
  if( strncmp( conn_info, "udp:", 4 ) == 0 ) {
    ch->is_udp = true;
    dbg( DbgInfo, "UDP creation\n");

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
      dbg( DbgError, "udp socket creation failed %d\n", sys_error_code);
      return false;
    }
    dbg( DbgInfo, "UDP create fd => %d\n", sockfd);
  }
  else if( strncmp( conn_info, "tcp_server:", 11 ) == 0 ) {
    ch->is_server = true;

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // Use with NULL for bind

    char str_port[32];
    sprintf(str_port, "%d", port);

    if (getaddrinfo(ip, str_port, &hints, &servinfo) != 0) {
      dbg( DbgError, "tcp_server getaddrinfo failed %d", sys_error_code);
      return false;
    }

    for ( p = servinfo; p != NULL; p = p->ai_next) {

      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        continue;

      set_reuse_addr(sockfd);

      // Disable the default behavior where an IPv6 socket only accepts IPv6 connections
      if (p->ai_family == AF_INET6) {
        int no = 0;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&no, sizeof(no)) != 0) {
          dbg( DbgError, "tcp_server setsockopt.IPV6_V6ONLY failed %d\n", sys_error_code);
          sys_close(sockfd);
          continue;
        }
      }

      if (bind(sockfd, p->ai_addr, (int)p->ai_addrlen) < 0) {
        dbg( DbgError, "tcp_server bind failed %d\n", sys_error_code);
        sys_close(sockfd);
        continue;
      }
      
      if (listen(sockfd, 5) < 0) {
        dbg( DbgError, "tcp_server listen failed %d\n", sys_error_code);
        sys_close(sockfd);
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
      dbg( DbgError, "tcp socket creation failed %d\n", sys_error_code);
      return false;
    }

    struct sockaddr_in remote_addr;
    if( !ch_make_address( &remote_addr, ip, port )) {
      sys_close(sockfd);
      return false;
    }

    if (connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
      int err_code = sys_error_code;
      if (err_code != EINPROGRESS) {
        // EINPROGRESS indicates that the connection is in progress in non-blocking mode
        dbg( DbgError, "tcp connect failed %d\n", err_code);
        sys_close(sockfd);
        return false;

      } else {
        //printf("Connecting...\n");
      }
    }

    set_tcp_no_delay( sockfd );
  }
  else {
    dbg( DbgError, "ch_create invalid conn str" );
    return false;
  }

  if( !ch->is_udp ) {
    if (set_non_blocking_socket(sockfd) < 0) {
      dbg( DbgError, "tcp set_non_blocking_socket failed %d\n", sys_error_code);
      sys_close(sockfd);
      return false;
    }
  }

  ch->port = port;
  ch->fd = sockfd;
  return true;
}


// -------------------------------------------------------
#define OP_READ    1
#define OP_WRITE   2

bool ch_can_io( channel_t* ch, int io_op, struct timeval* tv ) {
  socket_t sockfd = ch->fd;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);

  fd_set* read_fds = (io_op == OP_READ ) ? &fds : NULL;
  fd_set* write_fds = (io_op == OP_WRITE ) ? &fds : NULL;

  // Use select() to wait for the socket to be ready for reading/writing
  int ret = select((int)(sockfd + 1), read_fds, write_fds, NULL, tv);
  if (ret < 0) {
    dbg( DbgError, "select failed %d\n", errno);
    return false;

  } else if (ret > 0 && FD_ISSET(sockfd, &fds)) {
    return true;
  }

  return false;
}

void ch_wait( int usecs ) {
  struct timeval tv = {usecs / 1000000, usecs % 1000000};
  select(0, NULL, NULL, NULL, &tv);
}

// -------------------------------------------------------
int ch_read( channel_t* ch, void *out_buffer, uint32_t max_length, int usecs ) {
  uint8_t* obuf = (uint8_t*) out_buffer;
  socket_t sockfd = ch->fd;
  uint32_t total_bytes_read = 0;

  // Max time to wait for an answer
  struct timeval tv = { usecs / 1000000, usecs % 1000000 };

  while (total_bytes_read < max_length) {

    if( !ch_can_io( ch, OP_READ, &tv ) )
      break;

    int bytes_read = recv(sockfd, obuf, max_length - total_bytes_read, 0);
    if (bytes_read < 0) {
        int err_code = sys_error_code;
        if (err_code == EAGAIN || err_code == CH_ERR_WOULD_BLOCK ) {
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

  }

  return (int)total_bytes_read;
}

int ch_write( channel_t* ch, const void* buffer, uint32_t length ) {
  const uint8_t* ibuf = (uint8_t*)buffer;
  socket_t sockfd = ch->fd;
  int total_bytes_written = 0;

  while (total_bytes_written < (int)length) {

    if( !ch_can_io( ch, OP_WRITE, NULL ) )
      break;

    int bytes_written = send(sockfd, ibuf, length - total_bytes_written, 0);
    if (bytes_written < 0) {
      int err_code = sys_error_code;
      if (err_code == EAGAIN || err_code == CH_ERR_WOULD_BLOCK ) {
        break;

      } else {
        perror("write");
        return -1;
      }
    }
    total_bytes_written += bytes_written;
    ibuf += bytes_written;
  }

  return total_bytes_written;
}

bool ch_accept( channel_t* server, channel_t* out_new_client, int usecs ) {

  struct timeval tv = {usecs / 1000000, usecs % 1000000};

  if( !ch_can_io( server, OP_READ, &tv ) )
    return false;

  struct sockaddr_storage sa;
  socklen_t sa_sz = sizeof(sa);
  socket_t new_fd = accept( server->fd, (struct sockaddr*)&sa, (socklen_t*) &sa_sz );
  if( new_fd < 0 )
    return false;

  set_non_blocking_socket( new_fd );
  set_tcp_no_delay( new_fd );

  ch_clean( out_new_client );

  out_new_client->fd = new_fd;
  //out_new_client->port = 
  return true;
}

void make_network_interface_t( network_interface_t* out_ni, const char* ip, const char* name, const char* broadcast ) {
  memset( out_ni, 0x00, sizeof( network_interface_t ) );
  strncpy( out_ni->ip, ip, sizeof( out_ni->ip ) - 1 );
  strncpy( out_ni->name, name, sizeof( out_ni->name ) - 1 );
  strncpy( out_ni->broadcast, broadcast, sizeof( out_ni->broadcast ) - 1 );
}

int  ch_get_local_network_interfaces( network_interface_t* out_interfaces, uint32_t max_interfaces ) {
  int num_interfaces = 0;

#ifdef _WIN32

  ULONG outBufLen = 15000;
  PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
  if (pAddresses == NULL)
    return -1;

  DWORD dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, pAddresses, &outBufLen);
  if (dwRetVal == NO_ERROR) {
    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
      char friendly_name[256]; size_t n = 0;
      wcstombs_s(&n, friendly_name, sizeof(friendly_name), pCurrAddresses->FriendlyName, 128);
      char description[256];
      wcstombs_s(&n, description, sizeof(description), pCurrAddresses->Description, 128);
      // friendly_name : Ethernet
      // AdapterName   : {EE2F68CC-5A8F-4655-B3A2-4340A8A8F71D}
      // description   : Intel(R) I211 Gigabit Network Connection
      // printf("Adapter name: %s (%s) : %s\n", friendly_name, description, pCurrAddresses->AdapterName);
      PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
      while (pUnicast != NULL) {
        SOCKADDR* sa = pUnicast->Address.lpSockaddr;
        if (sa->sa_family == AF_INET) {

          if( num_interfaces >= max_interfaces )
            break;
          
          char str[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &(((struct sockaddr_in*)sa)->sin_addr), str, INET_ADDRSTRLEN);
          make_network_interface_t( out_interfaces + num_interfaces, str, description,  );
          ++num_interfaces;
        }
        pUnicast = pUnicast->Next;
      }
      pCurrAddresses = pCurrAddresses->Next;
    }
  }
  free(pAddresses);

#else

  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1)
    return -1;

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

    if (ifa->ifa_addr == NULL) 
      continue;

    if( num_interfaces >= max_interfaces )
      break;

    if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
      if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST))
        continue;

      if( !ifa->ifa_netmask )
        continue;

      char mask[NI_MAXHOST];
      if (getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), mask, NI_MAXHOST, NULL, 0, NI_NUMERICHOST))
        continue;

      // Back str to ip4
      // From my IP     : 172.20.10.1
      // And broadcast  : 255.255.255.240
      // Broadcast addr : 172.20.10.15 
      struct in_addr ip_addr, subnet_mask, broadcast_addr;
      inet_pton(AF_INET, host, &ip_addr);
      inet_pton(AF_INET, mask, &subnet_mask);
      broadcast_addr.s_addr = ip_addr.s_addr | ~subnet_mask.s_addr;
      char broadcast_str[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &broadcast_addr, broadcast_str, INET_ADDRSTRLEN );

      dbg( DbgInfo, "Name:%-16s IP:%-16s Mask:%-16s Broadcast:%-16s\n", ifa->ifa_name, host, mask, broadcast_str );
      make_network_interface_t( out_interfaces + num_interfaces, host, ifa->ifa_name, broadcast_str );
      ++num_interfaces;
    }
  }

  freeifaddrs(ifaddr);

#endif

  return num_interfaces;  
}


