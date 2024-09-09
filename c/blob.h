#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t count;
  uint32_t reserved;
  uint8_t* data;
} blob_t;

int blob_create( blob_t* blob, uint32_t num_bytes );
int blob_resize( blob_t* blob, uint32_t new_size );
int blob_clear( blob_t* blob );
int blob_destroy( blob_t* blob );
bool blob_is_valid( const blob_t* blob );
void blob_append( blob_t* blob, const blob_t* other );
void blob_write_u16( blob_t* blob, uint32_t offset, uint16_t value );
void blob_write_u32( blob_t* blob, uint32_t offset, uint32_t value );
uint16_t blob_read_u16( blob_t* blob, uint32_t offset );
uint32_t blob_read_u32( blob_t* blob, uint32_t offset );
void blob_dump( const blob_t* blob );
