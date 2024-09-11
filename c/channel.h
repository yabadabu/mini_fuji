#pragma once

typedef struct {
  int fd;
} channel_t;

int ch_open( channel_t* ch, const char* conn_info );
void ch_close( channel_t* ch );
