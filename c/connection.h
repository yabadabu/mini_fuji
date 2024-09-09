#pragma once

#include "blob.h"

// ---------------------------------------
typedef struct {
  uint16_t    id;
  const char* name;
  int       (*parse)( const blob_t*, void* );
} cmd_t;

// ---------------------------------------
typedef struct {
  blob_t   recv_data;
  blob_t   curr_packet;
  blob_t   last_answer;
  uint32_t sequence_id;
  cmd_t*   curr_cmd;
  void*    curr_output;
} conn_t;

bool conn_create( conn_t* );
void conn_add_data( conn_t*, const void* new_data, uint32_t data_size );
void conn_update( conn_t* );
void conn_create_cmd_msg( conn_t* conn, blob_t* msg, const cmd_t* cmd_id );

// ---------------------------------------
typedef struct {
  uint32_t size;
  uint16_t type;
  uint16_t id;
  uint32_t counter;
  uint8_t  payload[];
} packet_t;
// ---------------------------------------
typedef struct { 
  int16_t id; 
  int32_t ivalue;
  blob_t  blob;
} prop_t;

// ---------------------------------------
typedef struct {
  uint32_t storage_id;
} storage_id_t;

typedef struct { 
  int32_t  count;
  storage_id_t ids[3];
} storage_ids_t;

typedef struct { 
  int32_t value; 
} handle_t;

typedef struct { 
  int32_t  count;
  handle_t handles[7];
} handles_t;

int ptpip_get_storage_ids( conn_t*, storage_ids_t* ); 
int ptpip_get_objs( conn_t*, handles_t* ); 
int ptpip_get_prop( conn_t*, prop_t* prop ); 
int ptpip_set_prop( conn_t*, uint16_t prop_id, const prop_t* prop ); 
int ptpip_open_session( conn_t* );
int ptpip_close_session( conn_t* );
int ptpip_get_obj( conn_t*, handle_t handle, blob_t* out_obj, void (*opt_progress)( float ) );
int ptpip_del_obj( conn_t*, handle_t handle );


