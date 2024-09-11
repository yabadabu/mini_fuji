#pragma once

#include "blob.h"

// ---------------------------------------
typedef struct {
  uint16_t    id;
  const char* name;
  int       (*parse)( const blob_t*, void* );
} cmd_t;

typedef struct {
   void*  context;
   void (*callback)( void* context, uint32_t current, uint32_t required );
} callback_progress_t;

// ---------------------------------------
typedef struct {
  blob_t              recv_data;
  blob_t              curr_packet;
  blob_t              last_answer;
  blob_t              otf_msg;
  uint32_t            sequence_id;
  cmd_t*              curr_cmd;
  void*               curr_output;    // Extra arg to provide in the parse fn
  callback_progress_t on_progress;
} conn_t;

bool conn_create( conn_t* );
void conn_add_data( conn_t*, const void* new_data, uint32_t data_size );
void conn_update( conn_t* );
void conn_destroy( conn_t* conn );
void conn_clear_state( conn_t* conn );
bool conn_is_waiting_answer( conn_t* conn );

// ---------------------------------------
enum eValueType {
  PDT_UNKNOWN = 0, 
  PDT_U16, 
  PDT_U32, 
  //U64, 
  PDT_String
};

typedef struct { 
  int16_t          id; 
  const char*      name;
  enum eValueType  data_type;
  int16_t          val16;
  int32_t          val32;
  blob_t           blob;
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
int ptpip_set_prop( conn_t*, prop_t* prop ); 
int ptpip_open_session( conn_t* );
int ptpip_close_session( conn_t* );
int ptpip_initiate_capture( conn_t* );
int ptpip_initiate_open_capture( conn_t* );
int ptpip_terminate_capture( conn_t* );
int ptpip_get_obj( conn_t*, handle_t handle, blob_t* out_obj, callback_progress_t on_progress );
int ptpip_del_obj( conn_t*, handle_t handle );

// ---------------------------------------
extern prop_t prop_quality;

