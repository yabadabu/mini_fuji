#define _CRT_SECURE_NO_WARNINGS

#include "blob.h"
#include "connection.h"
#include "channel.h"
#include "discovery.h"
#include "properties.h"
#include <assert.h>
#include <string.h>     // strlen
#include <stdio.h>      // printf
#include <stdlib.h>     // atoi

bool ch_read_blob( channel_t* ch, blob_t* blob );

bool test_blobs() {
  printf( "Tessting blobs...\n");

  blob_t b0;
  blob_create( &b0, 6, 6 );
  bool is_valid = blob_is_valid( &b0 );
  assert( is_valid );
  blob_write_u32le( &b0, 0, 0x11223344 );
  blob_write_u16le( &b0, 4, 0x5566 );
  blob_dump( &b0 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[1] == 0x33 );
  assert( b0.data[2] == 0x22 );
  assert( b0.data[3] == 0x11 );
  assert( b0.data[4] == 0x66 );
  assert( b0.data[5] == 0x55 );

  blob_t b1;
  blob_create( &b1, 4, 4 );
  blob_write_u32le( &b1, 0, 0x88776655 );
  blob_dump( &b1 );
  uint32_t data_read = blob_read_u32le( &b1, 0 );
  assert( data_read == 0x88776655 );
  blob_append_blob( &b0, &b1 );
  blob_dump( &b0 );
  assert( b0.count == 6 + 4 );
  assert( b0.data[0] == 0x44 );
  assert( b0.data[6] == 0x55 ); 
  assert( blob_read_u16le( &b0, 6 ) == 0x6655 );

  blob_t b2;
  blob_create( &b2, 0, 0 );
  blob_remove_from_start( &b0, 2, &b2 );
  assert( blob_read_u16le( &b2, 0 ) == 0x3344 );
  blob_dump( &b2 );
  assert( b0.count == 8 );
  blob_dump( &b0 );
  assert( blob_read_u32le( &b0, 0 ) == 0x55661122 );

  blob_destroy( &b0 );
  blob_destroy( &b1 );
  assert( !blob_is_valid( &b0 ) );
  assert( !blob_is_valid( &b1 ) );

  blob_t bhex;
  blob_create( &bhex, 4, 32 );
  blob_write_u32le( &bhex, 0, 0x88664422 );
  printf( "Tessting blob 1\n");
  blob_from_hex_string( &bhex, 2, "A00a 33445566");
  blob_dump( &bhex );
  blob_from_hex_string( &bhex, -1, "33445566");
  assert( blob_read_u32le( &bhex, 0 ) == 0x0aA04422 );
  assert( blob_read_u32le( &bhex, 4 ) == 0x66554433 );
  assert( blob_read_u32le( &bhex, 8 ) == 0x66554433 );
  assert( blob_size( &bhex ) == 12 );
  blob_dump( &bhex );

  printf( "Tessting blobs OK\n");
  return true;
} 

void send_end_of_packet( conn_t* c ) {
  assert( conn_is_waiting_answer( c ) );
  blob_t eoc;
  blob_create( &eoc, 0, 256 );
  blob_from_hex_string( &eoc, 0, "0c000000 0300 1210 05000000" );
  conn_recv( c, &eoc );
  conn_update( c, 1 );
  assert( !conn_is_waiting_answer( c ) );
}

bool test_get_storage(conn_t* c) {
  printf( "test_get_storage\n" );
  conn_clear_state( c );
  // Send request
  storage_ids_t storage_ids;
  ptpip_get_storage_ids( c, &storage_ids );
  assert( conn_is_waiting_answer( c ) );

  // Fake network answer
  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "18000000 0200 0210 05000000 020000000100001002000010");
  blob_dump( &ans );
  conn_recv( c, &ans );
  conn_update( c, 1 );

  send_end_of_packet( c );

  assert( storage_ids.count == 2 );
  assert( storage_ids.ids[0].id == 0x10000001 );
  assert( storage_ids.ids[1].id == 0x10000002 );
  return true;
}

bool test_get_prop(conn_t* c) {
  printf( "test_get_prop\n" );
  conn_clear_state( c );
  prop_t p = prop_quality;
  ptpip_get_prop( c, &p );

  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "0e000000 0200 1510 05000000 0100");
  blob_dump( &ans );
  conn_recv( c, &ans );
  conn_update( c, 1 );

  send_end_of_packet( c );

  assert( c->recv_data.count == 0);
  assert( p.id == prop_quality.id );
  assert( p.ivalue == 0x0001 );
  return true;
}

bool test_set_prop(conn_t* c) {
  printf( "test_set_prop\n" );
  conn_clear_state( c );
  prop_t p = prop_quality;
  p.ivalue = 2;
  ptpip_set_prop( c, &p );
  send_end_of_packet( c );
  return true;
}

