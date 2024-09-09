// https://github.com/malc0mn/ptp-ip/blob/10de86c522f5b53fbe84872c66acf38665d02a26/ip/packets_fuji.go#L219

const repl = require('node:repl')
const fs = require('fs')

const net = require('net')
const cfg = require('./config')
const dgram = require('dgram')

let interval_handle = 0
let udp_sock
function startDiscoveryMsg() {
  const str = `DISCOVERY * HTTP/1.1\r\nHOST: ${cfg.host.ip}\r\nMX: 5\r\nSERVICE: PCSS/1.0\r\n`
  const message = Buffer.from( str )
  udp_sock = dgram.createSocket('udp4')
  udp_sock.bind(() => {
    udp_sock.setBroadcast(true);
    interval_handle = setInterval( ()=> {
      console.log( "Sending UDP msg")
      udp_sock.send(message, 0, message.length, cfg.camera.udp_port, '255.255.255.255')
    }, 1000)
  });
}

function stopDiscovery() {
  clearInterval( interval_handle )
  udp_sock.close()
}

async function startTCPServer( port ) {
  return new Promise( (accept, reject)=> {

    const parseCameraDiscoveryMsg = ( msg ) => {
      const lines = msg.split( "\n" )
      if( lines.length < 1 || lines[0] != 'NOTIFY * HTTP/1.1\r')
        return {}
      const camera = {}
      lines.forEach( line => {
        const re = line.match( /(\w+): (.*)\r/)
        if( re ) {
          const key = re[1]
          const value = re[2]
          camera[ key ] = value
        }
      })
      return camera
    }

    // The camera connects to the sender of the udp msg
    // And sends his ip/port using html protocol
    server = net.createServer((socket) => {
      console.log('TCP.Client connected')
      stopDiscovery();

      // Handle client disconnection
      socket.on('end', () => {
          console.log('TCP.Client disconnected');
      });

      // Handle errors
      socket.on('error', (err) => {
          console.error(`Socket error: ${err.message}`);
      });

      // Handle incoming data from the client
      socket.on('data', (data) => {
        socket.write( 'HTTP/1.1 200 OK\r\n' );
        const msg = new Buffer.from(data).toString()
        const camera = parseCameraDiscoveryMsg( msg )
        socket.end()
        accept( camera )
      });
    })

    // Start listening on the specified port
    server.listen(port, () => {
      console.log(`TCP.Server listening on port ${port}`);
      startDiscoveryMsg()
    });

      // Handle server errors
    server.on('error', (err) => {
      console.error(`TCP.Server error: ${err.message}`);
      server.close();
    });
  })

}

const Conn = require( "./connection")
const ctes = require( "./constants")

const fake_sock = {
  handlers : {},
  on( evt, fn ) {
    this.handlers[ evt ] = fn
  },
  sent : 0,
  write( data, ans ) {
    console.log( "Fake.Net.Sending", data.toString("hex"))
    setTimeout( ()=> { 
      //const ans = Buffer.from( '170000000200141038000000', 'hex')
      //const ans_get_prop = Buffer.from( '0e000000020015100a0000000400' + '0c000000030001200a000000', 'hex' )
      //const ans_set_prop = Buffer.from( '0c000000030001200a000000', 'hex' )
      if( ans )
        this.recv( ans ) 
    }, 1000)
  },
  recv( data ) {
    this.handlers[ 'data' ]( data )
  }
}

const c = Conn.create();
c.addModule( 'cmd_basic' )
c.addModule( 'cmd_props' )
c.addModule( 'cmd_storage' )

