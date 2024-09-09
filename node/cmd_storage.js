const fs = require( 'fs' )
const Conn = require( './connection' )
const { storage, formats } = require( './constants' )

function checkEqual( a, b, msg ) {
  if( a !== b ) throw( `${msg}. Values don't match. ${a} !== ${b}` )
}

function cmdGetStorageIds( ) {
  return {
    id : '1004',
    name : 'get_storage_ids',

    create( conn ) {
      return Conn.makeMsg( this.id )
    },

    parse( args ) {
      const count = args.readInt32LE( 0 )
      const ids = []
      for( let idx = 0; idx < count; ++idx )
        ids.push( args.readInt32LE( 4 + idx * 4 ))
      return ids;
    },

    test( ) {
      // Full Ans: 18000000 0200 0410 05000000 020000000100001002000010 0c0000000300012005000000
      const ids = this.parse( Buffer.from( '0200000001000000ffffffff', 'hex' ) )
      checkEqual( ids.length, 2, 'cmdGetStorageIds. Invalid length')
      checkEqual( ids[0], 1, 'cmdGetStorageIds. Invalid idx 0')
      checkEqual( ids[1], -1, 'cmdGetStorageIds. Invalid idx 1')
      const ids2 = this.parse( Buffer.from( '02000000'+'01000010'+'02000010', 'hex' ) )
      console.log( ids2 )
    }
  }
}

function cmdGetStorageInfo( ) {
  return {
    id : '1005',
    name : 'get_storage_info',

    create( conn, storage_id ) {
      this.storage_id = storage_id
      return Conn.makeMsg( this.id, Conn.asLEHexString( storage_id ) )
    },

    parse( args ) {
      //onsole.log( "Parsing cmdGetStorageInfo", args.toString( 'hex' ))
      if( args.length < 16 + 4 + 2 ) {
        return { "error" : `Buffer length is too small`}
      } 

      const storage_type = args.readInt16LE( 0 )
      const filesystem_type = args.readInt16LE( 2 )
      const access = args.readInt16LE( 4 )
      let base = 6
      const max_capacity = args.readBigUInt64LE( base )
      const free_bytes = args.readBigUInt64LE( base + 8 )
      const free_objects = args.readUInt32LE( base + 16 )
      base += 8 * 2 + 4

      const readStr = ( ) => {
        const sz = args.readInt8( base );
        base += 1
        const name = args.subarray( base, base + sz * 2 - 2).toString( 'utf16le' )
        base += sz * 2;
        return name;  
      }

      const description = readStr( )
      const volume_label = readStr( )

      return {
        storage_id,
        storage_type,
        storage_type_name : storage.types[ storage_type ] || "Unknown storage type",
        filesystem_type,
        filesystem_type_name : storage.file_systems[ filesystem_type ] || "Unknown filesystem type",
        access,
        access_name : storage.access[ access ] || "Unknown access type",
        max_capacity,
        free_bytes,
        free_objects,
        description,
        volume_label,
      }
    },

    test( ) {
      // Invalid storage id
      const info1 = this.parse( Buffer.from( '0c000000 0200 0510 06000000 0c0000000300082006000000', 'hex' ) )
      //const info2 = this.parse( Buffer.from( '74000000 0200 0510 07000000 030002000200ffffffffffffffffffffffffffffffffffffffff065300740069006c006c0000002031003500390033003500330033003300310033003000330034003100370030003800320035003700430030003000330030003200310042003100330033000000', 'hex' ))
      //const info3 = this.parse( Buffer.from( '72000000 0200 0510 08000000 030002000200ffffffffffffffffffffffffffffffffffffffff054c00690076006500000020320035003900330035003300330033003100330030003300340031003700300038003200350037004300300030003300300032003100420031003300330000000c0000000300012008000000', 'hex' ))
      const info2 = this.parse( Buffer.from( '030002000200ffffffffffffffffffffffffffffffffffffffff065300740069006c006c0000002031003500390033003500330033003300310033003000330034003100370030003800320035003700430030003000330030003200310042003100330033000000', 'hex' ))
      checkEqual( info2.description, 'Still', 'cmdGetStorageInfo.Desc')
      checkEqual( info2.access, 2, 'cmdGetStorageInfo.Desc')
      
    }
  }
}

function cmdGetNumObjs( ) {
  return {
    id : '1006',
    name : 'get_num_objs',

    create( conn, storage_id, format = 0, parent = -1 ) {
      const b = Buffer.alloc( 3 * 4 )
      b.writeInt32LE( storage_id, 0 )
      b.writeInt32LE( format, 4 )
      b.writeInt32LE( parent, 8 )
      return Conn.makeMsg( this.id, b.toString( 'hex' ) )
    },

    parse( args ) {
      const count = args.readInt32LE(0)
      console.log( "cmdGetNumObjs:", count )
      return { count, args }
    },

    test( ) {
      // After taking a picture
      //const info = this.parse( Buffer.from( '10000000'+'0300'+'0120'+'0b000000'+'02000000', 'hex' ) )
      const info = this.parse( Buffer.from( '10000000030001200b00000002000000', 'hex' ) )
      const d = this.parse( info )
      console.log( d )

    }
  }
}

