#include "blob.h"
#include "connection.h"
#include <assert.h>
#include <stdio.h>      // printf

bool test_blobs() {
  printf( "Tessting blobs...\n");

  blob_t b0;
  blob_create( &b0, 6 );
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
  blob_create( &b1, 4 );
  blob_write_u32le( &b1, 0, 0x88776655 );
  blob_dump( &b1 );
  uint32_t data_read = blob_read_u32le( &b1, 0 );
  assert( data_read == 0x88776655 );
  blob_append_blob( &b0, &b1 );
  blob_dump( &b0 );
  assert( b0.count == 6 + 4 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[6] == 0x55 ); 
  blob_read_u16le( &b0, 6 ) == 0x6655;

  blob_t b2;
  blob_create( &b2, 0 );
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

extern cmd_t cmd_get_storage_ids;

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
  blob_create( b, nbytes );

  p = hex_data;
  char* op = b->data;
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
  prop_t p = { .id = 0x5005 };
  ptpip_get_prop( c, &p );
  printf( "basic msgs complete\n");
  
  // Send request
  storage_ids_t storage_ids;
  ptpip_get_storage_ids( c, &storage_ids );
  // Fake network answer
  blob_t storage_ids_ans;
  blob_from( &storage_ids_ans, "18000000 0200 0410 05000000 020000000100001002000010");
  conn_add_data( c, storage_ids_ans.data, storage_ids_ans.count );
  conn_update( c );
  assert( storage_ids.count == 2 );
  assert( storage_ids.ids[0].storage_id == 0x10000001 );
  assert( storage_ids.ids[1].storage_id == 0x10000002 );

  // blob_t msg_open_session;
  // conn_create_cmd_msg( &conn, &msg_open_session, &cmd_open_session );
  // blob_dump( &msg_open_session );
  return true;
}