bool test_initiate_capture(conn_t* c) {
  printf( "test_initiate_capture\n" );
  conn_clear_state( c );
  ptpip_initiate_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_initiate_open_capture(conn_t* c) {
  printf( "test_initiate_open_capture\n" );
  conn_clear_state( c );
  ptpip_initiate_open_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_terminate_capture(conn_t* c) {
  printf( "test_terminate_capture\n" );
  conn_clear_state( c );
  ptpip_terminate_capture( c );
  send_end_of_packet( c );
  return true;
}

bool test_del_obj(conn_t* c) {
  printf( "test_del_obj\n" );
  conn_clear_state( c );
  handle_t h = { .value = 0x88224411 };
  ptpip_del_obj( c, h );
  send_end_of_packet( c );
  return true;
}

static void callback_progress( void* context, uint32_t curr, uint32_t required ) {
  printf( "On callback_progress %d/%d (Ctx:%p)\n", curr, required, context );
  assert( context );
  int* counter = (int*) context;
  *counter += 1;
}

bool test_get_obj(conn_t* c) {
  printf( "test_get_obj\n" );
  conn_clear_state( c );
  handle_t h = { .value = 0x88224411 };
  blob_t output;
  blob_create( &output, 0, 0 );

  int called = 0;
  callback_progress_t my_progress = { .context = &called, .callback = &callback_progress };
  c->on_progress = my_progress;
  ptpip_get_obj( c, h, &output );

  blob_t ans;
  blob_create( &ans, 0, 256 );
  blob_from_hex_string( &ans, 0, "14000000 0200 1510 05000000 01002233");
  blob_dump( &ans );
  conn_recv( c, &ans );
  conn_update( c, 1 );

  blob_t ans2;
  blob_create( &ans2, 0, 256 );
  blob_from_hex_string( &ans2, 0, "88998877");
  conn_recv( c, &ans2 );

  conn_update( c, 1 );
  assert( output.count == 8 );
  blob_dump( &output );

  send_end_of_packet( c );

  assert( output.data[0] == 0x01 );
  assert( output.data[4] == 0x88 );
  assert( called == 3 );
  return true;
}

bool test_close_session( conn_t* c ) {
  printf( "ptpip_close_session\n");
  conn_clear_state( c );
  ptpip_close_session( c );
  return true;
}

bool test_open_session( conn_t* c ) {
  printf( "Test open session\n" );
  conn_clear_state( c );
  ptpip_open_session( c );
  return true;
}

bool test_initialize( conn_t* c ) {
  printf( "Test initialize\n" );
  conn_clear_state( c );
  ptpip_initialize( c );
  assert( conn_is_waiting_answer( c ) );

  const char* ans = "4400000002000000000000000870b0610a8b4593b2e79357dd36e05058002d00540032000000000000000000000000000000000000000000000000000000000000000000";
  blob_t bans;
  blob_create( &bans, 0, 256 );
  blob_from_hex_string( &bans, 0, ans );
  conn_recv( c, &bans );
  conn_update( c, 1 );
  assert( !conn_is_waiting_answer( c ) );

  return true;
}

bool test_conn() {
  printf( "Testing connections...\n");
  conn_t conn;
  conn_t* c = &conn;
  conn_create( c );
  printf( "Conn created\n");
  conn_update( c, 1 );
  printf( "update complete\n");
  
  if( !test_initialize( c ))
    return false;
  
  if( !test_close_session( c ))
    return false;

  if( !test_open_session( c ))
    return false;

  if( !test_get_storage(c) )
    return false;

  if( !test_get_prop(c) )
    return false;

  if( !test_set_prop(c) )
    return false;

  if( !test_initiate_capture(c) )
    return false;

  if( !test_initiate_open_capture(c) )
    return false;

  if( !test_terminate_capture(c) )
    return false;

  if( !test_del_obj(c) )
    return false;

  if( !test_get_obj(c) )
    return false;

  return true;
}

bool test_channel_accept() {

  channel_t ch_server;
  if( !ch_create( &ch_server, "tcp_server:127.0.0.1", 40000))
    return false;

  while( true ) {
    channel_t ch_client;
    if( ch_accept( &ch_server, &ch_client, 5 * 1000000 ) ) {
      printf( "New connectiong accepted!\n" );
      break;
    }
  }

  ch_close( &ch_server );
  return true;
}

static void show_waiting_answer() {
  char tc[5] = "\\|/-";
  static int idx = 0;
  printf( "\rWaiting answer from the camera... %c ", tc[idx] );
  fflush( stdout );
  idx = ( idx + 1 ) % 4;
}

void wait_until_cmd_processed( conn_t* c) {
  while( conn_is_waiting_answer( c ) ) {
    conn_update( c, 100000 );
    show_waiting_answer();
  }
  printf( "%s\n", ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ) );
}

void set_prop( conn_t* c, const prop_t* prop, uint32_t value ) {
  prop_t p = *prop;
  p.ivalue = value;
  ptpip_set_prop( c, &p );
  wait_until_cmd_processed( c );
  printf( "set_prop( %04x:%s, %04x:%s ) complete %s => %02x\n", p.id, p.name, p.ivalue, prop_get_value_str( &p ), ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ), p.ivalue );
}

