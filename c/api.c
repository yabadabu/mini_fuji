#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int32_t value; } handle_t;
typedef struct { int32_t value; } storage_id_t;

typedef struct { 
  int32_t  count;
  handle_t data[7];
} array_handles_t;

typedef struct {
  int32_t  count;
  int32_t  reserved;
  uint8_t* data;
} blob_t;

typedef struct {
  // ...
} connection_t;

int blob_create( blob_t* blob, int num_bytes ) {
  blob->data = malloc( num_bytes );
  blob->count = num_bytes;
  blob->reserved = num_bytes;
  return 0;
}

int blob_resize( blob_t* blob, int new_size ) {
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

int blob_append( blob_t* blob, const blob_t* other ) {
  int32_t prev_sz = blob->count;
  int32_t new_sz = blob->count + other->count;
  blob_resize( blob, new_sz );
  memcpy( blob->data + prev_sz, other->data, other->count );
  return 0;
}

int blob_write_u16( blob_t* blob, int offset, uint16_t value ) {
  assert( offset + value <= blob->count );
  // ...
  return 0;
}

int blob_write_u32( blob_t* blob, int offset, uint32_t value ) {
  assert( offset + value <= blob->count );
  // ...
  return 0;
}

int blob_msg( blob_t* blob, int16_t id, const uint8_t* payload, size_t payload_size, const uint16_t* msg_type, const uint32_t* counter ) {

  // // msg_size(4) | num_part(2) | msg_type(2) | msg_idx(4) | payload( N )
  // function makeMsg( msg_id, payload = '', msg_type = msg_type_cmd, msg_counter = null) {
  //   if( !msg_counter ) 
  //     msg_counter = generateMsgCounter();
  //   const net_msg_type = swapInt16( msg_type )
  //   const net_msg_id = swapInt16( msg_id )
  //   const msg = Buffer.from( net_msg_type + net_msg_id + msg_counter + payload, 'hex')
  //   return prefixSize( msg )
  // }
  int size = 4 + 2 + 2 + 4 + payload_size;
  blob_resize( blob, size );
  blob_write_u32( blob, 0, size );
  blob_write_u16( blob, 4, *msg_type );
  blob_write_u16( blob, 4, id );
  blob_write_u32( blob, 4, *counter );

  return 0;
}

// -------------------
int ptpip_open_session() {
  return 0;  
}

int ptpip_get_storage_ids( array_handles_t* out_handles ) {
  out_handles->count = 2;
  out_handles->data[0].value = 0x2345612;
  out_handles->data[1].value = 0x22334455;
  return 0;
}

bool run() {
  array_handles_t handles;
  if( ptpip_get_storage_ids( &handles ) )
    return false;
  for( int i=0; i<handles.count; ++i )
    printf( "%04d : %08x\n", i, handles.data[i].value );
  return true;
}

int main( int argc, char** argv ) {
  if( !run() )
    return -1;
  printf( "OK\n");
  return 0;
}