async function runTest( c ) {
  c.setSocket( fake_sock )
  
  //const reg_ans = Buffer.from( '5d0200000200011002000000640006000000640016660075006a006900660069006c006d002e0063006f002e006a0070003a00200031002e0030003b002000000000001b0000000110021003100410051006100710081009100a100b100e100f10141015101610171018101b101c100c900d901d9001980298039805980600000002400340044005400d4006c0a80000000150035005500a500b500c500e500f50115012501550185019501c5001d002d003d004d005d007d008d009d00ad00bd00cd00dd00ed00fd010d011d012d013d014d015d016d017d018d019d01ad01bd01cd000d101d102d103d104d105d106d107d108d109d10ad10bd10cd10dd10ed10fd110d111d112d113d114d115d116d117d118d119d11ad11bd11cd11dd11ed11fd120d121d122d123d124d125d126d127d128d129d12ad12bd12cd12dd12ed12fd130d131d132d133d134d135d136d137d138d139d13ad13bd13cd13dd13ed13fd140d141d142d143d144d145d146d147d148d149d14ad14bd14cd14dd14ed14fd150d151d152d153d154d155d157d158d159d15ad15bd15cd15dd15ed15fd160d161d100d201d202d203d204d205d206d207d208d209d20ad20bd20cd20dd20ed20fd210d211d212d213d214d215d216d217d218d219d21ad21bd206d407d4030000000038013803b1030000000038013803b109460055004a004900460049004c004d0000000558002d005400320000000534002e003400300000001f3500390033003500330033003300310033003000330034003100370030003800320035003700430030003000330030003200310042003100330033000000', 'hex' )
  await c.init_comm( 'iPhone' )

  //const prop1 = await c.get_prop( '1122' )
  //const prop2 = await c.get_prop_desc( '1133' )
  //const res = await c.open_session( )
  //const prop1 = await c.set_prop( '1122', '0100' )
  console.log( "Complete" )
}

async function connectToCamera( cam ) {

  return new Promise( (accept, reject)=>{
    const cam_port = parseInt(cam.DSCPORT)
    const cam_ip = cam.DSC;
    console.log( "Connection with camera", cam)

    // Establish TCP connection to the camera
    const sock = new net.Socket();
    const sock_evt = new net.Socket();
    c.setSockets( sock, sock_evt )

    sock.connect(cam_port, cam_ip, async () => {
      let conn_id = 0;
      console.log(`Connected to camera`);
      while( true ) {
        console.log( "Waiting 100ms")
        await Conn.delay( 50 )
        console.log( "Sending init_command")
        const res = await c.init_command( 'iPhone' )
        console.log(`init_command res is`, res)
        if( res.rc_code != '2019' ) {
          conn_id = res.conn_id
          break
        }
      }

      console.log(`Init Complete. ConnId is ${conn_id}`)

      // console.log(`Retrying Complete`);
      //sock_evt.connect(cam_port, cam_ip, async () => {
        //await 
        //c.init_events( conn_id )
        accept()
      //})
    })
  })
}

