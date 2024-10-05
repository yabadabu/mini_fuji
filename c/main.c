#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "blob.h" 
#include "connection.h" 
#include "properties.h" 
#include "discovery.h" 
#include "actions.h" 
#include "actions_list.h" 

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
  conn.trace_io = false;
  conn.trace_processed_packets = false;

  evaluation_t ev;
  eval_create( &ev, &conn, actions_take );
  //eval_create( &ev, &conn, actions_set_config );

  prop_array_t parr;
  prop_arr_clear( &parr );
  prop_arr_set( &parr, PDV_Quality,        PDV_Quality_Fine );
  prop_arr_set( &parr, PDV_Exposure_Index, PDV_Exposure_Index_ISO_200 );
  // Camera needs to be in Time Mode
  prop_arr_set( &parr, PDV_Exposure_Time,  PDV_Exposure_Time_5_secs );
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

