#include "blob.h"
#include "connection.h"
#include "channel.h"
#include <assert.h>
#include <string.h>     // strlen
#include <stdio.h>      // printf

bool test_blobs() {
  printf( "Tessting blobs...\n");

  blob_t b0;
  blob_create( &b0, 6, 6 );
  bool is_valid = blob_is_valid( &b0 );
  assert( is_valid );
  blob_write_u32le( &b0, 0, 0x11223344 );
  blob_write_u16le( &b0, 4, 0x5566 );
  blob_dump( &b0 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[1] == 0x33 );
  assert( b0.data[2] == 0x22 );
  assert( b0.data[3] == 0x11 );
  assert( b0.data[4] == 0x66 );
  assert( b0.data[5] == 0x55 );

  blob_t b1;
  blob_create( &b1, 4, 4 );
  blob_write_u32le( &b1, 0, 0x88776655 );
  blob_dump( &b1 );
  uint32_t data_read = blob_read_u32le( &b1, 0 );
  assert( data_read == 0x88776655 );
  blob_append_blob( &b0, &b1 );
  blob_dump( &b0 );
  assert( b0.count == 6 + 4 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[6] == 0x55 ); 
  assert( blob_read_u16le( &b0, 6 ) == 0x6655 );

  blob_t b2;
  blob_create( &b2, 0, 0 );
  blob_remove_from_start( &b0, 2, &b2 );
  assert( blob_read_u16le( &b2, 0 ) == 0x3344 );
  blob_dump( &b2 );
  assert( b0.count == 8 );
  blob_dump( &b0 );
  assert( blob_read_u32le( &b0, 0 ) == 0x55661122 );

  blob_destroy( &b0 );
  blob_destroy( &b1 );
  assert( !blob_is_valid( &b0 ) );
  assert( !blob_is_valid( &b1 ) );

  blob_t bhex;
  blob_create( &bhex, 4, 32 );
  blob_write_u32le( &bhex, 0, 0x88664422 );
  printf( "Tessting blob 1\n");
  blob_from_hex_string( &bhex, 2, "A00a 33445566");
  blob_dump( &bhex );
  blob_from_hex_string( &bhex, -1, "33445566");
  assert( blob_read_u32le( &bhex, 0 ) == 0x0aA04422 );
  assert( blob_read_u32le( &bhex, 4 ) == 0x66554433 );
  assert( blob_read_u32le( &bhex, 8 ) == 0x66554433 );
  assert( blob_size( &bhex ) == 12 );
  blob_dump( &bhex );

  printf( "Tessting blobs OK\n");
  return true;
} 

void send_end_of_packet( conn_t* c ) {
  assert( conn_is_waiting_answer( c ) );
  blob_t eoc;
  blob_create( &eoc, 0, 256 );
  blob_from_hex_string( &eoc, 0, "0c000000 0300 1210 05000000" );
  conn_add_data( c, eoc.data, eoc.count );
  conn_update( c );
  assert( !conn_is_waiting_answer( c ) );
}

bool test_get_storage(conn_t* c) {
  printf( "test_get_storage\n" );
  conn_clear_state( c );
  // Send request
  storage_ids_t storage_ids;
  ptpip_get_storage_ids( c, &storage_ids );
  assert( conn_is_waiting_answer( c ) );

  // Fake network answer
  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "18000000 0200 0210 05000000 020000000100001002000010");
  blob_dump( &ans );
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );

  send_end_of_packet( c );

  assert( storage_ids.count == 2 );
  assert( storage_ids.ids[0].storage_id == 0x10000001 );
  assert( storage_ids.ids[1].storage_id == 0x10000002 );
  return true;
}

bool test_get_prop(conn_t* c) {
  printf( "test_get_prop\n" );
  conn_clear_state( c );
  prop_t p = prop_quality;
  ptpip_get_prop( c, &p );

  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "0e000000 0200 1510 05000000 0100");
  blob_dump( &ans );
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );

  send_end_of_packet( c );

  assert( c->recv_data.count == 0);
  assert( p.id == prop_quality.id );
  assert( p.val16 == 0x0001 );
  return true;
}

bool test_set_prop(conn_t* c) {
  printf( "test_set_prop\n" );
  conn_clear_state( c );
  prop_t p = prop_quality;
  p.val16 = 2;
  ptpip_set_prop( c, &p );
  send_end_of_packet( c );
  return true;
}

