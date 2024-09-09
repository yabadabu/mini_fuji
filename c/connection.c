#include <assert.h>
#include <stdio.h>
#include "connection.h"

cmd_t cmd_open_session    = { .id = 0x1002, .name = "open_session" };
cmd_t cmd_close_session   = { .id = 0x1003, .name = "close_session" };
cmd_t cmd_get_prop        = { .id = 0x1014, .name = "get_prop" };

int parse_get_storage_ids( const blob_t* args, void* output ) {
  storage_ids_t* out_ids = (storage_ids_t*) output;
  out_ids->count = blob_read_u32le( args, 0 );
  if( out_ids->count > 3 )
    out_ids->count = 3;
  for( uint32_t i=0; i<out_ids->count; ++i ) {
    uint32_t id = blob_read_u32le( args, 4 + i * 4 );
    out_ids->ids[i].storage_id = id;
  }
  return 0; 
}

cmd_t cmd_get_storage_ids = { .id = 0x1004, .name = "get_storage_ids", .parse = &parse_get_storage_ids };

const uint16_t msg_type_cmd  = 0x0001;
const uint16_t msg_type_data = 0x0002;
const uint16_t msg_type_end  = 0x0003;

// Local functions
bool conn_has_packet_ready( conn_t*, uint32_t* packet_size );
uint32_t conn_next_sequence( conn_t* );
void conn_send( conn_t*, const blob_t* blob );

bool conn_create( conn_t* conn ) {
  blob_create( &conn->recv_data, 0 );
  blob_reserve( &conn->recv_data, 256 );
  blob_dump( &conn->recv_data );
  blob_create( &conn->curr_packet, 0 );
  blob_reserve( &conn->curr_packet, 256 );
  blob_create( &conn->last_answer, 0 );
  blob_reserve( &conn->last_answer, 256 );
  conn->sequence_id = 1;
  conn->curr_output = NULL;
  conn->curr_cmd = NULL;
  return true;
}

void conn_destroy( conn_t* conn ) {
  blob_destroy( &conn->last_answer );
  blob_destroy( &conn->recv_data );
  blob_destroy( &conn->curr_packet );
}

uint32_t conn_next_msg_sequence( conn_t* conn) {
  conn->sequence_id += 1;
  return conn->sequence_id - 1;
}

void conn_add_data( conn_t* conn, const void* new_data, uint32_t data_size ) {
  printf( "Recv    %4d bytes\n", data_size );
  blob_append_data( &conn->recv_data, new_data, data_size );
}

void conn_send( conn_t* conn, const blob_t* data ) {
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

void conn_dispatch( conn_t* conn, const blob_t* msg ) {
  printf( "Processing packet");
  blob_dump( msg );
  uint32_t packet_size = blob_read_u32le( msg, 0 );
  uint16_t msg_type = blob_read_u16le( msg, 4 );
  uint16_t cmd_id = blob_read_u16le( msg, 6 );
  uint16_t seq_id = blob_read_u32le( msg, 8 );
  printf( "%d bytes %04x %04x %08x\n", packet_size, msg_type, cmd_id, seq_id);
  if( conn->curr_cmd ) {
    blob_t args;
    args.data = msg->data + 12;
    args.count = packet_size - 12;
    args.reserved = 0;
    conn->curr_cmd->parse( &args, conn->curr_output );
  } 

}

int conn_transaction( conn_t* conn, const blob_t* data, cmd_t* cmd, void* output_data) {
  conn_send( conn, data );
  conn->curr_output = output_data;
  conn->curr_cmd = cmd;

  // Wait answer
  return 0;
}

void conn_update( conn_t* conn ) {

  uint32_t required_bytes = 0;
  while( conn_has_packet_ready( conn, &required_bytes ) ) {
    blob_remove_from_start( &conn->recv_data, required_bytes, &conn->curr_packet );
    conn_dispatch( conn, &conn->curr_packet );
  }
  
}

void create_cmd_msg(  blob_t* msg, const cmd_t* cmd, uint16_t msg_type, uint32_t msg_seq_id, uint32_t payload_size ) {
  uint32_t msg_full_size = 4 + 2 + 2 + 4 + payload_size;
  blob_resize( msg, msg_full_size );
  blob_write_u32le( msg, 0, msg_full_size );
  blob_write_u16le( msg, 4, msg_type );
  blob_write_u16le( msg, 6, cmd->id );
  blob_write_u32le( msg, 8, msg_seq_id );
}

// -------------------------------------------------------------
void conn_create_cmd_msg( conn_t* conn, blob_t* msg, const cmd_t* cmd ) {
  uint32_t msg_seq_id = conn_next_msg_sequence( conn );
  create_cmd_msg( msg, cmd, msg_type_cmd, msg_seq_id, 0 );
}

void conn_create_cmd_msg_u32( conn_t* conn, blob_t* msg, const cmd_t* cmd, uint32_t payload_int ) {
  uint32_t msg_seq_id = conn_next_msg_sequence( conn );
  create_cmd_msg( msg, cmd, msg_type_cmd, msg_seq_id, 4 );
  blob_write_u32le( msg, 12, payload_int );
}

// -------------------------------------------------------------
int ptpip_open_session( conn_t* conn ) {
  blob_t msg;
  blob_create( &msg, 0 );
  conn_create_cmd_msg( conn, &msg, &cmd_open_session );
  conn_transaction( conn, &msg, &cmd_open_session, NULL );
  return 0;
}

int ptpip_close_session( conn_t* conn ) {
  blob_t msg;
  blob_create( &msg, 0 );
  conn_create_cmd_msg( conn, &msg, &cmd_close_session );
  conn_transaction( conn, &msg, &cmd_close_session, NULL );
  return 0;
}

int ptpip_get_prop( conn_t* conn, prop_t* prop ) {
  blob_t msg;
  blob_create( &msg, 0 );
  conn_create_cmd_msg_u32( conn, &msg, &cmd_get_prop, prop->id );
  conn_transaction( conn, &msg, &cmd_get_prop, prop );
  return 0;
}

int ptpip_get_storage_ids( conn_t* conn, storage_ids_t* storage_ids ) {
  blob_t msg;
  blob_create( &msg, 0 );
  conn_create_cmd_msg( conn, &msg, &cmd_get_storage_ids );
  conn_transaction( conn, &msg, &cmd_get_storage_ids, storage_ids );
  conn_send( conn, &msg );
  return 0;
}
