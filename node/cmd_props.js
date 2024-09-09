const Conn = require( './connection' )
const Ctes = require( './constants' )
const { format } = require( 'util' )
const { data_types, props } = Ctes

function dataTypeValueSize( data_type ) {
  const type_name = data_types[ data_type ] 
  if( !type_name ) {
    console.error( "Data type", data_type, "not supported")
    return 0
  }
  if( type_name.match( "8$" ) )
    return 1
  if( type_name.match( "16$" ) )
    return 2
  if( type_name.match( "32$" ) )
    return 4
  if( type_name.match( "64$" ) )
    return 8
  return 0
}

function cmdGetPropInfo( ) {
  return {
    id : '1014',
    name : 'get_prop_info',

    create( conn, prop_id ) {
      const net_prop_id = Conn.swapInt16( prop_id )
      return Conn.makeMsg( this.id, net_prop_id + '0000' )
    },

    parse( args ) {

      if( args.length == 0 ) {
        console.log( "cmdGetPropInfo.parse.failed.", args, args.toString('hex'))
        return;
      }

      // 0350 ffff 01 / 0a:36003000300030007800340030003000300000000a36003000300030007800340030003000300000000209000a33003000300038007800320030003000300000000a33003000300038007800310036003800380000000a32003000300030007800320030003000300000000a34003200340030007800320038003300320000000a34003200340030007800320033003800340000000a32003800330032007800320038003300320000000a36003000300030007800340030003000300000000a36003000300030007800330033003700360000000a3400300030003000780034003000300030000000

      // 0550 0400 01 / factory 0200 / current 0200 / form 02 / (num_vals) 0c00 
      // N Values: 0200 0400 0600 0800 0180 0280 0380 0680 0780 0880 0980 0a80
      let base = 0
      const prop_id = args.readUInt16LE( base );
      base += 2
      const prop_data_type = args.readUInt16LE( base );
      const prop_data_type_name = data_types[ prop_data_type ]
      base += 2
      const prop_get_set = args.readUInt8( base );
      base += 1

      const bytes_per_value = dataTypeValueSize( prop_data_type )

      const readStr = ( ) => {
        const sz = args.readInt8( base ); //.subarray( 12, 16 )
        base += 1
        const name = args.subarray( base, base + sz * 2 - 2).toString( 'utf16le' )
        base += sz * 2;
        return name;  
      }

      const readVal = () => {
        if( prop_data_type == 65535 ) {
          return readStr()
        } 
        const val = args.subarray( base, base + bytes_per_value )
        base += bytes_per_value
        return val.toString( 'hex' )
      }

      const host_prop_id = prop_id.toString( 16 )
      const prop_name = props[ host_prop_id ] || 'Unknown prop id'

      const ans = {
        prop_id : host_prop_id, 
        prop_name,
        type_name : prop_data_type_name,
        bytes_per_value 
      }

      // Read factory..
      ans.factory_val = readVal( )
      ans.curr_val = readVal( )

      if( base >= args.length )
        return;

      // Form flag
      const form_flag = args.readUInt8( base );
      base += 1

      if( prop_get_set == 1 ) {
        ans.can_write = true
        ans.access_name = "(Read/Write)"
      }
      else
        ans.access_name = "(Read Only)"

      // console.log( `  FactoryValue:`, ans.factory_val.toString('hex'))
      // console.log( `  CurrentValue:`, ans.curr_val.toString('hex'))

      if( form_flag == 2 ) { // Enum
        // Num values
        ans.num_enums = args.readUInt16LE( base ); base += 2
//        console.log( num_enums, "enum values" )
        ans.enums = []
        for( let i=0; i<ans.num_enums; ++i ) {
          const val = readVal()
          ans.enums.push( val )
        }
      } else if( form_flag == 1 ) { // range
        ans.ranged = true
        // Read min 
        // Read max 
        // Read min step 

      } else if( form_flag == 0 ) {
        console.log( "form none" )

      }  

      return ans
    }
  }
}

function cmdGetPropValue( ) {
  return {
    id : '1015',
    name : 'get_prop',

    create( conn, prop_id ) {
      this.prop_id = prop_id
      const net_prop_id = Conn.swapInt16( prop_id )
      return Conn.makeMsg( this.id, net_prop_id + '0000' )
    },

    parse( data ) {

      const ans = {
        bytes : data,
        data : data.toString( 'hex' ),
      }

      if( this.prop_id == Ctes.DPC_CurrentState ) {
        const num_events = data.readInt16LE(0)
        ans.num_events = num_events
        ans.changes = []
        let base = 2
        for( let idx = 0; idx < num_events; ++idx ) {
          const prop_id = Conn.swapInt16(data.subarray(base, base + 2).toString( 'hex' ))
          const prop_name = props[ prop_id ] || 'Unknown prop'
          const value = data.readInt16LE(base + 2)
          base += 6
          ans.changes.push( { prop_id, prop_name, value } )
        }
        
      } else if (this.prop_id == Ctes.DPC_ShotCount) {
        ans.shot_count = data.readInt32LE(0)
      }

      return ans;

    },

    test( ) {
      this.prop_id = Ctes.DPC_CurrentState
      const ans = this.parse( new Buffer.from( '01000ed20c000000', 'hex' ))
      console.log( ans )
    }
  }
}

function cmdResetPropValue( ) {
  return {
    id : '1017',
    name : 'reset_prop',
    create( conn, prop_id ) {
      this.prop_id = prop_id
      const net_prop_id = Conn.swapInt16( prop_id )
      return Conn.makeMsg( this.id, net_prop_id + '0000' )
    },
    parse( data ) { }
  }
}

function cmdSetPropValue( ) {
  return {
    id : '1016',
    name : 'set_prop',

    create( conn, prop_id, prop_values ) {
      if( prop_id.length != 4 ) throw( "setPropValue: prop_id must be 2 bytes")
      const net_prop_id = Conn.swapInt16( prop_id ) + '0000'
      const msg_counter = Conn.generateMsgCounter();
      const msg_idx = Conn.asLEHexString( msg_counter )
      const msg1 = Conn.makeMsg( this.id, net_prop_id, '0001', msg_counter)
      const msg2 = Conn.makeMsg( this.id, prop_values, '0002', msg_counter)
      return [msg1, msg2]
    },

    parse( data ) {
      return {
        bytes : data
      }
    }
  }
}


const cmds = [
  cmdGetPropInfo,
  cmdGetPropValue,
  cmdSetPropValue,
  cmdResetPropValue,
]

module.exports.getCommands = () => cmds
