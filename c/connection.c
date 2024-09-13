#include <stdio.h>
#include <assert.h>
#include "connection.h"

prop_t prop_quality = { .id = 0xd018, .name = "Quality", .data_type = PDT_U16 };

// ------------------------------------------------------
extern cmd_t cmd_open_session;
extern cmd_t cmd_close_session;
extern cmd_t cmd_initiate_capture;
extern cmd_t cmd_initiate_open_capture;
extern cmd_t cmd_terminate_capture;
extern cmd_t cmd_del_obj;
extern cmd_t cmd_get_obj;
extern cmd_t cmd_get_prop;
extern cmd_t cmd_set_prop;
extern cmd_t cmd_get_storage_ids;
extern cmd_t cmd_get_obj_handles;
extern cmd_t cmd_initialize_comm;

// ------------------------------------------------------
const uint16_t msg_type_cmd  = 0x0001;
const uint16_t msg_type_data = 0x0002;
const uint16_t msg_type_end  = 0x0003;
const uint16_t msg_type_init = 0x0005;

const uint32_t offset_payload = 12;

bool ch_read_blob( channel_t* ch, blob_t* blob, int usecs );

void callback_clear( callback_progress_t* cb ) {
  cb->context = NULL;
  cb->callback = NULL;
}

// ------------------------------------------------------
// Local functions
bool     conn_has_packet_ready( conn_t*, uint32_t* packet_size );
uint32_t conn_next_sequence( conn_t* );
void     conn_send( conn_t*, const blob_t* blob );

bool conn_create( conn_t* conn ) {

  blob_create( &conn->recv_data, 0, 256 );
  blob_create( &conn->curr_packet, 0, 256 );
  blob_create( &conn->last_answer, 0, 256 );
  blob_create( &conn->otf_msg, 0, 256 );
  blob_create( &conn->net_buffer, 0, 32768 );

  conn->sequence_id = 1;
  conn_clear_state( conn );

  callback_clear( &conn->on_progress );
  return true;
}

void conn_destroy( conn_t* conn ) {
  conn_clear_state( conn );
  blob_destroy( &conn->net_buffer );
  blob_destroy( &conn->otf_msg );
  blob_destroy( &conn->last_answer );
  blob_destroy( &conn->recv_data );
  blob_destroy( &conn->curr_packet );
  ch_close( &conn->channel );
}

void conn_clear_state( conn_t* conn ) {
  conn->curr_output = NULL;
  conn->curr_cmd = NULL;
  blob_clear( &conn->otf_msg );
  callback_clear( &conn->on_progress );
}

uint32_t conn_next_msg_sequence( conn_t* conn) {
  conn->sequence_id += 1;
  return conn->sequence_id - 1;
}

int  conn_get_last_ptpip_return_code( conn_t* conn ) {
  return conn->last_cmd_result;
}

void conn_recv( conn_t* conn, const blob_t* new_data ) {
  printf( "Recv %4d bytes\n", blob_size( new_data ) );
  blob_append_blob( &conn->recv_data, new_data );
  
  // Report to the progress callback
  if( conn->on_progress.callback ) {
    uint32_t required_bytes = 0;
    conn_has_packet_ready( conn, &required_bytes );
    uint32_t bytes_in_buffer = blob_size( &conn->recv_data );
    if( required_bytes > 0 && bytes_in_buffer > 0 )
      (*conn->on_progress.callback)( conn->on_progress.context, bytes_in_buffer, required_bytes );
  }
}

