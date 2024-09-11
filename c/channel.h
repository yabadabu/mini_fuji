#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int  fd;
  bool is_udp;
  bool is_broadcast;
  bool is_server;
  int  port;
} channel_t;

bool ch_create( channel_t* ch, const char* conn_info );
int  ch_read( channel_t* ch, void *out_buffer, uint32_t buffer_size );
int  ch_write( channel_t* ch, const void *buffer, uint32_t buffer_size );
void ch_close( channel_t* ch );
bool ch_accept( channel_t* ch_server, channel_t* ch_client, int usecs );
int  ch_broadcast( channel_t* ch, const void* msg, uint32_t msg_size );
void ch_wait( int usecs );