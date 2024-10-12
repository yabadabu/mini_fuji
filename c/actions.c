#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "connection.h"
#include "discovery.h"
#include "properties.h"
#include "actions.h"
#include "dbg.h"

// ------------------------------------------------------
int prop_arr_find_idx( prop_array_t* prar, uint32_t prop_id ) {
  assert( prar );
  uint32_t* ids = prar->ids;
  for(uint32_t i=0; i<prar->count; ++i, ++ids ) {
    if( *ids == prop_id )
      return i;
  } 
  return -1;
}

void prop_arr_clear( prop_array_t* prar ) {
  prar->count = 0;
}

void prop_arr_dump( prop_array_t* prar ) {
  dbg( DbgInfo, "Property Array contains %d properties/values\n", prar->count );
  for(uint32_t i=0; i<prar->count; ++i ) {
    uint32_t prop_id = prar->ids[ i ];
    uint32_t ivalue = prar->ivalues[ i ];
    uint32_t read_value = 0xffffffff;
    bool found = prop_arr_get( prar, prop_id, &read_value );
    assert( found );
    assert( read_value == ivalue );
    prop_t* p = prop_by_id( prop_id );
    assert( p );
    p->ivalue = ivalue;
    dbg( DbgInfo, "  [%d] 0x%04x:%32s => %08x (%s)\n", i, p->id, p->name, p->ivalue, prop_get_value_str( p ) ); 
  }
}

int prop_arr_set( prop_array_t* prar, uint32_t prop_id, uint32_t ivalue ) {
  int idx = prop_arr_find_idx( prar, prop_id );
  if( idx >= 0 ) {
    prar->ivalues[idx] = ivalue;
    return idx;
  }
  if( prar->count >= max_props_in_array )
    return -1;
  // Register a new value
  prar->ids[ prar->count ] = prop_id;
  prar->ivalues[ prar->count ] = ivalue;
  prar->count++;
  return prar->count - 1;
}

bool prop_arr_del( prop_array_t* prar, uint32_t prop_id ) {
  int idx = prop_arr_find_idx( prar, prop_id );
  if( idx < 0 )
    return false;
  uint32_t last_idx = prar->count - 1;
  if( idx != last_idx ) {
    prar->ids[ idx ] = prar->ids[ last_idx ];
    prar->ivalues[ idx ] = prar->ivalues[ last_idx ];
  }
  --prar->count;
  return true;
}

bool prop_arr_get( prop_array_t* prar, uint32_t prop_id, uint32_t* out_ivalue ) {
  int idx = prop_arr_find_idx( prar, prop_id );
  if( idx < 0 )
    return false;
  *out_ivalue = prar->ivalues[ idx ];
  return true;
}

void eval_create( evaluation_t* ev, conn_t* c, const op_code_t* actions ) {
  ev->actions = actions;
  ev->ip = 0;
  ev->steps_in_ip = 0;
  ev->iteration = 0;
  ev->conn = c;
  ev->max_time_per_step = 1000;
  blob_create( &ev->download_buffer, 0, 64 * 1024 );
}

static void eval_next_ip( evaluation_t* ev ) {
  ev->ip += 1;
  ev->steps_in_ip = 0;
  ev->iteration = 0;
  ev->cycles_in_substep = 0;
}

static void eval_next_iteration( evaluation_t* ev ) {
  ev->steps_in_ip = 0;
  ev->iteration += 1;
  ev->cycles_in_substep = 0;
}

static void eval_next_substep( evaluation_t* ev ) {
  ev->steps_in_ip += 1;
  ev->cycles_in_substep = 0;
}

