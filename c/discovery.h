#pragma once

#include <stdbool.h>

typedef struct {
  int  port;
  char ip[64];
  char name[80];
} camera_info_t;

bool discovery_start( const char* local_ip );
bool discovery_update( camera_info_t* out_camera, int accept_time_msecs );
void discovery_stop( );

