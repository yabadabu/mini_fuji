#include <string.h>
//#include <stdio.h>
#include <stdlib.h>     // atoi
#include <assert.h>
#include "discovery.h"
#include "channel.h"
#include "blob.h"

typedef struct {
  blob_t    buff;
  blob_t    discovery_msg;
  channel_t ch_udp;
  channel_t ch_discovery;
} discovery_service_t;

static discovery_service_t ds;

bool ch_read_blob( channel_t* ch, blob_t* blob ) {
  int bytes_read = ch_read( ch, blob->data, blob->reserved );
  if( bytes_read > 0 ) {
    blob->count = bytes_read;
    return true;
  }
  return false;
}

// "192.168.1.136"
bool discovery_start( const char* local_ip ) {
  blob_create( &ds.buff, 0, 1024 );
  if( !ch_create( &ds.ch_udp, "udp:255.255.255.255:5002") )
    return false;

  // Msg to the cameras my identify. prefix + ip + suffix
  const char* prefix = "DISCOVERY * HTTP/1.1\r\nHOST: ";
  const char* suffix = "\r\nMX: 5\r\nSERVICE: PCSS/1.0\r\n";
  blob_t* msg = &ds.discovery_msg;
  blob_create( msg, 0, 512 );
  blob_append_data( msg, prefix, strlen( prefix ) );
  blob_append_data( msg, local_ip, strlen( local_ip ) );
  blob_append_data( msg, suffix, strlen( suffix ) );

  // The camera will connect to this address via tcp and send his information
  if( !ch_create( &ds.ch_discovery, "tcp_server:0.0.0.0:51560") )
    return false;
  assert( ds.ch_discovery.port = 51560 );

  return true;
}

bool discovery_update( camera_info_t* out_camera, int accept_time_msecs ) {
  assert( out_camera );

  // Send a udp msg until we get a camera respondring
  int brc = ch_broadcast( &ds.ch_udp, ds.discovery_msg.data, ds.discovery_msg.count );

  // The response is ... he tries to connect to the ip we sent and sends his ip
  channel_t ch_client;

  int ms_to_usecs = 1000;

  if( ch_accept( &ds.ch_discovery, &ch_client, accept_time_msecs * ms_to_usecs ) ) {
    assert( ds.buff.reserved > 0 );

    while( !ch_read_blob( &ch_client, &ds.buff ) )
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
    const char* q0 = (const char*) ds.buff.data;
    uint32_t msg_size = blob_size( &ds.buff );
    const char* q = q0;
    while( q && ( q - q0 < msg_size ) ) {
      const char* eol = strchr( q, '\r' );
      if( !eol )
        break;

      const char* sep = strchr( q, ':' );
      if( !sep )
        break;
      sep += 2;

      if( strncmp( q, "DSCPORT: ", 9 ) == 0 ) 
        out_camera->port = atoi( sep );
      else if( strncmp( q, "DSC: ", 5 ) == 0 )
        strncpy( out_camera->ip, sep, eol - sep );
      else if( strncmp( q, "CAMERANAME: ", 12 ) == 0 )
        strncpy( out_camera->name, sep, eol - sep );
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
