#pragma once

enum eOpCode {
  OC_INVALID = 0,
  OP_DISCOVER_CAMERA,
  OP_CONNECT_TO_CAMERA, 
  OC_READ_STORAGE_IDS,
  OC_READ_OBJ_HANDLES,
  OC_SET_PROP,
  OC_GET_PROP_ARRAY,
  OC_SET_PROP_ARRAY,
  OC_INITIATE_CAPTURE,
  OC_TERMINATE_CAPTURE,
  OC_WAIT_SHOOT_ENDS,
  OC_SAVE_IMAGES,
  OC_DELETE_IMAGES,
  OC_END_OF_PROGRAM,
};

typedef struct {
  enum eOpCode op_code;
  uint32_t     prop_id;
  uint32_t     ivalue;
} op_code_t;

// ------------------------------------------------------
#define max_props_in_array        16
typedef struct {
  uint32_t     count;
  uint32_t     ids[ max_props_in_array ];
  uint32_t     ivalues[ max_props_in_array ];
} prop_array_t;

void prop_arr_clear( prop_array_t* prar );
int  prop_arr_set( prop_array_t* prar, uint32_t prop_id, uint32_t ivalue );
bool prop_arr_del( prop_array_t* prar, uint32_t prop_id );
bool prop_arr_get( prop_array_t* prar, uint32_t prop_id, uint32_t* out_ivalue );
void prop_arr_dump( prop_array_t* prar );

// ------------------------------------------------------
typedef struct {
  op_code_t*    actions;
  conn_t*       conn;
  int           max_time_per_step;    // usecs. 1ms = 1000

  // Runtime
  int           ip;
  int           steps_in_ip;
  int           iteration;

  // context ..
  storage_ids_t storage_ids;
  handles_t     handles;
  blob_t        download_buffer;
  camera_info_t camera_info;
  prop_array_t* custom_props;

} evaluation_t;

void eval_create( evaluation_t* ev, conn_t* c, op_code_t* actions );
bool eval_step( evaluation_t* ev );