void conn_send( conn_t* conn, const blob_t* blob ) {
  printf( "Send " );
  blob_dump( blob );
  ch_write( &conn->channel, blob->data, blob_size( blob ) );
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

bool conn_is_waiting_answer( conn_t* conn ) {
  assert( conn );
  return conn->curr_cmd != NULL;
}

void conn_dispatch( conn_t* conn, const blob_t* msg ) {
  printf( "Processing packet");
  blob_dump( msg );
  uint32_t packet_size = blob_read_u32le( msg, 0 );
  uint16_t msg_type = blob_read_u16le( msg, 4 );
  uint16_t cmd_id = blob_read_u16le( msg, 6 );
  uint16_t seq_id = blob_read_u32le( msg, 8 );
  printf( "%d bytes %04x %04x %08x CurrCmd:%s\n", packet_size, msg_type, cmd_id, seq_id, conn->curr_cmd ? conn->curr_cmd->name : "None");

  if( conn->curr_cmd ) {
    // expect seq_id == otf_msg->seq_id
    assert( msg_type == msg_type_data || msg_type == msg_type_end );

    // Send the extra bytes to the current cmd to parse
    // Sometimes there is data as part of the msg_type_end msg type
    blob_t args;
    args.data = msg->data + offset_payload;
    args.count = packet_size - offset_payload;
    args.reserved = 0;
    if( args.count > 0 && conn->curr_cmd->parse )
      conn->curr_cmd->parse( &args, conn->curr_output );

    // The command is over?
    if( msg_type == msg_type_end ) {
      conn_clear_state( conn );
      conn->last_cmd_result = cmd_id;
    }

    // We will not receive a 'end' for the initialization command
    else if( msg_type == msg_type_data && cmd_id == cmd_initialize_comm.id )
      conn_clear_state( conn );

  } 

}

int conn_transaction( conn_t* conn, const blob_t* data, cmd_t* cmd, void* output_data) {
  
  // Previous transaction should have been finished
  assert( conn->curr_cmd == NULL );

  conn_send( conn, data );
  conn->curr_output = output_data;
  conn->curr_cmd = cmd;

  // Wait answer
  return 0;
}

void conn_update( conn_t* conn, int usecs ) {

  if( ch_read_blob( &conn->channel, &conn->net_buffer, usecs ) )
    conn_recv( conn, &conn->net_buffer );

  uint32_t required_bytes = 0;
  while( conn_has_packet_ready( conn, &required_bytes ) ) {
    blob_remove_from_start( &conn->recv_data, required_bytes, &conn->curr_packet );
    conn_dispatch( conn, &conn->curr_packet );
  }
  
}

// -------------------------------------------------------------
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
  blob_write_u32le( msg, offset_payload, payload_int );
}

int ptpip_basic_cmd( conn_t* conn, cmd_t* cmd ) {
  blob_t* msg = &conn->otf_msg;
  conn_create_cmd_msg( conn, msg, cmd );
  conn_transaction( conn, msg, cmd, NULL );
  return 0;
}

int ptpip_basic_cmd_u32( conn_t* conn, cmd_t* cmd, uint32_t payload_int, void* output ) {
  blob_t* msg = &conn->otf_msg;
  conn_create_cmd_msg_u32( conn, msg, cmd, payload_int );
  conn_transaction( conn, msg, cmd, output );
  return 0;
}

// -------------------------------------------------------------
int ptpip_initialize( conn_t* conn ) {
  // 52000000 Size of the msg
  // 01000000 Init Cmd Request Packet
  // f2e4538f protcol verson
  // ada5485d87b27f0bd3d5ded0  Client GUID
  // < Host Name as string >
  // < Version as 4 bytes >
  
  blob_t* msg = &conn->otf_msg;
  cmd_t* cmd = &cmd_initialize_comm;
  uint32_t protocol_version = 0x8f53e4f2;
  create_cmd_msg( msg, cmd, msg_type_cmd, protocol_version, 0x46 );
  blob_from_hex_string( msg, 12, "ada5485d 87b27f0b d3d5ded0 fe7fa8c0 4a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000" );
  // we send: 52000000 0100 0000 f2e4538f ada5485d 87b27f0b d3d5ded0 fe7fa8c0 4a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000
  // we recv: 4400000002000000000000000870b0610a8b4593b2e79357dd36e05058002d00540032000000000000000000000000000000000000000000000000000000000000000000
  
  conn_transaction( conn, msg, cmd, NULL );

  return 0;
}

int ptpip_open_session( conn_t* conn ) {
  return ptpip_basic_cmd( conn, &cmd_open_session );
}

int ptpip_close_session( conn_t* conn ) {
  return ptpip_basic_cmd( conn, &cmd_close_session );
}

