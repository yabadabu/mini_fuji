#include <assert.h>
#include <stdio.h>
#include "connection.h"

// Local functions
bool conn_has_packet_ready( conn_t*, uint32_t* packet_size );
uint32_t conn_next_sequence( conn_t* );
void conn_send( conn_t*, const blob_t* blob );

bool conn_create( conn_t* conn ) {
  blob_create( &conn->recv_data, 0 );
  blob_reserve( &conn->recv_data, 256 );
  blob_create( &conn->curr_packet, 0 );
  blob_reserve( &conn->curr_packet, 256 );
  conn->sequence_id = 1;
  return true;
}

void conn_destroy( conn_t* conn ) {
  blob_destroy( &conn->recv_data );
  blob_destroy( &conn->curr_packet );
}

uint32_t conn_next_msg_sequence( conn_t* conn) {
  conn->sequence_id += 1;
  return conn->sequence_id - 1;
}

void conn_add_data( conn_t* conn, const void* new_data, uint32_t data_size ) {
  printf( "Recv    %4d bytes : ", data_size );
  blob_append_data( &conn->recv_data, new_data, data_size );
}

void conn_send( conn_t*, const blob_t* data ) {
  printf( "Sending %4d bytes : ", blob_size( data ) );
  blob_dump( data );
}

bool conn_has_packet_ready( conn_t* conn, uint32_t* packet_size ) {
  uint32_t bytes_in_buffer = blob_size( &conn->recv_data );
  if( bytes_in_buffer <= 4 )
    return false;
  *packet_size = blob_read_u32le( &conn->recv_data, 0 );
  return bytes_in_buffer >= *packet_size;
}

uint32_t conn_next_packet_size( conn_t* conn ) {
  if( blob_size( &conn->recv_data ) >= 4 )
    return blob_read_u32le( &conn->recv_data, 0 );
  return 0;
}

void conn_dispatch( conn_t* conn, const blob_t* packet ) {

  // 
}

void conn_update( conn_t* conn ) {

  uint32_t required_bytes = 0;
  while( conn_has_packet_ready( conn, &required_bytes ) ) {
    blob_remove_from_start( &conn->recv_data, required_bytes, &conn->curr_packet );
    conn_dispatch( conn, &conn->curr_packet );
  }
  
}

cmd_t cmd_open_session = { .id = 0x1015, .name = "open_session" };

void conn_create_cmd_msg( conn_t* conn, blob_t* msg, const cmd_t* cmd ) {
  uint16_t msg_type = 0x0001;
  uint32_t msg_seq_id = conn_next_msg_sequence( conn );
  uint32_t msg_full_size = 4 + 2 + 2 + 4; // + payload
  blob_resize( msg, msg_full_size );
  blob_write_u32le( msg, 0, msg_full_size );
  blob_write_u16le( msg, 4, msg_type );
  blob_write_u16le( msg, 6, cmd->id );
  blob_write_u32le( msg, 8, msg_seq_id );
}

int ptpip_open_session( conn_t* conn ) {
  blob_t msg;
  conn_create_cmd_msg( conn, &msg, &cmd_open_session );
  conn_send( conn, &msg );
}