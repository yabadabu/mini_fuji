#include <stdlib.h>     // malloc
#include <string.h>     // memcpy
#include <stdio.h>      // FILE
#include <assert.h>
#include "dbg.h"
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

void blob_from_hex_string( blob_t* blob, uint32_t offset, const char* hex_str ) {
  assert( blob );
  if( offset == 0xffffffff)
    offset = blob->count;
  assert( offset <= blob->count );
  const char* ip = hex_str;
  int num_chars = 0;
  uint8_t new_byte = 0;
  while( *ip ) {

    if( *ip == ' ' ) {
      ++ip;
      continue;
    } 
    uint8_t nibble = 0;
    if( *ip >= '0' && *ip <= '9' )
      nibble = *ip - '0';
    else if( *ip >= 'A' && *ip <= 'F' )
      nibble = *ip - 'A' + 10;
    else if( *ip >= 'a' && *ip <= 'f' )
      nibble = *ip - 'a' + 10;
    else {
      assert( "Invalid character found in hex string" );
    }

    if( num_chars & 1 ) {
      new_byte |= nibble;
      assert( offset < blob->reserved );
      blob->data[ offset ] = new_byte;
      ++offset;

    } else {
      new_byte = nibble << 4;
    }

    ++ip;
    ++num_chars;
  }
  if( offset > blob->count ) {
    dbg( DbgInfo, "setting new count to %d\n", offset );
    blob->count = offset;
  }
  assert( blob_is_valid( blob ) );
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

uint32_t blob_capacity( const blob_t *blob ) {
  return blob->reserved;
}

void* blob_data( blob_t *blob ) {
  return blob->data;
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
  // assert( blob );
  // assert( blob->data );
  // assert( blob->reserved >= blob->count );
  return blob && blob->data && blob->reserved >= blob->count;
}

void blob_dump( const blob_t* blob ) {
  dbg( DbgTrace, "%4d / %4d : ", blob->count, blob->reserved );
  uint32_t max_i = blob->count > 512 ? 512 : blob->count;
  for( uint32_t i=0; i<max_i; ++i ) {
    // if( i > 0 && ( i % 32 ) == 0 )
    //   printf( "\n                ");
    dbg( DbgTrace, "%02x ", blob->data[i] );
  } 
  if( max_i != blob->count )
    dbg( DbgTrace, "...");
  dbg( DbgTrace, "\n");
}

bool blob_save( const blob_t* blob, const char* ofilename ) {
  FILE *f = fopen( ofilename, "wb" );
  if( !f ) 
    return false;
  size_t buffer_size = blob_size( blob );
  size_t bytes_saved = fwrite( blob->data, 1, buffer_size, f);
  fclose( f );
  return buffer_size == bytes_saved;
}

bool blob_load( blob_t* blob, const char* ifilename ) {
  FILE *f = fopen( ifilename, "rb" );
  if( !f ) 
    return false;
  // Quick and dirty way to get the file size
  fseek( f, 0, SEEK_END );
  size_t sz = ftell( f );
  rewind( f );
  blob_create( blob, sz, sz );
  int bytes_read = fread( blob->data, 1, sz, f);
  fclose( f );
  return sz == bytes_read;
}

void blob_view( blob_t* blob, blob_t* source, uint32_t offset, uint32_t size ) {
  blob->reserved = 0;
  assert( offset + size <= source->count );
  blob->data = source->data + offset;
  blob->count = size;
}

bool blob_match( const blob_t* b1, const blob_t* b2 ) {
  if( b1->count != b2->count )
    return false;
  return memcmp( b1->data, b2->data, b1->count ) == 0;
}


/* 

  if( !equal ) {
    const uint8_t* p0 = b0.data;
    int match_i = -1;
    int max_i = b1.count - b0b.count;
    int max_j = b0.count - 4;
    printf( "Searching block of %d bytes in block of %d bytes\n", max_j, max_i );
    for( int i=0; i<max_i; ++i ) {
      const uint8_t* p1 = b1.data + i;
      bool equal = true;
      if( blob_match( b1, b2 )) {
        match_i = i;
        break;
      }
    }
    if( match_i >= 0 )
      printf( "Blocks match at offset %d\n", match_i);

  }


*/