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


bool test_conn() {
  printf( "Tessting connections...\n");
  conn_t conn;
  conn_create( &conn );
  conn_update( &conn );
  
  ptpip_open_session( &conn );
  prop_t p = { .id = 0x5005 };
  ptpip_get_prop( &conn, &p );

  // blob_t msg_open_session;
  // conn_create_cmd_msg( &conn, &msg_open_session, &cmd_open_session );
  // blob_dump( &msg_open_session );
  return true;
}