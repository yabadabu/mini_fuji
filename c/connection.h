#pragma once

#include "blob.h"
#include "channel.h"

// ---------------------------------------
typedef struct {
  uint16_t    id;
  const char* name;
  int       (*parse)( const blob_t*, void* );
} cmd_t;

// ---------------------------------------
typedef struct {
   void*  context;
   void (*callback)( void* context, uint32_t current, uint32_t required );
   bool   enabled;
} callback_progress_t;

typedef struct {
   void*  context;
   void (*callback)( void* context, const char* str );
} callback_event_t;

void clear_callback_progress( callback_progress_t* cb );
void clear_callback_event( callback_event_t* cb );

// ---------------------------------------
typedef struct {
  blob_t              recv_data;      // Accumulated data before split it in packets
  blob_t              curr_packet;
  blob_t              last_answer;
  blob_t              otf_msg;
  blob_t              net_buffer;     // Used to read from the channel

  uint32_t            sequence_id;
  cmd_t*              curr_cmd;
  void*               curr_output;    // Extra arg to provide in the parse fn
  channel_t           channel;
  int                 last_cmd_result;

  callback_progress_t on_progress;
  callback_event_t    on_event;

  bool                trace_io;
  bool                trace_processed_packets;
} conn_t;

bool conn_create( conn_t* );
void conn_recv( conn_t* conn, const blob_t* new_data );
void conn_update( conn_t* conn, int usecs );
void conn_destroy( conn_t* conn );
void conn_clear_state( conn_t* conn );
bool conn_is_waiting_answer( conn_t* conn );
int  conn_get_last_ptpip_return_code( conn_t* conn );

// ---------------------------------------
enum eValueType {
  PDT_UNKNOWN = 0, 
  PDT_U16, 
  PDT_U32, 
  //U64, 
  PDT_STRING,
  PDT_ARRAY_PROP_VALUES,    // Custom
};

typedef struct { 
  uint16_t         id; 
  const char*      name;
  enum eValueType  data_type;
  uint32_t         ivalue;
  blob_t           blob;
  bool             read_only;
  int32_t          num_enums;
  const void*      enums_data;
} prop_t;

// ---------------------------------------
typedef struct {
  uint32_t id;
} storage_id_t;

#define max_storage_id_t    3
typedef struct { 
  int32_t  count;
  storage_id_t ids[max_storage_id_t];
} storage_ids_t;

typedef struct { 
  int32_t value; 
} handle_t;

#define max_handles_t       7
typedef struct { 
  int32_t  count;
  handle_t handles[max_handles_t];
} handles_t;

int ptpip_initialize( conn_t* conn );
int ptpip_get_storage_ids( conn_t*, storage_ids_t* ); 
int ptpip_get_obj_handles( conn_t*, storage_id_t storage_id, handles_t* ); 
int ptpip_get_prop( conn_t*, prop_t* prop ); 
int ptpip_set_prop( conn_t*, prop_t* prop ); 
int ptpip_open_session( conn_t* );
int ptpip_close_session( conn_t* );
int ptpip_initiate_capture( conn_t* );
int ptpip_initiate_open_capture( conn_t* );
int ptpip_terminate_capture( conn_t* );
int ptpip_get_obj( conn_t*, handle_t handle, blob_t* out_obj );
int ptpip_get_partial_obj( conn_t*, handle_t handle, uint32_t start, uint32_t size, blob_t* out_obj );
int ptpip_get_thumbnail( conn_t*, handle_t handle, blob_t* out_obj );
int ptpip_del_obj( conn_t*, handle_t handle );
const char* ptpip_error_msg( int rc );

// ---------------------------------------

