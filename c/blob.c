#include <stdlib.h>     // malloc
#include <string.h>     // memcpy
#include <stdio.h>      // printf
#include <assert.h>
#include "blob.h"

void blob_create( blob_t* blob, uint32_t num_bytes, uint32_t bytes_to_reserve ) {
  assert( num_bytes <= bytes_to_reserve );
  if( bytes_to_reserve > 0 )
    blob->data = malloc( bytes_to_reserve );
  else
    blob->data = NULL;
  blob->count = num_bytes;
  blob->reserved = bytes_to_reserve;
}

void blob_reserve( blob_t* blob, uint32_t num_bytes ) {
  blob_resize( blob, num_bytes );
  blob_clear( blob );
}

void blob_resize( blob_t* blob, uint32_t new_size ) {
  if( new_size <= blob->reserved ) {
    blob->count = new_size;
    return;
  }
  if( !blob->reserved )
    blob->reserved = 16;
  while( blob->reserved < new_size )
    blob->reserved *= 2;
  blob->data = realloc( blob->data, blob->reserved );
  assert( blob->data );
  blob->count = new_size;
}

void blob_clear( blob_t* blob ) {
  blob->count = 0;
}

void blob_destroy( blob_t* blob ) {
  if( blob->data )
    free( blob->data );
  blob->data = NULL;
  blob->reserved = 0;
  blob->count = 0;
}

uint32_t blob_size( const blob_t *blob ) {
  return blob->count;
}

void blob_append_blob( blob_t* blob, const blob_t* other ) {
  blob_append_data( blob, other->data, other->count );
}

void blob_append_data( blob_t* blob, const void* new_data, uint32_t data_size ) {
  assert( blob_is_valid( blob ) );
  assert( new_data );
  int32_t prev_sz = blob->count;
  int32_t new_sz = blob->count + data_size;
  blob_resize( blob, new_sz );
  memcpy( blob->data + prev_sz, new_data, data_size );
}

void blob_remove_from_start( blob_t* blob, uint32_t num_bytes, blob_t* out_blob ) {
  assert( blob->count >= num_bytes );
  // I have 32. Remove 20 -> remain 12
  uint32_t remaining_bytes = blob->count - num_bytes;
  blob->count = remaining_bytes;
  if( out_blob ) {
    blob_resize( out_blob, num_bytes );
    memcpy( out_blob->data, blob->data, num_bytes );
  }
  if( remaining_bytes )
    memmove( blob->data, blob->data + num_bytes, remaining_bytes );
}

void blob_write_u16le( blob_t* blob, uint32_t offset, uint16_t value ) {
  assert( offset + 2 <= blob->count );
  uint8_t* p = blob->data + offset;
  *p++ = (value & 0x00FF) >>  0;
  *p++ = (value & 0xFF00) >>  8;
}

void blob_write_u32le( blob_t* blob, uint32_t offset, uint32_t value ) {
  assert( offset + 4 <= blob->count );
  uint8_t* p = blob->data + offset;
  *p++ = (value & 0x000000FF) >>  0;
  *p++ = (value & 0x0000FF00) >>  8;
  *p++ = (value & 0x00FF0000) >> 16;
  *p++ = (value & 0xFF000000) >> 24;
}

uint16_t blob_read_u16le( const blob_t* blob, uint32_t offset ) {
  assert( offset + 2 <= blob->count );
  uint8_t* p = blob->data + offset;
  return (p[0] << 0)
       | (p[1] << 8)
       ;
   }

uint32_t blob_read_u32le( const blob_t* blob, uint32_t offset ) {
  assert( offset + 4 <= blob->count );
  uint8_t* p = blob->data + offset;
  return (p[0] << 0)
       | (p[1] << 8)
       | (p[2] << 16)
       | (p[3] << 24)
       ;
}

bool blob_is_valid( const blob_t* blob ) {
  return blob && blob->data && blob->count < 16384 && blob->reserved >= blob->count;
}

void blob_dump( const blob_t* blob ) {
  printf( "%4d / %4d %p : ", blob->count, blob->reserved, blob->data );
  for( int i=0; i<blob->count; ++i ) {
    printf( "%02x ", blob->data[i] );
    // if( (i+1) % 16 == 0)
    //   printf( "\n");
  }
  printf( "\n");
}