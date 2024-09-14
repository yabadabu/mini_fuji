#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t count;
  uint32_t reserved;
  uint8_t* data;
} blob_t;

void blob_create( blob_t* blob, uint32_t num_bytes, uint32_t bytes_to_reserve );
void blob_reserve( blob_t* blob, uint32_t num_bytes );
void blob_resize( blob_t* blob, uint32_t new_size );
void blob_clear( blob_t* blob );
void blob_destroy( blob_t* blob );
bool blob_is_valid( const blob_t* blob );
void blob_append_blob( blob_t* blob, const blob_t* other );
void blob_append_data( blob_t* blob, const void* new_data, uint32_t data_size );
void blob_remove_from_start( blob_t* blob, uint32_t num_bytes, blob_t* out_blob );
void blob_write_u16le( blob_t* blob, uint32_t offset, uint16_t value );
void blob_write_u32le( blob_t* blob, uint32_t offset, uint32_t value );
uint32_t blob_size( const blob_t *blob );
uint16_t blob_read_u16le( const blob_t* blob, uint32_t offset );
uint32_t blob_read_u32le( const blob_t* blob, uint32_t offset );
void blob_dump( const blob_t* blob );
void blob_from_hex_string( blob_t* blob, uint32_t offset, const char* hex_str );
bool blob_save( const blob_t* blob, const char* ofilename );
bool blob_load( blob_t* blob, const char* ifilename );
void blob_view( blob_t* blob, blob_t* source, uint32_t offset, uint32_t size );
bool blob_match( const blob_t* b1, const blob_t* b2 );
