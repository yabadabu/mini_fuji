#pragma once

#include "blob.h"

// ---------------------------------------
typedef struct {
  blob_t recv_data;
} conn_t;

void conn_add_data( conn_t*, const void* new_data, uint32_t data_size );
void conn_update( conn_t*  );

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

int cmd_get_storage_ids( conn_t*, storage_ids_t* ); 
int cmd_get_objs( conn_t*, handles_t* ); 
int cmd_get_prop( conn_t*, uint16_t prop_id, prop_t* prop ); 
int cmd_set_prop( conn_t*, uint16_t prop_id, const prop_t* prop ); 
int cmd_open_session( conn_t* );
int cmd_close_session( conn_t* );
int cmd_get_obj( conn_t*, handle_t handle, blob_t* out_obj, void (*opt_progress)( float ) );
int cmd_del_obj( conn_t*, handle_t handle );


