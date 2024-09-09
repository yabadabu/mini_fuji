#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "blob.h" 
#include "connection.h" 

int blob_msg( blob_t* blob, int16_t id, const uint8_t* payload, size_t payload_size, const uint16_t* msg_type, const uint32_t* counter ) {

  // // msg_size(4) | num_part(2) | msg_type(2) | msg_idx(4) | payload( N )
  // function makeMsg( msg_id, payload = '', msg_type = msg_type_cmd, msg_counter = null) {
  //   if( !msg_counter ) 
  //     msg_counter = generateMsgCounter();
  //   const net_msg_type = swapInt16( msg_type )
  //   const net_msg_id = swapInt16( msg_id )
  //   const msg = Buffer.from( net_msg_type + net_msg_id + msg_counter + payload, 'hex')
  //   return prefixSize( msg )
  // }
  int size = 4 + 2 + 2 + 4 + payload_size;
  blob_resize( blob, size );
  blob_write_u32le( blob, 0, size );
  blob_write_u16le( blob, 4, *msg_type );
  blob_write_u16le( blob, 4, id );
  blob_write_u32le( blob, 4, *counter );

  return 0;
}

// -------------------
bool test_blobs();
bool test_conn();

int main( int argc, char** argv ) {
  if( !test_blobs() )
    return -1;
  if( !test_conn() )
    return -1;
  printf( "OK\n");
  return 0;
}

