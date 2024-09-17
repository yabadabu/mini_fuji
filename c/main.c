#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "blob.h" 
#include "connection.h" 
#include "properties.h" 
#include "discovery.h" 
#include "actions.h" 

// -------------------
bool test_blobs();
bool test_conn();
bool test_channels();
bool test_props();
bool test_evals();
bool take_shot();

op_code_t actions_take[] = {
  { OP_DISCOVER_CAMERA   },
  { OP_CONNECT_TO_CAMERA },
  { OC_READ_STORAGE_IDS  },
  { OC_GET_PROP_ARRAY,   NULL,                  0 },
  { OC_SET_PROP_ARRAY,   NULL,                  0 },
//  { OC_SET_PROP,         &prop_quality,         PDV_Quality_Fine },
  { OC_SET_PROP,         &prop_priority_mode,   PDV_Priority_Mode_USB },
//  { OC_SET_PROP,         &prop_exposure_time,   PDV_Exposure_Time_5_secs },

  // This is required... even when we don't want autofocus
  { OC_SET_PROP,         &prop_capture_control, PDV_Capture_Control_AutoFocus },
  { OC_INITIATE_CAPTURE, },
  { OC_SET_PROP,         &prop_capture_control, PDV_Capture_Control_Shoot },
  { OC_INITIATE_CAPTURE, },

  { OC_WAIT_SHOOT_ENDS,  },
  { OC_READ_OBJ_HANDLES  },
  //{ OC_SAVE_IMAGES       },
  { OC_DELETE_IMAGES     },
  { OC_TERMINATE_CAPTURE },
  { OC_SET_PROP,         &prop_priority_mode,   PDV_Priority_Mode_Camera },
  { OC_END_OF_PROGRAM    }
};

op_code_t actions_set_config[] = {
  { OP_DISCOVER_CAMERA   },
  { OP_CONNECT_TO_CAMERA },
  { OC_SET_PROP_ARRAY,   NULL,                  0 },
  { OC_END_OF_PROGRAM    }
};

// ---------------------------------
static void show_waiting_answer() {
  char tc[5] = "\\|/-";
  static int idx = 0;
  //printf( "\rWaiting answer from the camera... %c", tc[idx] );
  //fflush( stdout );
  idx = ( idx + 1 ) % 4;
}

static void notify_event( void* context, const char* event_str ) {
  printf( "Evt: %s\n", event_str );
}

static void download_progress( void* context, uint32_t curr, uint32_t required ) {
  float ratio = (float) curr / (float) required;
  if( ratio > 1.0 )
    ratio = 1.0f;
  printf( "\rdownload_progress %d/%d (%.2f)%%", curr, required, ratio * 100.0f );
  if( ratio == 1.0f )
    printf( "\rdownload_progress Complete           \n" );
  fflush( stdout );
}

bool take_shot() {
  
  conn_t       conn;
  conn_create( &conn );
  conn.on_event = (callback_event_t) { .context = NULL, .callback = &notify_event };
  conn.on_progress = (callback_progress_t){ .context = NULL, .callback = &download_progress, .enabled = false };
  conn.trace_io = true;
  conn.trace_processed_packets = true;

  evaluation_t ev;
  //eval_create( &ev, &conn, actions_take );
  eval_create( &ev, &conn, actions_set_config );

  prop_array_t parr;
  prop_arr_clear( &parr );
  //prop_arr_set( &parr, PDV_Quality,        PDV_Quality_Fine );
  prop_arr_set( &parr, PDV_Exposure_Index, PDV_Exposure_Index_ISO_1600 );
  prop_arr_set( &parr, PDV_Exposure_Time,  PDV_Exposure_Time_2_secs );
  prop_arr_dump( &parr );
  //return false;

  ev.custom_props = &parr;

  printf( "eval_step starts\n" );
  while( !eval_step( &ev )) {
    show_waiting_answer();
  }
  return true;
}


int main( int argc, char** argv ) {

  if( !take_shot() )
    return -1;
  /*
  if( !test_evals() )
    return -1;
  if( !test_props() )
    return -1;
  if( !test_channels() )
    return -1;
  if( !test_blobs() )
    return -1;
  if( !test_conn() )
    return -1;
    */
  printf( "OK\n");
  return 0;
}

