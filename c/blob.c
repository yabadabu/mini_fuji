#include <stdlib.h>     // malloc
#include <string.h>     // memcpy
#include <stdio.h>      // printf
#include <assert.h>
#include "blob.h"

typedef struct { int32_t value; } handle_t;
typedef struct { int32_t value; } storage_id_t;

typedef struct { 
  int32_t  count;
  handle_t data[];
} array_handles_t;

typedef struct {
  // ...
} connection_t;

int blob_create( blob_t* blob, uint32_t num_bytes ) {
  blob->data = malloc( num_bytes );
  blob->count = num_bytes;
  blob->reserved = num_bytes;
  return 0;
}

int blob_resize( blob_t* blob, uint32_t new_size ) {
  if( new_size <= blob->reserved ) {
    blob->count = new_size;
    return 0;
  }
  while( blob->reserved < new_size )
    blob->reserved *= 2;
  blob->data = realloc( blob->data, blob->reserved );
  blob->count = new_size;
  return 0;
}

int blob_clear( blob_t* blob ) {
  blob->count = 0;
  return 0;
}

int blob_destroy( blob_t* blob ) {
  if( blob->data )
    free( blob->data );
  blob->data = NULL;
  blob->reserved = 0;
  blob->count = 0;
  return 0;
}

void blob_append( blob_t* blob, const blob_t* other ) {
  int32_t prev_sz = blob->count;
  int32_t new_sz = blob->count + other->count;
  blob_resize( blob, new_sz );
  memcpy( blob->data + prev_sz, other->data, other->count );
}

void blob_write_u16( blob_t* blob, uint32_t offset, uint16_t value ) {
  assert( offset + 2 <= blob->count );
  uint8_t* p = blob->data + offset;
  *p++ = (value & 0x00FF) >>  0;
  *p++ = (value & 0xFF00) >>  8;
}

void blob_write_u32( blob_t* blob, uint32_t offset, uint32_t value ) {
  assert( offset + 4 <= blob->count );
  uint8_t* p = blob->data + offset;
  *p++ = (value & 0x000000FF) >>  0;
  *p++ = (value & 0x0000FF00) >>  8;
  *p++ = (value & 0x00FF0000) >> 16;
  *p++ = (value & 0xFF000000) >> 24;
}

uint16_t blob_read_u16( blob_t* blob, uint32_t offset ) {
  assert( offset + 2 <= blob->count );
  uint8_t* p = blob->data + offset;
  return (p[0] << 0)
     ||  (p[1] << 8)
     ;
}

uint32_t blob_read_u32( blob_t* blob, uint32_t offset ) {
  assert( offset + 4 <= blob->count );
  uint8_t* p = blob->data + offset;
  return (p[0] << 0)
     ||  (p[1] << 8)
     ||  (p[2] << 16)
     ||  (p[3] << 24)
     ;
}

bool blob_is_valid( const blob_t* blob ) {
  return blob && blob->data;
}

void blob_dump( const blob_t* blob ) {
  assert( blob_is_valid( blob ) );
  for( int i=0; i<blob->count; ++i ) {
    printf( "%02x ", blob->data[i] );
    if( (i+1) % 16 == 0)
      printf( "\n");
  }
  printf( "\n");
}