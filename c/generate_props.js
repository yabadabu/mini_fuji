const fs = require( 'fs' )
const props = require( './property_defs')

const headers = []
const body = []
const body_header = []
const fn = []
const prop_find = []
const registered_ids = {}
const prop_pointers = []

headers.push( "#pragma once" )
headers.push( "" )
headers.push( `#include "connection.h"`)
headers.push( "" )
headers.push( "// This file is autogenerated" )
headers.push( "" )
headers.push( `const char* prop_get_value_str( prop_t* p );` )
headers.push( `const prop_t* prop_by_id( uint16_t prop_id );` )
headers.push( `const prop_t** get_all_props( );` )

body_header.push( `#include "properties.h"`)
body_header.push( "" )
body_header.push( "// This file is autogenerated" )
body_header.push( "" )

prop_pointers.push( 'const prop_t* props[] = {')


function add_extern_decl( name ) {
  headers.push( `\nextern const prop_t prop_${name};` );
}

function add_extern_value( c_name, value ) {
  headers.push( `#define ${c_name}  ${value}` );
}

function add_case( cval, str ) {
  fn.push( `      case ${cval}: return "${str}";` )
}

function export_prop( Name, p ) {
  // Name = Quality
  const NameId = Name.replace( / /g, "_")
  const name = NameId.toLowerCase()
  add_extern_decl( name )

  if( registered_ids[p.id] ) 
    throw( `Property id 0x${p.id} (${Name}) is already registered to property (${registered_ids[p.id]})`)
  registered_ids[ p.id ] = Name;

  const id_cval = `0x${p.id}`;

  body.push( `const prop_t prop_${name} = { .id = ${id_cval}, .name = "${Name}", .data_type = PDT_${p.dt},` )
  body.push( `                              .read_only = ${p.read_only ? true : false}, .ivalue = 0, `)
  body.push( `                              .blob = { .count = 0, .data = 0, .reserved = 0 } };`)

  prop_find.push( `    case 0x${p.id}: return &prop_${name};` );

  prop_pointers.push( `&prop_${name},` )

  if( p.values ) {
    fn.push( `  case ${id_cval}: {  // ${name}\n      switch( p->ivalue ) {`)

    const def_name = `PDV_${NameId}`   // Prop_Exposure_Time_50secs
    add_extern_value( def_name, `0x${p.id}` )

    Object.keys( p.values ).forEach( kval => {
      const Str = p.values[ kval ];
      const str_id = Str.replace( / /g, "" ).replace( /\./g, "_")
      const c_val = `0x${kval}`
      const c_name = `PDV_${NameId}_${str_id}`.padEnd( 32 )   // PDV_Exposure_Time_50secs
      add_extern_value( c_name, c_val )
      add_case( c_name, Str );
    })
    fn.push( `      default: break;`)
    fn.push( `    }`)
    fn.push( `    break;`)
    fn.push( `  }\n`)
  }

}

function save( lines, ofilename ) {
  const contents = lines.join( '\n' )
  console.log( contents )
  fs.writeFileSync( ofilename, contents );
}

function flush() {

  headers.push( "\n" )
  body.push( "\n" )
  console.log( ">>>>>>>>>>>> properties.h")
  save( headers, 'properties.h' )

  console.log( ">>>>>>>>>>>> properties.c")
  body_header.push( `const prop_t* prop_by_id( uint16_t prop_id ) {` )
  body_header.push( `  switch( prop_id ) {` )
  body_header.push( prop_find.join( "\n") )
  body_header.push( "    default: break;" )
  body_header.push( "  }" )
  body_header.push( "  return 0;" )
  body_header.push( "}\n" )

  body_header.push( `const char*   prop_get_value_str( prop_t* p ) {` )
  fn.push( `return "Unknown";\n}\n`)
  body_header.push( fn.join( "\n  " ) )

  prop_pointers.push( "0L   // End of array" )
  body.push( prop_pointers.join( "\n  ") )
  body.push( '};\n')
  body.push( `const prop_t** get_all_props( ) { return props; }` )

  body_header.push( body.join( "\n" ) )

  save( body_header, 'properties.c' )
}

fn.push( `  switch( p->id ) {\n`)
Object.keys( props ).forEach( k => {
  export_prop( k, props[k] )
})
fn.push( `  default: break;\n  }`)

flush();
