#include "blob.h"
#include "connection.h"
#include <assert.h>
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

  return true;
} 

void blob_from( blob_t* b, const char* hex_data ) {
  const char* p = hex_data;

  // Count characters
  int n = 0;
  while( *p ) {
    if( *p != ' ')
      n++;
    ++p;
  }
  assert( ( n & 1 ) == 0 );
  int nbytes = n / 2;
  //printf( "%d bytes\n", nbytes );
  blob_create( b, nbytes, nbytes );

  p = hex_data;
  uint8_t* op = b->data;
  n = 0;
  uint8_t v = 0;
  while( *p ) {
    if( *p != ' ') {
      int hv = *p - '0';
      if( *p >= 'a' && *p <= 'f' )
        hv = 10 + (*p - 'a');
      assert( !( *p >= 'A' && *p <= 'F' ) );
      if( n & 1 ) {
        v |= hv;
        *op++ = v;
        printf( "%02x ", v );
        v = 0;
      } else {
        v |= hv << 4;
      }
      n++;
    }
    ++p;
  }
  printf( "\n" );

} 

bool test_get_storage(conn_t* c) {
  printf( "test_get_storage\n" );
  // Send request
  storage_ids_t storage_ids;
  ptpip_get_storage_ids( c, &storage_ids );
  // Fake network answer
  blob_t ans;
  blob_from( &ans, "18000000 0200 0410 05000000 020000000100001002000010");
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );
  assert( storage_ids.count == 2 );
  assert( storage_ids.ids[0].storage_id == 0x10000001 );
  assert( storage_ids.ids[1].storage_id == 0x10000002 );
  return true;
}

bool test_get_prop(conn_t* c) {
  printf( "test_get_prop\n" );
  prop_t p = prop_quality;
  ptpip_get_prop( c, &p );

  blob_t ans;
  blob_from( &ans, "0e000000 0200 1510 05000000 0100");
  blob_dump( &ans );
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );
  assert( c->recv_data.count == 0);
  assert( p.id == prop_quality.id );
  assert( p.val16 == 0x0001 );
  return true;
}

bool test_set_prop(conn_t* c) {
  printf( "test_set_prop\n" );
  prop_t p = prop_quality;
  p.val16 = 2;
  ptpip_set_prop( c, &p );
  return true;
}

bool test_initiate_capture(conn_t* c) {
  printf( "test_initiate_capture\n" );
  ptpip_initiate_capture( c );
  return true;
}

bool test_initiate_open_capture(conn_t* c) {
  printf( "test_initiate_open_capture\n" );
  ptpip_initiate_open_capture( c );
  return true;
}

bool test_terminate_capture(conn_t* c) {
  printf( "test_terminate_capture\n" );
  ptpip_terminate_capture( c );
  return true;
}

bool test_del_obj(conn_t* c) {
  handle_t h = { .value = 0x88224411 };
  printf( "test_del_obj\n" );
  ptpip_del_obj( c, h );
  return true;
}

static void callback_progress( void* context, float progress ) {
  printf( "On callback_progress %f (Ctx:%p)\n", progress, context );
  assert( context );
  int* counter = (int*) context;
  *counter += 1;
}

bool test_get_obj(conn_t* c) {
  printf( "test_get_obj\n" );
  handle_t h = { .value = 0x88224411 };
  blob_t output;
  blob_create( &output, 0, 0 );

  int called = 0;
  callback_progress_t my_callback = { .context = &called, .callback = &callback_progress };
  ptpip_get_obj( c, h, &output, my_callback );

  blob_t ans;
  blob_from( &ans, "14000000 0200 1510 05000000 01002233");
  blob_dump( &ans );
  conn_add_data( c, ans.data, ans.count );
  conn_update( c );

  blob_t ans2;
  blob_from( &ans2, "88998877");
  conn_add_data( c, ans2.data, ans2.count );

  conn_update( c );
  assert( output.count == 8 );
  blob_dump( &output );
  assert( output.data[0] == 0x01 );
  assert( output.data[4] == 0x88 );
  assert( called == 2 );
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
  
  ptpip_close_session( c );
  printf( "ptpip_close_session\n");
  ptpip_open_session( c );
  printf( "Test open session\n" );

  printf( "basic msgs complete\n");
  
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

  // blob_t msg_open_session;
  // conn_create_cmd_msg( &conn, &msg_open_session, &cmd_open_session );
  // blob_dump( &msg_open_session );
  return true;
}