bool eval_error( evaluation_t* ev, const char* msg ) {
  dbg( DbgError, "EvalError: %s\n", msg );
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

  const op_code_t* cmd = ev->actions + ev->ip;
  int sub_step = ev->steps_in_ip;

  //dbg( DbgTrace, "eval.step( Cmd:%d IP:%d.%d.%d\n", cmd->op_code, ev->ip, ev->iteration, sub_step );
  switch( cmd->op_code ) {

  case OP_DISCOVER_CAMERA:

    if( sub_step == 0 ) {
      dbg( DbgInfo, "OP_DISCOVER_CAMERA. Start\n");
      network_interface_t ni[16];
      int num_interfaces = ch_get_local_network_interfaces( ni, 16 );
      int best_idx = -1;
      bool best_is_ethernet = false;
      bool best_is_bridge = false;
      for( int i=0; i<num_interfaces; ++i ) {

        // Skip localhost
        if( strcmp( ni[i].ip, "127.0.0.1" ) == 0 )
          continue;


        // in iOS en0/en1 are the ethernet adapters
        //        pdp_ip* are 3G,4G cellular data
        //        ipsec*  are VPN addresses
        //        bridge* are the WIFI shared connections
        bool is_ethernet = strncmp( ni[i].name, "en", 2 ) == 0;
        bool is_bridge = false; //strncmp( ni[i].name, "bridge", 6 ) == 0;

        bool keep_it = ( best_idx == -1 )
                    || ( is_bridge && !best_is_bridge)
                    || ( !best_is_bridge && is_ethernet && !best_is_ethernet );
        dbg( DbgInfo, "%16s : %s (E:%d B:%d -> %d)\n", ni[i].ip, ni[i].name, is_ethernet, is_bridge, keep_it );
        if( keep_it ) {
          best_idx = i;
          best_is_ethernet = is_ethernet;
          best_is_bridge = is_bridge;
        }
      }

      const char* local_ip = ( best_idx != -1 ) ? ni[ best_idx ].ip : NULL;
      if( !local_ip )
        return eval_error( ev, "Failed to identify local ethernet ip" ); 

      dbg( DbgInfo, "Using localIP %s\n", local_ip);
      if( !discovery_start( local_ip ) )
        return eval_error( ev, "Failed to discovery_start" ); 
      eval_next_substep( ev );

    } else if( sub_step == 1 ) {
      if( ev->cycles_in_substep == 0 )
        dbg( DbgInfo, "OP_DISCOVER_CAMERA. Waiting for camera\n");

      if( discovery_update( &ev->camera_info, ev->max_time_per_step ) )
        eval_next_substep( ev );
      else
        ev->cycles_in_substep +=1;

    } else if( sub_step == 2 ) {
      dbg( DbgInfo, "OP_DISCOVER_CAMERA. Camera found\n");
      discovery_stop();    
      eval_next_ip( ev );
    }

    break;

  case OP_CONNECT_TO_CAMERA:
    if( sub_step == 0 ) {
      char conn_str[128] = {"tcp:"};
      strcat(conn_str, ev->camera_info.ip);
      dbg( DbgInfo, "OP_CONNECT_TO_CAMERA. Connect to camera at %s:%d\n", conn_str, ev->camera_info.port );
      if( !ch_create( &c->channel, conn_str, ev->camera_info.port) )
        return eval_error( ev, "Failed to identify camera info");
      eval_next_substep( ev );

    } else if( sub_step == 1 ) {
      dbg( DbgInfo, "OP_CONNECT_TO_CAMERA. Initialize\n" );
      ptpip_initialize( c );
      eval_next_substep( ev );

    } else if( sub_step == 2 ) {
      dbg( DbgInfo, "OP_CONNECT_TO_CAMERA. Close session\n" );
      ptpip_close_session( c );
      eval_next_substep( ev );

    } else if( sub_step == 3 ) {
      dbg( DbgInfo, "OP_CONNECT_TO_CAMERA. Open session\n" );
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
      dbg( DbgInfo, "No storage founds!!\n");
      return true;
    }
    break;

  case OC_GET_PROP_ARRAY:
    if( ev->custom_props && ev->iteration < ev->custom_props->count ) {
      uint32_t prop_id = ev->custom_props->ids[ ev->iteration ];
      prop_t* prop = prop_by_id( prop_id );
      if( prop ) {
        if( sub_step == 0 ) {
          ptpip_get_prop( c, prop );
          eval_next_substep( ev );
        } else {
          dbg( DbgInfo, "GET_PROP_ARRAY[%d] %04x:%s => %08x (%s) (RC:%s)\n", ev->iteration, prop->id, prop->name, prop->ivalue, prop_get_value_str( prop ), ptpip_error_msg( conn_get_last_ptpip_return_code( c ) )); 
          prop_arr_set( ev->custom_props, prop_id, prop->ivalue );
          eval_next_iteration( ev );
        }
      } else {
        // If the prop is not registered report and try the next one
        dbg( DbgInfo, "GET_PROP_ARRAY.Property %04x is not registered\n", prop_id );
        eval_next_iteration( ev );
      }
    } else {
      eval_next_ip( ev );
    }
    break;

  case OC_SET_PROP_ARRAY:
    if( ev->custom_props && ev->iteration < ev->custom_props->count ) {
      uint32_t prop_id = ev->custom_props->ids[ ev->iteration ];
      prop_t* prop = prop_by_id( prop_id );
      if( prop ) {
        if( !prop->read_only ) {
          if( prop_arr_get( ev->custom_props, prop_id, &prop->ivalue ) ) {
            dbg( DbgInfo, "SET_PROP_ARRAY[%d] %04x:%s => %08x (%s)\n", ev->iteration, prop->id, prop->name, prop->ivalue, prop_get_value_str( prop )); 
            ptpip_set_prop( c, prop );
          } else {
            dbg( DbgInfo, "SET_PROP_ARRAY[%d].Property %04x is read-only\n", ev->iteration, prop_id );
          }
        } else {
          dbg( DbgInfo, "SET_PROP_ARRAY.Property %04x is read-only\n", prop_id );
        }
      } else {
        dbg( DbgInfo, "SET_PROP_ARRAY.Property %04x is not registered\n", prop_id );
      }
      eval_next_iteration( ev );
    } else {
      eval_next_ip( ev );
    }
    break;

  case OC_SET_PROP: {
    prop_t* p = prop_by_id( cmd->prop_id );
    if( p ) {
      p->ivalue = cmd->ivalue;
      dbg( DbgInfo, "Setting prop %04x:%s to %08x:%s\n", p->id, p->name, p->ivalue, prop_get_value_str( p ) );
      ptpip_set_prop( c, p );
    }
    eval_next_ip( ev );
    break; }

  case OC_INITIATE_CAPTURE:
    dbg( DbgInfo, "Initiate Capture\n" );
    ptpip_initiate_capture( c );
    eval_next_ip( ev );
    break;

  case OC_TERMINATE_CAPTURE:
    dbg( DbgInfo, "Terminate Capture\n" );
    ptpip_terminate_capture( c );
    eval_next_ip( ev );
    break;

  case OC_WAIT_SHOOT_ENDS: {
    if( sub_step == 0 )
      dbg( DbgInfo, "Checking if shoot ends\n" );

    if( ( ev->steps_in_ip & 1 ) == 0 ) {
      prop_pending_events.ivalue = 0;
      ptpip_get_prop( c, &prop_pending_events );
      eval_next_substep( ev );
    } 
    else if( prop_pending_events.ivalue > 0 ) {
      eval_next_ip( ev );
    }
    else {
      eval_next_substep( ev );
    }
    break; }

  case OC_READ_OBJ_HANDLES:
    dbg( DbgInfo, "Reading obj handles\n" );
    ptpip_get_obj_handles( c, ev->storage_ids.ids[0], &ev->handles );
    eval_next_ip( ev );
    break;

  case OC_SAVE_IMAGES:
    dbg( DbgInfo, "Downloading obj %d / %d [%d.%d]\n", ev->iteration, ev->handles.count, ev->iteration, sub_step );
    if( ev->iteration < ev->handles.count ) {
      handle_t h = ev->handles.handles[ ev->iteration ];
      if( ev->steps_in_ip == 0 ) {
        c->on_progress.enabled = true;
        ptpip_get_obj( c, h, &ev->download_buffer );
        eval_next_substep( ev );
      }
      else if( ev->steps_in_ip == 1 ) {
        c->on_progress.enabled = false;
        char ofilename[256];
        sprintf( ofilename, "img_%04d.jpg", ev->iteration );
        dbg( DbgInfo, "Saving %d bytes to %s\n", blob_size( &ev->download_buffer ), ofilename );
        blob_save( &ev->download_buffer, ofilename );
        eval_next_iteration( ev );
      }

    } else {
      eval_next_ip( ev );
    }
    break;

  case OC_DELETE_IMAGES:
    dbg( DbgInfo, "Deleting obj %d / %d\n", ev->iteration, ev->handles.count );
    if( ev->iteration < ev->handles.count ) {
      handle_t h = ev->handles.handles[ ev->iteration ];
      ptpip_del_obj( c, h );
      eval_next_iteration( ev );

    } else {
      eval_next_ip( ev );
    }

    break;

  case OC_END_OF_PROGRAM:
    dbg( DbgInfo, "End of program\n" );
    return true;

  default:
    dbg( DbgError, "Unsupported op code %04x", cmd->op_code );
    return true;

  }
  return false;
}


