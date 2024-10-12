const net = require("net");
const udp = require('dgram');

const port = 5002;
const udp_server = udp.createSocket('udp4');

// emits on new datagram msg
udp_server.on('message',function(msg,info){
  console.log('Data received from client : ' + msg.toString());
  console.log('Received %d bytes from %s:%d',msg.length, info.address, info.port);
})

udp_server.on('listening', () => {
  const address = udp_server.address();
  console.log(`UDP.Server listening ${address.address}:${address.port}`);
});

udp_server.bind(port);

