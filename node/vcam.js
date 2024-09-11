const net = require("net");
var udp = require('dgram');

const port = 5001;

function connectToClient( ip, port, msg ) {
  const client = new net.Socket();

  // Handle connection close event
  client.on('close', () => {
    console.log('Client.Connection closed');
  });
    
  console.log(`Connecting to server ${ip}:${port}`);
  client.connect(port, ip, () =>{
    console.log('Connected to client asking');
    console.log("Writing to server", msg)
    client.write(msg);
    //client.destroy();
  });
}

const udp_server = udp.createSocket('udp4');
// emits on new datagram msg
udp_server.on('message',function(msg,info){
  console.log('Data received from client : ' + msg.toString());
  console.log('Received %d bytes from %s:%d',msg.length, info.address, info.port);
/*
DISCOVERY * HTTP/1.1
HOST: 192.168.1.136
MX: 5
SERVICE: PCSS/1.0
*/
  const lines = msg.toString().split( "\n");
  const ip_asking = lines[1].split( ":")[1].trim();
  console.log( "IP Asking is", ip_asking)

  // Connect to the client-ip and using tcp tell him our ip
  const my_ip = "192.168.1.136"
  const ans = `NOTIFY * HTTP/1.1\r\nDSC: ${my_ip}\r\nCAMERANAME: X-T2\r\nDSCPORT: ${port}\r\nMX: 7\r\nSERVICE: PCSS/1.0\r\n`
  connectToClient( ip_asking, 51560, ans )
})

udp_server.on('listening', () => {
  const address = udp_server.address();
  console.log(`UDP.Server listening ${address.address}:${address.port}`);
});
udp_server.bind(5002);


const server = net.createServer((socket) => {
    console.log("Client connected to the virtual cam");

    socket.on("data", (data) => {
        const strData = data.toString();
        console.log(`Received: ${strData}`);
        let result = "BLAH"
        const n = socket.write(result.toString());
        console.log( "Answering", result, n )
    });

    socket.on("end", () => {
        console.log("Client disconnected");
    });

    socket.on("error", (error) => {
        console.log(`Socket Error: ${error.message}`);
    });
});

server.on("error", (error) => {
    console.log(`Server Error: ${error.message}`);
});

server.listen(port, () => {
    console.log(`TCP socket server is running on port: ${port}`);
});
