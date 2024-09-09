const Ctes = require( './constants' )
const ProgressBar = require('progress');

function asLEHexString(number) {
    const b_sz = Buffer.alloc( 4 )
    b_sz.writeInt32LE( number )
    return b_sz.toString( 'hex' )
}

function swapInt16( hexString ) {
  const buffer = Buffer.from(hexString, 'hex');
  const reversedBuffer = Buffer.from(buffer.reverse());
  return reversedBuffer.toString('hex');
}

function prefixSize( msg ) {
  const b_sz = Buffer.alloc( 4 )
  b_sz.writeInt32LE( msg.length + 4 )
  return Buffer.concat( [b_sz, msg] )
}

function delay(time) { return new Promise(resolve => setTimeout(resolve, time)); }

const msg_type_cmd  = '0001'
const msg_type_data = '0002'
const msg_type_end  = '0003'
const msg_type_conn = '0005'

// msg_size(4) | num_part(2) | msg_type(2) | msg_idx(4) | payload( N )
function makeMsg( msg_id, payload = '', msg_type = msg_type_cmd, msg_counter = null) {
  if( !msg_counter ) 
    msg_counter = generateMsgCounter();
  const net_msg_type = swapInt16( msg_type )
  const net_msg_id = swapInt16( msg_id )
  const msg = Buffer.from( net_msg_type + net_msg_id + msg_counter + payload, 'hex')
  return prefixSize( msg )
}

let msg_global_counter = 0;
function generateMsgCounter( ) {
  const id = msg_global_counter
  msg_global_counter += 1
  return asLEHexString( id );
}

// ----------------------------------------------------------------
function create( ) {
  const conn = {
    cmd_ids : {},
    otf : null,
    net_trace : false,

    setSockets( new_cmd_sock ) {
      this.sock = new_cmd_sock
      new_cmd_sock.on('data', (data)=>{
        if( this.net_trace )
          console.log( "Sock.Read", data.toString("hex"))
        this.pending_buffer = Buffer.concat( [this.pending_buffer, data] )
        this.parseNetData( data )
      })
    },

    send( buffer ) {
      if( !buffer ) throw( "create.send an emtpy buffer!")

      if( !this.sock ) throw( "create.send. this.sock is still invalid")
      if( this.net_trace )
        console.log( "Sock.Write", buffer.toString("hex"))
      this.sock.write( buffer )
    },

    addModule( mod_name ) {
      const mod = require( "./" + mod_name)
      const cmds = mod.getCommands()
      cmds.forEach( async cmd_fn => {
        const cmd = cmd_fn()
        const cmd_id = cmd.id
        const cmd_name = cmd.name

        const net_cmd_id = swapInt16( cmd_id )
        this.cmd_ids[ net_cmd_id ] = cmd

        // For the get_dev_info output
        Ctes.cmds[ cmd_id ] = cmd_name

        this[ cmd.name ] = ( ...args ) => {
          if( this.otf ) throw( "otf should be null")
          const msg = cmd.create( this, ...args )
          console.log( `Send ${cmd.name}(`, ...args, ') =>', msg )
          let task = null

          // send...
          this.otf = { cmd }
          task = new Promise((resolve, reject) => {
            this.otf.resolve = resolve
            this.otf.reject = reject
          })

          if( Array.isArray( msg ) ) {
            msg.forEach( m => this.send( m ) )
          } else {
            this.send( msg )
          }

          return task
        }
      })
    },

    pending_buffer : Buffer.alloc(0),
    bar : null,
    parseNetData( ) {
      while( this.pending_buffer.length >= 8 ) {
        const remaining_bytes = this.pending_buffer.length
        const sz = this.pending_buffer.readInt32LE( 0 )
        //console.log( `There are ${remaining_bytes} and we need ${sz}`)
        if( sz > remaining_bytes ) {

          // Show a progress bar?
          if( sz > 4 * 1024 ) {
            if( !this.bar ) {
              this.bar = new ProgressBar('  downloading [:bar] :rate/bps :percent :etas', {
                                          complete: '=',
                                          incomplete: ' ',
                                          width: 50,
                                          total: sz
                                        });
            }
            const ratio = ( remaining_bytes + 0.0) / sz;
            this.bar.update( ratio )
          }

          //console.log( `Incomming ptp msg requires ${sz} bytes, but we only have ${remaining_bytes}`)
          break
        }

        const ptp = this.pending_buffer.slice( 0, sz )
        this.parsePTPPacket( ptp )

        this.pending_buffer = this.pending_buffer.slice( sz )
      }  
    },

    finishOTF() {
        if( this.otf.resolve ) {
          const resolve = this.otf.resolve
          const answer = this.otf.result
          this.otf = null
          //console.log( "OTF is now null")
          if( this.bar )
            this.bar.terminate()
          this.bar = null
          resolve( answer )
        }      
    },

    parsePTPPacket( data ) {

      // Each packet is preceeded by the size (4 bytes)
      const sz           = data.readInt32LE( 0 )
      const net_msg_type = data.subarray( 4, 6 ).toString( 'hex' ).toUpperCase() // Data/End
      const net_arg      = data.subarray( 6, 8 ).toString( 'hex' ).toUpperCase()
      const msg_type     = swapInt16( net_msg_type )
      const arg          = swapInt16( net_arg )
      
      //console.log( `Parse PTP packet. Sz:${sz} Type:${msg_type} Arg:${arg}`, data )
      if( !this.otf ) throw( "Recv ptp packet but no otf packet" )

      if( msg_type == msg_type_data ) {
        const host_msg_idx = data.readInt32LE( 8 )
        const args_size = sz - 12;
        const args = ( args_size > 0 ) ? data.subarray( 12, 12 + args_size ) : Buffer.alloc(0)
        const cmd_id = arg
        //if( cmd_id != this.otf.cmd.id ) throw( `Expecting data msg to be associated to the cmd id. ${cmd_id} vs ${this.otf.cmd.id}` )
        this.otf.result = this.otf.cmd.parse( args )

        if( cmd_id == '0000' ) {
          this.otf.result.conn_id = host_msg_idx
          this.finishOTF();
        }
      }

      else if( msg_type == msg_type_end ) {
        const host_msg_idx = data.readInt32LE( 8 )
        const rc_code_name = Ctes.return_codes[ arg ] || "Unknown rc code"
        console.log( "Recv end of host cmd idx", host_msg_idx, this.otf.name, rc_code_name )
        const args_size = sz - 12;
        // In some cmds there is some extra data at the end of the end_msg
        if( args_size > 0 ) { 
          const args = ( args_size > 0 ) ? data.subarray( 12, 12 + args_size ) : Buffer.alloc(0)
          this.otf.result = this.otf.cmd.parse( args )
        }
        this.finishOTF();
      }

      else if( msg_type == msg_type_conn ) {
        const net_rc_code = data.subarray( 8, 8 + 2 )
        const args =  data.subarray( 10 )
        const rc_code = swapInt16( net_rc_code )
        const rc_code_name = Ctes.return_codes[ rc_code ] || "Unknown rc code"

        console.log( `Init Rc = ${rc_code} (${rc_code_name}) Packet as expected. Args:`, args.toString( 'hex' ),)
        this.otf.result = { rc_code, args }
        this.finishOTF();
      }

    },

  }

  return conn
}

module.exports.makeMsg = makeMsg
module.exports.swapInt16 = swapInt16
module.exports.asLEHexString = asLEHexString
module.exports.generateMsgCounter = generateMsgCounter
module.exports.delay = delay

module.exports.create = create



