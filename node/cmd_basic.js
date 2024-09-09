const Conn = require( './connection' )
const Ctes = require( './constants' )

function cmdInitComm( ) {
  return {
    id : '0000',
    name : 'init_command',
    create( conn, client_name ) {

      /*
      //const b_msg = Buffer.from("01000000"+protocol_version+"87b27f0b"+"d3d5ded0" + "0278" + "a8c0", "hex")
      const b_msg = Buffer.from("01000000"+protocol_version+client_id+"d3d5ded0" + "0278" + "a8c0", "hex")
      const b_name = Buffer.from( name + "\0", 'utf16le' )
      //const b_version = Buffer.from( "00000000", "hex" )  // V: 1.0
      const b_padding = Buffer.alloc( 54 - b_name.length )
      //return Buffer.concat( [b_msg, b_name, b_padding] )
      return prefixSize( Buffer.concat( [b_msg, b_name, b_padding] ) )
      */
      const client_id = "ada5485d87b27f0b"  
      const magic = 'd3d5ded0'
      const payload = client_id + magic + 'fe7fa8c04a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000';
      const protocol_version = "f2e4538f"   // Byte order changed from go version
      return Conn.makeMsg( this.id, payload, '0001', protocol_version)
      // 52000000 Size of the msg
      // 01000000 Init Cmd Request Packet
      // f2e4538f ada5485d 87b27f0bd3d5ded0  Client GUID
      // < Host Name as string >
      // < Version as 4 bytes >
    },

    parse( data ) {
      console.log( "Parsing cmdInitComm", data )
      let base = 0
      const guid = data.subarray( base, base + 16 ).toString( 'hex' )
      base += 16
      const name = data.subarray( base ).toString( 'utf16le' ).replace(/\0+$/, '');
      return { guid, name }
    }

    // we send: 5200000001000000f2e4538fada5485d87b27f0bd3d5ded0fe7fa8c04a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000
    // we recv: 4400000002000000000000000870b0610a8b4593b2e79357dd36e05058002d00540032000000000000000000000000000000000000000000000000000000000000000000
  }
}

// function cmdInitEventComm( ) {
//   return {
//     id : '0000',
//     name : 'init_events',
//     use_event_channel : true,
//     create( conn, conn_id ) {
//       const client_id = "ada5485d87b27f0b"  
//       const magic = 'd3d5ded0'
//       const payload = client_id + magic + 'fe7fa8c04a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000';
//       return Conn.makeMsg( this.id, payload, '0003', '00000000')
//     },

//     parse( data ) {
//       console.log( "Parsing cmdInitEventComm", data )
//     }

//     // we send: 5200000001000000f2e4538fada5485d87b27f0bd3d5ded0fe7fa8c04a00750061006e0073002d004d006100630042006f006f006b002d00500072006f000000000000000000000000000000000000000000
//     // we recv: 4400000002000000000000000870b0610a8b4593b2e79357dd36e05058002d00540032000000000000000000000000000000000000000000000000000000000000000000
//   }
// }

function cmdDeviceInfo( ) {
  return {
    id : '1001',
    name : 'get_dev_info',
    create( conn ) {
      return Conn.makeMsg( this.id )
    },

    parse( args ) {
      let base = 8
      
      const readStr = ( ) => {
        const sz = args.readInt8( base );
        base += 1
        const name = args.subarray( base, base + sz * 2).toString( 'utf16le' ).replace(/\0+$/, '');
        base += sz * 2;
        return name;  
      }

      const readAttrs = ( prefix ) => {
        const num_ids = args.readInt32LE( base );
        base += 4
        const vals = {}
        for( let i=0; i<num_ids; ++i ) {
          const id = args.readUInt16LE( base );
          const ids = id.toString( 16 );
          base += 2

          if( prefix == 'Prop' ) {
            vals[ ids ] = Ctes.props[ ids ] || 'Unknown Prop'

          } else if( prefix == 'Event' ) {
            vals[ ids ] = Ctes.events[ ids ] || 'Unknown Event'

          } else if( prefix == 'Cmd' ) {
            vals[ ids ] = Ctes.cmds[ ids ] || 'Unknown Cmd'

          } else if( prefix == 'Format' ) {
            vals[ ids ] = Ctes.formats[ ids ] || 'Unknown format'

          }
        }
        return vals
      }

      const name = readStr();
      base += 2;  // ptpip protocol version???
      const cmds = readAttrs( 'Cmd' )
      const evts = readAttrs( 'Event' )
      const props = readAttrs( 'Prop' )
      const fmts  = readAttrs( 'Format' )
      const img = readAttrs( 'Img' )
      const manufacturer = readStr();
      const model = readStr();
      const firmware_version = readStr();
      const serial_number = readStr();
      return {
        cmds, evts, props, fmts,
        manufacturer,
        model,
        firmware_version,
        serial_number,
      }
    }
  }
}

function cmdOpenSession( ) {
  return {
    id : '1002',
    name : 'open_session',
    create( conn ) {
      return Conn.makeMsg( this.id, '01000000' )
    }
  }
}

function cmdCloseSession( ) {
  return {
    id : '1003',
    name : 'close_session',
    create( conn ){
      return Conn.makeMsg( this.id )
    }
  }
}

function cmdInitiateCapture( ) {
  return {
    id : '100e',
    name : 'initiate_capture',
    create( conn ){
      return Conn.makeMsg( this.id, '00000000' + '00000000' )
    }
  }
}

function cmdInitiateOpenCapture( ) {
  return {
    id : '101c',
    name : 'initiate_open_capture',
    create( conn ){
      return Conn.makeMsg( this.id, '00000000' + '00000000' )
    }
  }
}

function cmdGetPartialObj( ) {
  return {
    id : '101b',
    name : 'get_partial_obj',
    create( conn, handle, offset, size ) {
      const b = Buffer.alloc( 8 )
      b.writeInt32LE( offset, 0 )
      b.writeInt32LE( size, 4 )
      return Conn.makeMsg( this.id, handle + b.toString( 'hex' ) )
    }
  }
}

function cmdTerminateOpenCapture( ) {
  return {
    id : '1018',
    name : 'terminate_open_capture',
    create( conn ) {
      return Conn.makeMsg( this.id )
    }
  }
}

function cmdRaw( ) {
  return {
    id : 'ffff',
    name : 'raw',

    create( conn, custom_id, payload ) {
      return Conn.makeMsg( custom_id, payload )
    },

    parse( args ) {
      const data = args.toString( 'hex' )
      console.log( "cmdRaw:", data )
      return { 
        size : args.length, 
        args, 
        data
      }
    }
  }
}

const cmds = [
  cmdInitComm,
  //cmdInitEventComm,
  cmdOpenSession,
  cmdCloseSession,
  cmdDeviceInfo,
  cmdInitiateCapture,
  cmdInitiateOpenCapture,
  cmdGetPartialObj,
  cmdTerminateOpenCapture,
  cmdGetPartialObj,
  cmdRaw,
]

module.exports.getCommands = () => cmds
