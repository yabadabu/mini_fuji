#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "blob.h" 
#include "connection.h" 

// -------------------
bool test_blobs();
bool test_conn();

int main( int argc, char** argv ) {
  if( !test_blobs() )
    return -1;
  if( !test_conn() )
    return -1;
  printf( "OK\n");
  return 0;
}

