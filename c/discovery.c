#include <string.h>
#include <stdlib.h>     // atoi
#include <assert.h>
#include "discovery.h"
#include "channel.h"
#include "blob.h"
#include "dbg.h"

typedef struct {
  blob_t    buff;
  blob_t    discovery_msg;
  channel_t ch_udp;
  channel_t ch_discovery;
  char      broadcast_addr[32];
} discovery_service_t;

#define DISCOVERY_BROADCAST_UDP_PORT    51562
#define DISCOVERY_SERVER_ANSWERS        51560

// Camera is listenning for msg in port 51562
// App creates a TCP Server listenning on port 51560
// App sends a udp broadcast msg with this IP address
// Camera gets the udp messages and creates a TCP connection to the host defined in the msg and port 51560
// Camera sends in this msg his IP and port, like 192.168.1.136 and 15740
// Camera closes the connection
// App can close the TCP server
// App starts a TCP connection to the camera IP and port

static discovery_service_t ds;

bool ch_read_blob( channel_t* ch, blob_t* blob, int usecs ) {
  blob_clear( blob );
  int bytes_read = ch_read( ch, blob_data(blob), blob_capacity( blob ), usecs );
  if( bytes_read > 0 ) {
    blob_resize( blob, bytes_read );
    return true;
  }
  return false;
}

// The local ip is required because it tells the camera where he needs to connect to
// The broadcast is required because in some sceneario the default 255.255.255.255
// might not reach the camera, for example when the iPhone is acting as access point to
// the camera and the broadcast is sent from the iPhone
bool discovery_start( const char* local_ip, const char* broadcast_addr ) {
  
  blob_create( &ds.buff, 0, 1024 );
  if( !ch_create( &ds.ch_udp, "udp:0.0.0.0", DISCOVERY_BROADCAST_UDP_PORT) )
    return false;

  strcpy( ds.broadcast_addr, broadcast_addr );

  // Msg to the cameras my identify. prefix + ip + suffix
  const char* prefix = "DISCOVERY * HTTP/1.1\r\nHOST: ";
  const char* suffix = "\r\nMX: 5\r\nSERVICE: PCSS/1.0\r\n";
  blob_t* msg = &ds.discovery_msg;
  blob_create( msg, 0, 512 );
  blob_append_data( msg, prefix, (uint32_t)strlen( prefix ) );
  blob_append_data( msg, local_ip, (uint32_t)strlen( local_ip ) );
  blob_append_data( msg, suffix, (uint32_t)strlen( suffix ) );

  // The camera will connect to this address via tcp and send his information
  if( !ch_create( &ds.ch_discovery, "tcp_server:0.0.0.0", DISCOVERY_SERVER_ANSWERS) )
    return false;
  assert( ds.ch_discovery.port = DISCOVERY_SERVER_ANSWERS );

  dbg( DbgInfo, "Discovery Msg: %s\n", ds.discovery_msg.data );

  return true;
}

bool discovery_update( camera_info_t* out_camera, int accept_time_usecs ) {
  assert( out_camera );

  // Send a udp msg until we get a camera responding
  int brc = ch_broadcast( &ds.ch_udp, blob_data( &ds.discovery_msg ), blob_size( &ds.discovery_msg ), ds.broadcast_addr );

  // The response is ... he tries to connect to the ip we sent and sends his ip
  channel_t ch_client;

  if( ch_accept( &ds.ch_discovery, &ch_client, accept_time_usecs ) ) {

    const int ms_to_usecs = 1000;
    while( !ch_read_blob( &ch_client, &ds.buff, 0 ) )
      ch_wait( ms_to_usecs );
    
    ch_close( &ch_client );

    /*
    NOTIFY * HTTP/1.1
    DSC: 192.168.1.136
    CAMERANAME: X-T2
    DSCPORT: 15740
    MX: 7
    SERVICE: PCSS/1.0
    */

    memset( out_camera, 0x00, sizeof( camera_info_t ));

    // Parse the results
    char* q0 = (char*) blob_data( &ds.buff );
    uint32_t msg_size = blob_size( &ds.buff );
    char* q = q0;
    while( q && ( q - q0 < msg_size ) ) {
      char* eol = strchr( q, '\r' );
      if( !eol )
        break;

      const char* sep = strchr( q, ':' );
      if( !sep )
        break;
      sep += 2;
      if (eol - sep > 0) {
        if( strncmp( q, "DSCPORT: ", 9 ) == 0 ) 
          out_camera->port = atoi( sep );
        else if( strncmp( q, "DSC: ", 5 ) == 0 )
          memcpy( out_camera->ip, sep, eol - sep);
        else if( strncmp( q, "CAMERANAME: ", 12 ) == 0 )
          memcpy( out_camera->name, sep, eol - sep);
      }
      q = eol + 2;
    }

    //printf( "Camera ip is >>%s:%d<< Name:>>%s<<\n", out_camera->ip, out_camera->port, out_camera->name );
    return out_camera->port > 0;
  }

  return false;
}

void discovery_stop( ) {
  blob_destroy( &ds.discovery_msg );
  blob_destroy( &ds.buff );
  ch_close( &ds.ch_udp );
  ch_close( &ds.ch_discovery );
}
