#include <assert.h>
#include <stdio.h>
#include "connection.h"

cmd_t cmd_open_session      = { .id = 0x1002, .name = "open_session" };
cmd_t cmd_close_session     = { .id = 0x1003, .name = "close_session" };
cmd_t cmd_initiate_capture  = { .id = 0x100e, .name = "initiate_capture" };
cmd_t cmd_initiate_open_capture  = { .id = 0x101c, .name = "initiate_open_capture" };
cmd_t cmd_terminate_capture = { .id = 0x1018, .name = "terminate_capture" };
cmd_t cmd_del_obj           = { .id = 0x100b, .name = "del_obj" };

// ------------------------------------------------------
int parse_initialize_comm( const blob_t* args, void* output ) {
  printf( "Parse initialize_comm\n" );
  blob_dump( args );
  // 16 bytes guis
  // friendly name in utf16
  return 0;
}

cmd_t cmd_initialize_comm = { 
  .id = 0x0000, 
  .name = "initialize_comm",
  .parse = &parse_initialize_comm
};

// ------------------------------------------------------
int parse_get_prop( const blob_t* args, void* output ) {
  prop_t* prop = (prop_t*) output;
  assert( output );
  
  printf( "Parsing get_prop answer\n" );

  if( prop->data_type == PDT_U16 ) {
    assert( blob_size( args ) == 2 );
    prop->ivalue = blob_read_u16le( args, 0 );

  } else if( prop->data_type == PDT_U32 ) {
    assert( blob_size( args ) == 4 );
    prop->ivalue = blob_read_u32le( args, 0 );

  } else if( prop->data_type == PDT_ARRAY_PROP_VALUES ) {
    assert( blob_size( args ) >= 2 );
    prop->ivalue = blob_read_u16le( args, 0 );
    // Store the full stream here
    blob_create( &prop->blob, 0, blob_size( args ) );
    blob_append_blob( &prop->blob, args );
  }

  return 0; 
}

cmd_t cmd_get_prop = { 
  .id = 0x1015, 
  .name = "get_prop", 
  .parse = &parse_get_prop 
};

cmd_t cmd_set_prop = { 
  .id = 0x1016, 
  .name = "set_prop", 
};

// ------------------------------------------------------
int parse_get_storage_ids( const blob_t* args, void* output ) {
  storage_ids_t* out_ids = (storage_ids_t*) output;
  out_ids->count = blob_read_u32le( args, 0 );
  if( out_ids->count > 3 )
    out_ids->count = 3;
  for( int i=0; i<out_ids->count; ++i ) {
    uint32_t id = blob_read_u32le( args, 4 + i * 4 );
    out_ids->ids[i].id = id;
  }
  return 0; 
}

cmd_t cmd_get_storage_ids = { 
  .id = 0x1004, 
  .name = "get_storage_ids", 
  .parse = &parse_get_storage_ids 
};

// ------------------------------------------------------
int parse_get_obj_handles( const blob_t* args, void* output ) {
  handles_t* out_ids = (handles_t*) output;
  out_ids->count = blob_read_u32le( args, 0 );
  if( out_ids->count > 3 )
    out_ids->count = 3;
  for( int i=0; i<out_ids->count; ++i ) {
    uint32_t id = blob_read_u32le( args, 4 + i * 4 );
    out_ids->handles[i].value = id;
  }
  return 0; 
}

cmd_t cmd_get_obj_handles = { 
  .id = 0x1007, 
  .name = "get_obj_handles", 
  .parse = &parse_get_obj_handles
};

// ------------------------------------------------------
int store_contents_in_blob( const blob_t* args, void* output ) {
  blob_t* b = (blob_t*) output;
  assert( b && args );
  //printf( "Adding %d bytes to existing blob of %d/%d\n", args->count, b->count, b->reserved );
  // if( args->count < 32 )
  //   blob_dump( args );
  blob_append_data( b, args->data, blob_size(args) );
  return 0; 
}

cmd_t cmd_get_obj = { 
  .id = 0x1009, 
  .name = "get_obj", 
  .parse = &store_contents_in_blob
};

cmd_t cmd_get_thumbnail = { 
  .id = 0x100a, 
  .name = "get_thumbnail", 
  .parse = &store_contents_in_blob
};


// ------------------------------------------------------
int parse_partial_obj( const blob_t* args, void* output ) {
  blob_t* b = (blob_t*) output;
  assert( b && args );
  // The camera is sending an extra 4 bytes with the offset where the block is meant
  uint32_t sz = blob_size( args );
  if( sz > 4 )
    blob_append_data( b, args->data, sz - 4 );
  return 0; 
}

cmd_t cmd_get_partial_obj = { 
  .id = 0x101b, 
  .name = "get_partial_obj", 
  .parse = &parse_partial_obj
};