int ptpip_set_prop( conn_t* conn, prop_t* prop ) {
  assert( prop && conn );
  cmd_t* cmd = &cmd_set_prop;
  blob_t* msg = &conn->otf_msg;
  uint32_t msg_seq_id = conn_next_msg_sequence( conn );
  
  create_cmd_msg( msg, cmd, msg_type_cmd, msg_seq_id, 4 );
  blob_write_u32le( msg, offset_payload, (prop->id & 0xffff) );
  conn_send( conn, msg );

  create_cmd_msg( msg, cmd, msg_type_data, msg_seq_id, 2 );
  blob_write_u16le( msg, offset_payload, prop->val16 );

  conn_transaction( conn, msg, cmd, prop );
  return 0;
}

int ptpip_get_prop( conn_t* conn, prop_t* prop ) {
  return ptpip_basic_cmd_u32( conn, &cmd_get_prop, prop->id, prop );
}

int ptpip_get_storage_ids( conn_t* conn, storage_ids_t* storage_ids ) {
  assert( storage_ids && conn );
  blob_t* msg = &conn->otf_msg;
  conn_create_cmd_msg( conn, msg, &cmd_get_storage_ids );
  conn_transaction( conn, msg, &cmd_get_storage_ids, storage_ids );
  return 0;
}

int ptpip_initiate_capture( conn_t* conn ) {
  return ptpip_basic_cmd( conn, &cmd_initiate_capture );
}

int ptpip_initiate_open_capture( conn_t* conn ) {
  return ptpip_basic_cmd( conn, &cmd_initiate_open_capture );
}

int ptpip_terminate_capture( conn_t* conn ) {
  return ptpip_basic_cmd( conn, &cmd_terminate_capture );
}

int ptpip_del_obj( conn_t* conn, handle_t handle ) {
  // No output parameter expected
  return ptpip_basic_cmd_u32( conn, &cmd_del_obj, handle.value, NULL );
}

int ptpip_get_obj_handles( conn_t* conn, storage_id_t storage_id, handles_t* out_handles) {
  assert( out_handles && conn );
  blob_t* msg = &conn->otf_msg;
  uint32_t msg_seq_id = conn_next_msg_sequence( conn );
  create_cmd_msg( msg, &cmd_get_obj_handles, msg_type_cmd, msg_seq_id, 12 );
  blob_write_u32le( msg, offset_payload + 0, storage_id.id );
  blob_write_u32le( msg, offset_payload + 4, 0 );
  blob_write_u32le( msg, offset_payload + 8, 0xffffffff );
  conn_transaction( conn, msg, &cmd_get_obj_handles, out_handles );
  return 0;
}

int ptpip_get_obj( conn_t* conn, handle_t handle, blob_t* out_obj, callback_progress_t on_progress ) {
  assert( out_obj && conn && handle.value );
  conn->on_progress = on_progress;
  int rc = ptpip_basic_cmd_u32( conn, &cmd_get_obj, handle.value, out_obj );
  return rc;
}

const char* ptpip_error_msg( int rc ) {
  switch( rc ) {
  case 0x2000: return "Undefined";
  case 0x2001: return "OK";
  case 0x2002: return "GeneralError";
  case 0x2003: return "SessionNotOpen";
  case 0x2004: return "InvalidTransactionID";
  case 0x2005: return "OperationNotSupported";
  case 0x2006: return "ParameterNotSupported";
  case 0x2007: return "IncompleteTransfer";
  case 0x2008: return "InvalidStorageId";
  case 0x2009: return "InvalidObjectHandle";
  case 0x200a: return "DevicePropNotSupported";
  case 0x200b: return "InvalidObjectFormatCode";
  case 0x2010: return "NoThumbnailPresent";
  case 0x2015: return "NoValidObjectInfo";
  case 0x2018: return "CaptureAlreadyTerminated";
  case 0x2019: return "DeviceBusy";
  case 0x201b: return "InvalidDevicePropFormat";
  case 0x201c: return "InvalidDevicePropValue";
  case 0x201d: return "InvalidParameter";
  case 0x201e: return "SessionAlreadyOpened";
  }
  return "Unknown return code";
}