bool test_prop_value( ) {
  prop_t p0 = prop_quality;
  p0.ivalue = 3;
  const char* prop_val = prop_get_value_str( &p0 );
  printf( "For prop %s, val 0x%04x is %s\n", p0.name, p0.ivalue, prop_val );
  assert( strcmp( prop_val, "Normal" ) == 0 );
  
  p0 = prop_exposure_time;
  p0.ivalue = PDV_Exposure_Time_5_secs;
  prop_val = prop_get_value_str( &p0 );
  printf( "For prop %s, val 0x%04x is %s\n", p0.name, p0.ivalue, prop_val );
  assert( strcmp( prop_val, "5 secs" ) == 0 );

  return true;
}

bool test_props() {
  test_prop_value();

  bool dump_all_values = true;
  if( dump_all_values ) { 
    const prop_t** props = prop_get_all();
    while( *props ) {
      prop_t p = *(*props++);

      printf( "Prop %04x %s\n", p.id, p.name);
      int n = 0;
      while( prop_get_nth_value( &p, n, &p.ivalue ) ) {
        printf( "  [%2d] %08x %s\n", n, p.ivalue, prop_get_value_str( &p ));
        ++n;
      }
    }
  }

  return false;
}

bool test_channels() {
  if( !test_prop_value() )
    return false;
  return true;
}

extern void download_progress( void* context, uint32_t curr, uint32_t required );

