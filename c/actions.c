#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "connection.h"
#include "discovery.h"
#include "properties.h"

extern void download_progress( void* context, uint32_t curr, uint32_t required );
extern void notify_event( void* context, const char* event_str );


enum eOpCode {
  OC_INVALID = 0,
  OP_DISCOVER_CAMERA,
  OP_CONNECT_TO_CAMERA, 
  OC_READ_STORAGE_IDS,
  OC_READ_OBJ_HANDLES,
  OC_SET_PROP,
  OC_SET_PROPS,
  OC_INITIATE_CAPTURE,
  OC_WAIT_SHOOT_ENDS,
  OC_SAVE_IMAGES,
  OC_DELETE_IMAGES,
  OC_END_OF_PROGRAM,
};

typedef struct {
  enum eOpCode op_code;
  prop_t*      prop;
  uint32_t     arg2;
} op_code_t;

op_code_t action_take[] = {
  { OP_DISCOVER_CAMERA   },
  { OC_READ_STORAGE_IDS  },
  { OC_SET_PROP,         &prop_quality,         PDV_Quality_Fine },
  { OC_SET_PROP,         &prop_priority_mode,   PDV_Priority_Mode_USB },
  { OC_SET_PROP,         &prop_exposure_time,   PDV_Exposure_Time_5_secs },

  { OC_SET_PROP,         &prop_capture_control, PDV_Capture_Control_AutoFocus },
  { OC_INITIATE_CAPTURE, },
  { OC_SET_PROP,         &prop_capture_control, PDV_Capture_Control_Shoot },
  { OC_INITIATE_CAPTURE, },

  { OC_WAIT_SHOOT_ENDS,  },
  { OC_SET_PROP,         &prop_priority_mode,   PDV_Priority_Mode_Camera },
  { OC_READ_OBJ_HANDLES  },
  { OC_END_OF_PROGRAM    }
};

typedef struct {
  op_code_t*    actions;
  conn_t*       conn;
  int           max_time_per_step;

  // Runtime
  int           ip;
  int           steps_in_ip;
  int           iteration;

  // context ..
  storage_ids_t storage_ids;
  handles_t     handles;
  blob_t        download_buffer;
  camera_info_t camera_info;

} evaluation_t;

void eval_create( evaluation_t* ev, conn_t* c, op_code_t* actions ) {
  ev->actions = actions;
  ev->ip = 0;
  ev->steps_in_ip = 0;
  ev->iteration = 0;
  ev->conn = c;
  ev->max_time_per_step = 1000;
  c->on_event = (callback_event_t){ .context = NULL, .callback = &notify_event };
  blob_create( &ev->download_buffer, 0, 64 * 1024 );
}

static void eval_next_ip( evaluation_t* ev ) {
  ev->ip += 1;
  ev->steps_in_ip = 0;
  ev->iteration = 0;
}

static void eval_next_substep( evaluation_t* ev ) {
  ev->steps_in_ip += 1;
}

static void eval_next_iteration( evaluation_t* ev ) {
  ev->iteration += 1;
}

bool eval_error( evaluation_t* ev, const char* msg ) {
  printf( "Error: %s\n", msg );
  return true;
}

