#include "blob.h"
#include <assert.h>
#include <stdio.h>      // printf

bool run_tests() {

  blob_t b0;
  blob_create( &b0, 6 );
  bool is_valid = blob_is_valid( &b0 );
  assert( is_valid );
  blob_write_u32( &b0, 0, 0x11223344 );
  blob_write_u16( &b0, 4, 0x5566 );
  blob_dump( &b0 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[1] == 0x33 );
  assert( b0.data[2] == 0x22 );
  assert( b0.data[3] == 0x11 );
  assert( b0.data[4] == 0x66 );
  assert( b0.data[5] == 0x55 );

  blob_t b1;
  blob_create( &b1, 4 );
  blob_write_u32( &b1, 0, 0x88776655 );
  blob_read_u32( &b1, 0 ) == 0x88776655;
  blob_append( &b0, &b1 );
  blob_dump( &b0 );
  assert( b0.count == 6 + 4 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[6] == 0x55 );
  blob_read_u16( &b0, 6 ) == 0x6655;

  blob_destroy( &b0 );

  return true;
} 