const fs = require( 'fs' )

const props = {
  Quality : {
    id : 'd018',
    dt : 'U16',
    values : {
      '0001' : 'Raw',
      '0002' : 'Fine',
      '0003' : 'Normal',
      '0004' : 'Fine Raw',
      '0005' : 'Normal Raw',
    }
  },
  
  "Priority Mode" : {
    id : 'd207',
    dt : 'U16',
    values : {
      '0001' : 'Camera',
      '0002' : 'USB',
    }
  },

  "Capture Control" : {
    id : 'd208',
    dt : 'U16',
    values : {
      '0200' : 'AutoFocus',
      '0304' : 'Shoot',
      //'0500' : 'Bulb On',
      //'000c' : 'Bulb Off',
    }
  },

  "Pending Events" : {      // CurrentState in other sources
    id : 'd212',
    dt : 'ARRAY_PROP_VALUES',
    read_only : true,
  },

  "Pre Capture Delay" : {      // CurrentState in other sources
    id : '5012',
    dt : 'U16',
    values : {
      '07d0' : '10 Secs',
      '2710' : '2 Secs',
      '0000' : 'Off',
    }
  },

  "Focus Mode" : {      // CurrentState in other sources
    id : '500a',
    dt : 'U16',
    values : {
      '0001' : 'Manual',
      '8001' : 'Single Auto',
      '8002' : 'Continuous Auto',
    }
  },

  "Exposure Time" : {
    id : '500d',
    dt : 'U32',
    //factory_val: 'f4000000',
    values : {
      /*
      '7a000000', '99000000', 'c1000000', 'f4000000',
      '33010000', '83010000', 'e8010000', '67020000',
      '07030000', 'd0030000', 'ce040000', '0e060000',
      'a1070000', '9c090000', '1c0c0000', '420f0000',
      '39130000', '38180000', '841e0000', 
      '00002673' : "1/100s", /// OK
      '71300000', '093d0000', 'e64c0000', 'e3600000',
      '127a0000', 'cc990000', 'c6c10000', '24f40000',
      '99330100', '8c830100', '48e80100', '32670200',
      '19070300', '90d00300', '64ce0400', '320e0600',
      //2.5, 3.4,5,6,8,,10,13, 
      '20a10700' : "1/2s", 
      'c89c0900', 
      '641c0c00', 
      '40420f00' : "1 Sec",
      '91391300' : "1.3 Secs"
      'c9381800' : "1.5 Secs"
      '80841e00' : "2 Secs"
      '22732600' : "2.5 Secs"
      '92713000' : "3 Secs"
      '00093d00' : "4 Secs"
      */
      '004ce644' : "5 secs",    // <<< This is host .   network is 44 e6 4c 00
      '0060e324' : "6.5 secs",
      '007a1200' : "8 secs",
      '0099cc88' : "10 secs",
      '00c1c648' : "13 secs",
      '00f42400' : "15 secs",
      '01339910' : "20 secs",
      '01838c90' : "25 secs",
      '01e84800' : "30 secs",
      '02673221' : "40 secs",
      '03071921' : "50 secs",
      '03d09000' : "60 secs",
      '03d0901e' : "2 mins",
      '03d0903c' : "4 mins",
      '03d0905a' : "8 mins",
      '03d09078' : "15 mins",
      //'ffffffff'
      /*
      enums: [
    '7a000000', '99000000', 'c1000000', 'f4000000',
    '33010000', '83010000', 'e8010000', '67020000',
    '07030000', 'd0030000', 'ce040000', '0e060000',
    'a1070000', '9c090000', '1c0c0000', '420f0000',
    '39130000', '38180000', '841e0000', '73260000',
    '71300000', '093d0000', 'e64c0000', 'e3600000',
    '127a0000', 'cc990000', 'c6c10000', '24f40000',
    '99330100', '8c830100', '48e80100', '32670200',
    '19070300', '90d00300', '64ce0400', '320e0600',
    '20a10700', 'c89c0900', '641c0c00', '40420f00',
    '91391300', 'c9381800', '80841e00', '22732600',
    '92713000', '00093d00', '44e64c00', '24e36000',
    '00127a00', '88cc9900', '48c6c100', '0024f400',
    '10993301', '908c8301', '0048e801', '21326702',
    '21190703', '0090d003', '1e90d003', '3c90d003',
    '5a90d003', '7890d003', 'ffffffff'
*/
    }
  },

}

function swapInt16( hexString ) {
  const buffer = Buffer.from(hexString, 'hex');
  const reversedBuffer = Buffer.from(buffer.reverse());
  return reversedBuffer.toString('hex');
}

const headers = []
const body = []
const body_header = []
const fn = []
const prop_find = []
const registered_ids = {}

headers.push( "#pragma once" )
headers.push( "" )
headers.push( `#include "connection.h"`)
headers.push( "" )
headers.push( "// This file is autogenerated" )
headers.push( "" )
headers.push( `const char* prop_get_value_str( prop_t* p );` )
headers.push( `const prop_t* prop_by_id( uint16_t prop_id );` )

body_header.push( `#include "properties.h"`)
body_header.push( "" )
body_header.push( "// This file is autogenerated" )
body_header.push( "" )

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

  body.push( `const prop_t prop_${name} = { .id = 0x${p.id}, .name = "${Name}", .data_type = PDT_${p.dt},` )
  body.push( `                              .read_only = ${p.read_only ? true : false}, .ivalue = 0, `)
  body.push( `                              .blob = { .count = 0, .data = 0, .reserved = 0 } };`)

  prop_find.push( `    case 0x${p.id}: return &prop_${name};` );

  if( p.values ) {
    fn.push( `  case 0x${p.id}: {  // ${name}\n      switch( p->ivalue ) {`)

    Object.keys( p.values ).forEach( kval => {
      const Str = p.values[ kval ];
      const str_id = Str.replace( / /g, "" ).replace( /\./g, "_")
      const c_val = `0x${kval}`
      const c_name = `PDV_${NameId}_${str_id}`.padEnd( 32 )
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
  fs.writeFileSync( ofilename, contents );
}

function flush() {

  headers.push( "\n" )
  body.push( "\n" )
  console.log( ">>>>>>>>>>>> properties.h")
  console.log( headers.join( "\n" ))
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
  body_header.push( body.join( "\n" ) )
  console.log( body_header.join( "\n" ))
  save( body_header, 'properties.c' )
}

fn.push( `  switch( p->id ) {\n`)
Object.keys( props ).forEach( k => {
  export_prop( k, props[k] )
})
fn.push( `  default: break;\n  }`)

flush();