bool take_shot() {
  printf( "Take shot starts...\n");

  camera_info_t camera_info;

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
    return false;

  if( !discovery_start( local_ip ) )
    return false;
  while( !discovery_update( &camera_info, 5 * 1000 ) )
    printf( "Searching camera at %s...\n", local_ip);
  discovery_stop();

  conn_t conn;
  conn_t* c = &conn;
  // set camera info?
  conn_create( c );

  char conn_str[128] = {"tcp:"};
  strcat(conn_str, camera_info.ip);
  printf( "Connecting to >>%s<<\n", conn_str );

  // Connect to the camera using the conn channel
  if( !ch_create( &c->channel, conn_str, camera_info.port) )
    return false;
  printf( "Connected\n" );

  ptpip_initialize( c );
  wait_until_cmd_processed( c );

  ptpip_close_session( c );
  wait_until_cmd_processed( c );

  ptpip_open_session( c );
  wait_until_cmd_processed( c );

  // Via wifi at least this reports 2 storages, one for SDRam of stills, and another for the video preview
  storage_ids_t storage_ids;
  ptpip_get_storage_ids( c, &storage_ids );
  wait_until_cmd_processed( c );
  printf( "%d storages found %s\n", storage_ids.count, ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ) );
  for( int i=0; i<storage_ids.count; ++i ) 
    printf( "    ID:%08x\n", storage_ids.ids[i].id );

  prop_t p = prop_quality;
  ptpip_get_prop( c, &p );
  wait_until_cmd_processed( c );
  printf( "get_prop( %s ) complete %s => %02x (%s)\n", p.name, ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ), p.ivalue, prop_get_value_str( &p ) );

  set_prop( c, &prop_quality, PDV_Quality_Normal);

  p.ivalue = 0xffff;    // Just to confirm it's changing
  ptpip_get_prop( c, &p );
  wait_until_cmd_processed( c );
  assert( p.ivalue == PDV_Quality_Normal );
  printf( "get_prop( %s ) complete %s => %02x (%s)\n", p.name, ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ), p.ivalue, prop_get_value_str( &p ) );

  // Take shoot sequence (without autofocus)
  set_prop( c, &prop_priority_mode, PDV_Priority_Mode_USB);
  
  /*
  bool test_open_capture = false;
  if( test_open_capture ) {
    set_prop( c, &prop_capture_control, PDV_Capture_Control_BulbOn);
    ptpip_initiate_open_capture( c );
    wait_until_cmd_processed( c );

    printf( "Waiting 1 sec");
    ch_wait( 1000 * 1000 );

    printf( "Stop after 1s");
    set_prop( c, &prop_capture_control, PDV_Capture_Control_BulbOff);
    ptpip_terminate_capture( c );
    wait_until_cmd_processed( c );
  }
  else {
    */

  // prop_t p2 = prop_exposure_time;
  // p2.ivalue = 0xffffffff;
  // ptpip_get_prop( c, &p2 );
  // wait_until_cmd_processed( c );
  // printf( "get_prop( %s ) complete %s => %02x (%s)\n", p2.name, ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ), p2.ivalue, prop_get_value_str( &p2 ) );
  
  set_prop( c, &prop_exposure_time, PDV_Exposure_Time_5_secs);
  
  set_prop( c, &prop_capture_control, PDV_Capture_Control_AutoFocus);

  ptpip_initiate_capture( c );
  wait_until_cmd_processed( c );
  
  set_prop( c, &prop_capture_control, PDV_Capture_Control_Shoot);
  ptpip_initiate_capture( c );
  wait_until_cmd_processed( c );
  //}

  // Wait until the take is taken
  prop_t pending_events = prop_pending_events;
  int max_waits = 30;
  while( max_waits-- > 0 ) {
    ptpip_get_prop( c, &pending_events );
    wait_until_cmd_processed( c );
    if( pending_events.ivalue > 0 )
      break;
    ch_wait( 100 * 1000 );  // 100 ms
  }

  set_prop( c, &prop_priority_mode, PDV_Priority_Mode_Camera);

  ptpip_terminate_capture( c );
  wait_until_cmd_processed( c );

  // Images are already saved in the SD Cards (no real need to download)
  // Download images (and remove from the sdram)
  if( storage_ids.count > 0 ) {
    handles_t handles;
    ptpip_get_obj_handles( c, storage_ids.ids[0], &handles );
    wait_until_cmd_processed( c );
    printf( "%d handles found in storage[0]. Complete %s\n", handles.count, ptpip_error_msg( conn_get_last_ptpip_return_code( c ) ) );

    blob_t obj;
    blob_create( &obj, 0, 64 * 1024 );
    char oname[64];

    bool download_images = false;
    bool test_partial_download = false;

    if( download_images ) {
      callback_progress_t my_progress = { .context = NULL, .callback = &download_progress };
      c->on_progress = my_progress;

      for( int i=0; i<handles.count; ++i ) {
        handle_t h = handles.handles[i];
        printf( "Recovering img:%08x\n", h.value );

        // ----------------------------
        ptpip_get_obj( c, h, &obj );
        while( conn_is_waiting_answer( c ) )
          conn_update( c, 1000 );     // 1 ms

        printf( "Img recovered: %d bytes\n", blob_size( &obj ));

        sprintf( oname, "img_%08x.jpg", h.value);
        if( blob_save( &obj, oname ) )
          printf( "Saved file %s\n", oname );

        // ----------------------------
        ptpip_get_partial_obj( c, h, 1024, 4096, &obj );
        while( conn_is_waiting_answer( c ) )
          conn_update( c, 1000 );     // 1 ms

        printf( "block recovered: %d bytes\n", blob_size( &obj ));
        blob_dump( &obj );
        sprintf( oname, "block4k_%08x.jpg", h.value);
        if( blob_save( &obj, oname ) )
          printf( "Saved file %s\n", oname );

        // ----------------------------
        printf( "Recovering thumbnail:%08x\n", h.value );

        ptpip_get_thumbnail( c, h, &obj );
        while( conn_is_waiting_answer( c ) )
          conn_update( c, 1000 );     // 1 ms

        printf( "Thumbnail recovered: %d bytes\n", blob_size( &obj ));

        sprintf( oname, "thumb_%08x.jpg", h.value);
        if( blob_save( &obj, oname ) )
          printf( "Saved file %s\n", oname );

      }

      clear_callback_progress( &c->on_progress );

    } 

    if( test_partial_download ) {

      for( int i=0; i<handles.count; ++i ) {
        handle_t h = handles.handles[i];

        // ----------------------------
        ptpip_get_partial_obj( c, h, 1024, 4096, &obj );
        while( conn_is_waiting_answer( c ) )
          conn_update( c, 1000 );  

        printf( "block recovered: %d bytes\n", blob_size( &obj ));
        sprintf( oname, "block4k_%08x.jpg", h.value);
        if( blob_save( &obj, oname ) )
          printf( "Saved file %s\n", oname );
      }
    }

    blob_destroy( &obj );

    for( int i=0; i<handles.count; ++i ) {
      handle_t h = handles.handles[i];
      printf( "Deleting img from sdram (%08x)\n", h.value );
      ptpip_del_obj( c, h );
      wait_until_cmd_processed( c );
    }

  }

  ptpip_close_session( c );
  wait_until_cmd_processed( c );

  conn_destroy( c );

  printf( "Initialization OK\n" );
  return true;
}