bool test_initiate_capture(conn_t* c) {
  printf( "test_initiate_capture\n" );
  conn_clear_state( c );
  ptpip_initiate_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_initiate_open_capture(conn_t* c) {
  printf( "test_initiate_open_capture\n" );
  conn_clear_state( c );
  ptpip_initiate_open_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_terminate_capture(conn_t* c) {
  printf( "test_terminate_capture\n" );
  conn_clear_state( c );
  ptpip_terminate_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_del_obj(conn_t* c) {
  printf( "test_del_obj\n" );
  conn_clear_state( c );
  handle_t h = { .value = 0x88224411 };
  ptpip_del_obj( c, h );
  send_end_of_packet( c );
  return true;
}

static void callback_progress( void* context, uint32_t curr, uint32_t required ) {
  printf( "On callback_progress %d/%d (Ctx:%p)\n", curr, required, context );
  assert( context );
  int* counter = (int*) context;
  *counter += 1;
}

bool test_get_obj(conn_t* c) {
  printf( "test_get_obj\n" );
  conn_clear_state( c );
  handle_t h = { .value = 0x88224411 };
  blob_t output;
  blob_create( &output, 0, 0 );

  int called = 0;
  callback_progress_t my_callback = { .context = &called, .callback = &callback_progress };
  ptpip_get_obj( c, h, &output, my_callback );

  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "14000000 0200 1510 05000000 01002233");
  blob_dump( &ans );
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );

  blob_t ans2;
  blob_create( &ans2, 0, 256 );
  blob_from_hex_string( &ans2, 0, "88998877");
  conn_add_data( c, ans2.data, ans2.count );

  conn_update( c );
  assert( output.count == 8 );
  blob_dump( &output );

  send_end_of_packet( c );

  assert( output.data[0] == 0x01 );
  assert( output.data[4] == 0x88 );
  assert( called == 3 );
  return true;
}

bool test_close_session( conn_t* c ) {
  printf( "ptpip_close_session\n");
  conn_clear_state( c );
  ptpip_close_session( c );
  return true;
}

bool test_open_session( conn_t* c ) {
  printf( "Test open session\n" );
  conn_clear_state( c );
  ptpip_open_session( c );
  return true;
}

bool test_initialize( conn_t* c ) {
  printf( "Test initialize\n" );
  conn_clear_state( c );
  ptpip_initialize( c );
  assert( conn_is_waiting_answer( c ) );

  const char* ans = "4400000002000000000000000870b0610a8b4593b2e79357dd36e05058002d00540032000000000000000000000000000000000000000000000000000000000000000000";
  blob_t bans;
  blob_create( &bans, 0, 256 );
  blob_from_hex_string( &bans, 0, ans );
  conn_add_data( c, bans.data, bans.count );
  conn_update( c );
  assert( !conn_is_waiting_answer( c ) );

  return true;
}

bool test_conn() {
  printf( "Testing connections...\n");
  conn_t conn;
  conn_t* c = &conn;
  conn_create( c );
  printf( "Conn created\n");
  conn_update( c );
  printf( "update complete\n");
  
  if( !test_initialize( c ))
    return false;
  
  if( !test_close_session( c ))
    return false;

  if( !test_open_session( c ))
    return false;

  if( !test_get_storage(c) )
    return false;

  if( !test_get_prop(c) )
    return false;

  if( !test_set_prop(c) )
    return false;

  if( !test_initiate_capture(c) )
    return false;

  if( !test_initiate_open_capture(c) )
    return false;

  if( !test_terminate_capture(c) )
    return false;

  if( !test_del_obj(c) )
    return false;

  if( !test_get_obj(c) )
    return false;

  return true;
}

bool test_channel_accept() {

  channel_t ch_server;
  if( !ch_create( &ch_server, "tcp_server:127.0.0.1:40000"))
    return false;

  while( true ) {
    channel_t ch_client;
    if( ch_accept( &ch_server, &ch_client, 5 * 1000000 ) ) {
      printf( "New connectiong accepted!\n" );
      break;
    }
  }

  ch_close( &ch_server );
  return true;
}


bool test_channels() {
  printf( "Testing channels...\n");

  blob_t buff;
  blob_create( &buff, 0, 1024 );

  channel_t ch_udp;
  if( !ch_create( &ch_udp, "udp:255.255.255.255:5002") )
    return false;

  const char* my_ip = "192.168.1.136";
  const char* prefix = "DISCOVERY * HTTP/1.1\r\nHOST: ";
  const char* suffix = "\r\nMX: 5\r\nSERVICE: PCSS/1.0\r\n";
  blob_t discovery_msg;
  blob_create( &discovery_msg, 0, 512 );
  blob_append_data( &discovery_msg, prefix, strlen( prefix ) );
  blob_append_data( &discovery_msg, my_ip, strlen( my_ip ) );
  blob_append_data( &discovery_msg, suffix, strlen( suffix ) );

  channel_t ch_discovery;
  if( !ch_create( &ch_discovery, "tcp_server:0.0.0.0:51560") )
    return false;
  assert( ch_discovery.port = 51560 );
  printf( "TCP.Server listening at port %d\n", ch_discovery.port );

  channel_t ch_client;
  while( true ) {
    int brc = ch_broadcast( &ch_udp, discovery_msg.data, discovery_msg.count );
    printf( "Broadcasted msg\n" );

    if( ch_accept( &ch_discovery, &ch_client, 5000000 ) ) {
      printf( "New connectiong accepted!\n" );
      while( true ) {
        int bytes_read = ch_read( &ch_client, buff.data, buff.reserved );
        if( bytes_read > 0 ) {
          buff.count = bytes_read;
          buff.data[ buff.count ] = 0x00;
          printf( "Recv:\n%s\n", (char*)buff.data );
          blob_dump( &buff );
          break;
        }
      }
      break;
    }
  }

  ch_close( &ch_udp );
  ch_close( &ch_discovery );
  ch_close( &ch_client );

return false;

  channel_t ch;
  channel_t* c = &ch;
  if( !ch_create( c, "tcp:127.0.0.1:5001") )
    return false;
  int bytes_written = ch_write( c, "JOHN", 4);
  assert( bytes_written == 4 );

  while( true ) {
    int bytes_read = ch_read( c, buff.data, buff.reserved );
    if( bytes_read > 0 ) {
      buff.count = bytes_read;
      blob_dump( &buff );
      break;
    }
  }

  ch_close( c );
  printf( "Testing channels OK\n");
return false;
  return true;
}