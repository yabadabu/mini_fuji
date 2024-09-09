#include <assert.h>
#include "connection.h"

// Local functions
bool conn_has_packet_ready( conn_t* );

void conn_add_data( conn_t* conn, const void* new_data, uint32_t data_size ) {
  blob_append_data( &conn->recv_data, new_data, data_size );
}

bool conn_has_packet_ready( conn_t* conn ) {
  uint32_t bytes_in_buffer = blob_size( &conn->recv_data );
  if( bytes_in_buffer <= 4 )
    return false;
  uint32_t required_bytes = blob_read_u32le( &conn->recv_data, 0 );
  return bytes_in_buffer >= required_bytes;
}

uint32_t conn_next_packet_size( conn_t* conn ) {
  if( blob_size( &conn->recv_data ) >= 4 )
    return blob_read_u32le( &conn->recv_data, 0 );
  return 0;
}

void conn_dispatch( conn_t* conn, const blob_t* packet ) {

}

void conn_update( conn_t* conn ) {

  while( conn_has_packet_ready( conn ) ) {
    uint32_t required_bytes = conn_next_packet_size( conn );
    assert( required_bytes > 0 );

    blob_t packet;
    blob_create( &packet, 0 );
    blob_remove_from_start( &conn->recv_data, required_bytes, &packet );

    conn_dispatch( conn, &packet );
  }
  
}