async function testTakeImage( ) {

  const cam = await startTCPServer( cfg.host.port )

  await connectToCamera( cam )

  await c.close_session( )
  
  await c.open_session( )

  await c.set_prop( ctes.DPC_Quality, '0300' )

  // const DPC_WhiteBalance = ctes.DPC_WhiteBalance
  // const Info_WhiteBalance = await c.get_prop_info( DPC_WhiteBalance )
  // console.log( "WhiteBalance.Info is", Info_WhiteBalance )
  // const WhiteBalance = await c.get_prop( DPC_WhiteBalance )
  // console.log( "WhiteBalance.Value is", WhiteBalance )
  // await c.set_prop( DPC_WhiteBalance, '0200' )

  const rs = repl.start( { prompt:'> ' })
  const ctx = rs.context

  // Load the history if the file exists
  const historyFile = '.repl_history';
  if (fs.existsSync(historyFile)) {
    fs.readFileSync(historyFile, 'utf-8')
      .split('\n')
      .reverse()
      .filter(line => line.trim())
      .forEach(line => rs.history.push(line));
  }

  // Save the history on exit
  rs.on('line', () => {
    fs.writeFileSync(historyFile, rs.history.slice(-1000).join('\n'));
  });

  ctx.c = c
  ctx.ctes = ctes
  
  ctx.take = async () => {

    console.log( "Taking a shot..." )
 //   await c.set_prop( ctes.DPC_AutoFocusStatus, '0100' )  // This is read-only!!!
    await c.set_prop( ctes.DPC_PriorityMode,    '0200' )
    
    await c.set_prop( ctes.DPC_CaptureControl,  '0002' )
    await c.initiate_capture( )
    
    // console.log( "Waiting for Autofocus" )
    // while( true ) {
    //   const r = await c.get_prop( ctes.DPC_AutoFocusStatus )
    //   console.log( "AutoFocus is", r)
    //   if( r.data != '0000' )
    //     break;
    // }
    // console.log( "Autofocus OK" )

    await c.set_prop( ctes.DPC_CaptureControl,  '0403' )
    const ans = await c.initiate_capture( )

    // Wait for the camera to capture
    while( true ) {
      const curr_state = await c.get_prop( ctes.DPC_CurrentState )
      console.log( curr_state )
      if( !curr_state.data )
        break;
      if( curr_state.data.length > 4 )
        break;
      await Conn.delay( 100 )
    }

    console.log( "Restoring camera")
    await c.set_prop( ctes.DPC_PriorityMode, '0100' )
    await c.terminate_open_capture()
  } 

  ctx.props = async () => {
    const props = Object.keys( ctes.props )
    for( let idx = 0; idx < props.length; ++idx ) {
      const id = props[idx]
      const info = await c.get_prop_info( id )
      console.log( info )
    }
  } 

  ctx.dir = async () => {
    const storage_ids = await c.get_storage_ids()
    console.log( "StorageIDs", storage_ids)
    
    for( let idx = 0; idx < storage_ids.length; ++idx ) {
      const info = await c.get_storage_info( storage_ids[idx] )
      console.log( info )
    }

    if( storage_ids.length > 0 ) {
      const sid = storage_ids[0];
      const num_objs = await c.get_num_objs( sid )
      const handles = await c.get_obj_handles( sid )
      console.log( handles )
      for( let hidx = 0; hidx < handles.length; ++hidx ) {
        const h = handles[hidx]
        const h_info = await c.get_obj_info( h )
        console.log( `  Handle ${hidx}/${handles.length}`)
        console.log( h_info )
        const h_supported_pros = await c.get_obj_supported_props( h )
        console.log( h_supported_pros )
      }
    }

  } 

  ctx.download = async () => {
    const storage_ids = await c.get_storage_ids()
    if( storage_ids.length > 0 ) {
      const sid = storage_ids[0];
      const num_objs = await c.get_num_objs( sid )
      const handles = await c.get_obj_handles( sid )
      console.log( handles )
      for( let hidx = 0; hidx < handles.length; ++hidx ) {
        const h = handles[hidx]
        const h_info = await c.get_obj_info( h )
        await c.get_obj( h )
        await c.del_obj( h )
      }
    }
  } 
}


if( 0 ) {
  const mb = require( "./cmd_basic")
  const mp = require( "./cmd_props")
  const ms = require( "./cmd_storage")
  const cmd = ms.getCommands()[2]()
  console.log( cmd.test() )
  // const m = ms.getCommands()[3]()
  // m.test();
  //const m2 = ms.getCommands()[1]()
  //m2.test();
} else {
//runTest( c )
  testTakeImage( c )
}

// Send a udp packet with my ip
// The camera answers with his name, his ip and his port

// Check answer from a prop description
//const prop_info = Buffer.from( '300000000200141003000000055004000102000200020c00020004000600080001800280038006800780088009800a800c0000000300012003000000', 'hex' )
//parsePTPPacket( {}, prop_info )

//const prop_info2 = Buffer.from( 'fb00000002001410070000000350ffff010a36003000300030007800340030003000300000000a36003000300030007800340030003000300000000209000a33003000300038007800320030003000300000000a33003000300038007800310036003800380000000a32003000300030007800320030003000300000000a34003200340030007800320038003300320000000a34003200340030007800320033003800340000000a32003800330032007800320038003300320000000a36003000300030007800340030003000300000000a36003000300030007800330033003700360000000a34003000300030007800340030003000300000000c0000000300012007000000', 'hex')
//parsePTPPacket( {}, prop_info2 )

//const prop_info3 = Buffer.from( '170000000200141038000000', 'hex')
//parsePTPPacket( {}, prop_info3 )

// 10 00 00 00 01 00 16 10 01 00 00 00 01 df 00 00 0e 00 00 00 02 00 16 10 01 00 00 00 00 00
//const set_prop_msg = makeSetPropValue( '01DF', '0000' )
//console.log( set_prop_msg )

//const ans = "NOTIFY * HTTP/1.1\r\nDSC: 172.20.10.5\r\nCAMERANAME: X-T2\r\nDSCPORT: 15740\r\nMX: 7\r\nSERVICE: PCSS/1.0\r\n"
//const cam = parseCameraDiscoveryMsg( ans )
//console.log( cam )