bool eval_step( evaluation_t* ev ) {
  conn_t* c = ev->conn;
  assert( c );

  // Are we busy?
  if( conn_is_waiting_answer( c ) ) {
    conn_update( c, ev->max_time_per_step );
    // We are still waiting... let's go
    if( conn_is_waiting_answer( c ) )
      return false;
  }

  op_code_t* cmd = ev->actions + ev->ip;
  int sub_step = ev->steps_in_ip;
  switch( cmd->op_code ) {

  case OP_DISCOVER_CAMERA:

    if( sub_step == 0 ) {
      printf( "OP_DISCOVER_CAMERA. Start\n");
      network_interface_t ni[4];
      int num_interfaces = ch_get_local_network_interfaces( ni, 4 );
      const char* local_ip = NULL;
      for( int i=0; i<num_interfaces; ++i ) {
        if( strcmp( ni[i].ip, "127.0.0.1" ) != 0 ) {
          printf( "%16s : %s\n", ni[i].ip, ni[i].name );
          if( !local_ip )
            local_ip = ni[i].ip;
        }
      }
      if( !local_ip )
        return eval_error( ev, "Failed to identify local ip" ); 
      if( !discovery_start( local_ip ) )
        return eval_error( ev, "Failed to discovery_start" ); 
      eval_next_substep( ev );

    } else if( sub_step == 1 ) {
      printf( "OP_DISCOVER_CAMERA. Waiting for camera\n");

      if( discovery_update( &ev->camera_info, ev->max_time_per_step ) )
        eval_next_substep( ev );

    } else if( sub_step == 2 ) {
      printf( "OP_DISCOVER_CAMERA. Camera found\n");
      discovery_stop();    
      eval_next_ip( ev );
    }

    break;

  case OP_CONNECT_TO_CAMERA:
    if( sub_step == 0 ) {
      char conn_str[128] = {"tcp:"};
      strcat(conn_str, ev->camera_info.ip);
      printf( "OP_CONNECT_TO_CAMERA. Connect to camera at %s:%d\n", conn_str, ev->camera_info.port );
      if( !ch_create( &c->channel, conn_str, ev->camera_info.port) )
        return eval_error( ev, "Failed to identify camera info");
      eval_next_substep( ev );

    } else if( sub_step == 1 ) {
      printf( "OP_CONNECT_TO_CAMERA. Initialize" );
      ptpip_initialize( c );
      eval_next_substep( ev );

    } else if( sub_step == 2 ) {
      printf( "OP_CONNECT_TO_CAMERA. Close session" );
      ptpip_close_session( c );
      eval_next_substep( ev );

    } else if( sub_step == 3 ) {
      printf( "OP_CONNECT_TO_CAMERA. Open session" );
      ptpip_open_session( c );
      eval_next_ip( ev );

    }
    break;

  case OC_READ_STORAGE_IDS:
    if( ( ev->steps_in_ip & 1 ) == 0 ) {
      ptpip_get_storage_ids( c, &ev->storage_ids );
      eval_next_substep( ev );
    } else if( ev->storage_ids.count > 0 ) {
      eval_next_ip( ev );
    } else {
      printf( "No storage founds!!\n");
      return true;
    }
    break;

  case OC_SET_PROP: {
    prop_t* p = cmd->prop;
    p->ivalue = cmd->arg2;
    printf( "Setting prop %04x %s to %08x:%s\n", p->id, p->name, p->ivalue, prop_get_value_str( p ) );
    ptpip_set_prop( c, p );
    eval_next_ip( ev );
    break; }

  case OC_INITIATE_CAPTURE:
    printf( "Initiate Capture\n" );
    eval_next_ip( ev );
    break;

  case OC_WAIT_SHOOT_ENDS: {
    printf( "Checking if shoot ends\n" );

    if( ( ev->steps_in_ip & 1 ) == 0 ) {
      prop_pending_events.ivalue = 0;
      ptpip_get_prop( c, &prop_pending_events );
      eval_next_substep( ev );
    } 
    else if( prop_pending_events.ivalue > 0 ) {
      eval_next_ip( ev );
    }
    break; }

  case OC_READ_OBJ_HANDLES:
    printf( "Reading obj handles\n" );
    ptpip_get_obj_handles( c, ev->storage_ids.ids[0], &ev->handles );
    eval_next_substep( ev );
    break;

  case OC_SAVE_IMAGES:
    printf( "Downloading obj %d / %d\n", ev->iteration, ev->handles.count );
    if( ev->iteration < ev->handles.count ) {
      handle_t h = ev->handles.handles[ ev->iteration ];
      if( ev->steps_in_ip == 0 ) {
        c->on_progress = (callback_progress_t){ .context = NULL, .callback = &download_progress };
        ptpip_get_obj( c, h, &ev->download_buffer );
        eval_next_substep( ev );
      }
      else {
        clear_callback_progress( &c->on_progress );
        char ofilename[256];
        sprintf( ofilename, "img_%04d.jpg", ev->iteration - 1 );
        printf( "Saving %d bytes to %s\n", blob_size( &ev->download_buffer ), ofilename );
        blob_save( &ev->download_buffer, ofilename );
        eval_next_iteration( ev );
      }

    } else {
      eval_next_ip( ev );
    }
    break;

  case OC_DELETE_IMAGES:
    printf( "Deleting obj %d / %d\n", ev->iteration, ev->handles.count );
    if( ev->iteration < ev->handles.count ) {
      handle_t h = ev->handles.handles[ ev->iteration ];
      ptpip_del_obj( c, h );
      eval_next_iteration( ev );

    } else {
      eval_next_ip( ev );
    }

    break;

  case OC_END_OF_PROGRAM:
    printf( "End of program\n" );
    return true;

  default:
    printf( "Unsupported op code %04x", cmd->op_code );
    return true;

  }
  return false;
}

// ---------------------------------
static void show_waiting_answer() {
  char tc[5] = "\\|/-";
  static int idx = 0;
  printf( "\rWaiting answer from the camera... %c ", tc[idx] );
  fflush( stdout );
  idx = ( idx + 1 ) % 4;
}

bool test_evals() {
  conn_t       conn;
  evaluation_t ev;
  eval_create( &ev, &conn, action_take );
  while( !eval_step( &ev )) {
    show_waiting_answer();
  }
  return false;
}