function cmdGetObjHandles( ) {
  return {
    id : '1007',
    name : 'get_obj_handles',

    create( conn, storage_id, format = 0, parent = -1 ) {
      const b = Buffer.alloc( 3 * 4 )
      b.writeInt32LE( storage_id, 0 )
      b.writeInt32LE( format, 4 )
      b.writeInt32LE( parent, 8 )
      return Conn.makeMsg( this.id, b.toString( 'hex' ) )
    },

    parse( args ) {
      console.log( "cmdGetObjHandles:", args.toString( 'hex' ) )
      const num_handles = args.readInt32LE( 0 )
      const ids = []
      let base = 4;
      for( let idx = 0; idx < num_handles; ++idx ) {
        ids.push( args.subarray(base, base + 4).toString( 'hex' ))
        base += 4
      }
      return ids;
    }
  }
}

function cmdGetObjInfo( ) {
  return {
    id : '1008',
    name : 'get_obj_info',

    create( conn, handle ) {
      return Conn.makeMsg( this.id, handle )
    },

    parse( args ) {
      console.log( "cmdGetObjInfo:", args.toString( 'hex' ) )

      if( args.length < 54 )
        return {}

      let base = 52
      const readStr = ( ) => {
        const sz = args.readInt8( base );
        base += 1
        const name = args.subarray( base, base + sz * 2 - 2).toString( 'utf16le' )
        base += sz * 2;
        return name;  
      }
      const filename = readStr( )
      const creation_date_time = readStr( )
      const mod_date_time = readStr( )

      const ans = {
        storage_id : args.subarray( 0, 4 ), //readInt32LE( 0 ),
        format_hex : Conn.swapInt16( args.subarray( 4, 6 ).toString( 'hex' ) ),
        format : args.readInt16LE( 4 ),
        access : args.readInt16LE( 6 ),
        compressed_size : args.readInt32LE( 8 ),
        thumb_format : args.readInt16LE( 12 ),
        thumb_compressed_size : args.readInt32LE( 14 ),
        thumb_width : args.readInt32LE( 18 ),
        thumb_height : args.readInt32LE( 22 ),
        image_width : args.readInt32LE( 26 ),
        image_height : args.readInt32LE( 30 ),
        image_bit_depth : args.readInt32LE( 34 ),
        parent_obj : args.readInt32LE( 38 ),
        assoc_type : args.readInt32LE( 42 ),
        assoc_desc : args.readInt32LE( 44 ),
        sequence_number : args.readInt32LE( 48 ),
        filename, 
        creation_date_time,
        mod_date_time,
      }

      ans.format_name = formats[ ans.format ] || 'Unknown'
      ans.thumb_format_name = formats[ ans.thumb_format ] || 'Unknown'

      return ans
    },

    test() {

      //  0: 01000010 Storaget id
      //  4: 0138 
      //  6: 0000
      //  8: 878e7400 Comp size
      // 12: 0138
      // 14: 22560000 Thumb size
      // 18: 00000000 Thumb w
      // 22: 00000000 Thumb h
      // 26: 00000000 Img w
      // 30: 00000000 Img h
      // 34: 00000000 Img depth
      // 38: 00000000 Parent
      // 42: 7000     Assoc Type
      // 44: 00000000 Assoc Desc
      // 48: 00000000 Seq Number
      // 52: d440053004300460030003000300034002e006a00700067000000
      //     103200300032003400300039003000370054003100360033003100320038000000
      //     0000

      const sample = Buffer.from( '0100001001380000878e7400013822560000000000000000000000000000000000000000000000000000070000000000000000000d440053004300460030003000300034002e006a007000670000001032003000320034003000390030003700540031003600330031003200380000000000', 'hex' )
      const d = this.parse( sample ); 
      console.log( d )
    }
  }
}

function cmdGetObj( ) {
  return {
    id : '1009',
    name : 'get_obj',

    create( conn, obj_handle, out_name = "output.jpg" ) {
      this.out_name = out_name
      // obj_handle is 4 bytes in string '01000000'
      return Conn.makeMsg( this.id, obj_handle )
    },

    parse( args ) {
      if( args.length > 0 ) {
        console.log( `Saving image ${this.out_name} ${args.length} bytes` )
        fs.writeFileSync( this.out_name, args )
      }
      return { size : args.length }
    }
  }
}

function cmdGetObjSupportedProps( ) {
  return {
    id : '9801',
    name : 'get_obj_supported_props',

    create( conn, obj_handle ) {
      return Conn.makeMsg( this.id, obj_handle )
    },

    parse( args ) {
      console.log( "cmdGetObjSupportedProps:", args.toString( 'hex' ) )
      return { size : args.length }
    }
  }
}

function cmdGetThumbnailObj( ) {
  return {
    id : '100a',
    name : 'get_thumbnail_obj',

    create( conn, handle, out_name = "thumb.jpg" ) {
      this.out_name = out_name
      return Conn.makeMsg( this.id, handle )
    },

    parse( args ) {
      console.log( "cmdGetThumbnailObj:", args.toString( 'hex' ) )
      if( args.length > 0 ) {
        console.log( "Saving image", this.out_name )
        fs.writeFileSync( this.out_name, args )
      }
      return { size : args.length }
    }
  }
}

function cmdDeleteObj( ) {
  return {
    id : '100b',
    name : 'del_obj',
    create( conn, handle ) {
      return Conn.makeMsg( this.id, handle )
    }
  }
}

const cmds = [
  cmdGetStorageIds,
  cmdGetStorageInfo,
  cmdGetNumObjs,
  cmdGetObjHandles,
  cmdGetObjInfo,
  cmdGetObj,
  cmdGetObjSupportedProps,
  cmdGetThumbnailObj,
  cmdDeleteObj,
]

module.exports.getCommands = () => cmds
