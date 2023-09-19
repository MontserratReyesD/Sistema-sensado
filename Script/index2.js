var today;
var express = require('express');
var cors = require("cors");
const { createServer }  = require( "http");
const { Server } = require("socket.io");
const httpServer = createServer();
var io = require('socket.io')(httpServer, {
  cors: {
          origin: "*",
          methods: ["GET", "POST"],
          transports: ['websocket', 'polling'],
        },
  });
httpServer.listen(3000);
const { SerialPort } = require('serialport')
const { ReadlineParser } = require('@serialport/parser-readline')
const port = new SerialPort({
  path: 'COM8',
  baudRate: 115200,  
});
port.on('readable', function () {
  console.log('Data:', port.read());
})
port.on('data', function (data) {
  console.log('Data:', data);
})

const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }))
parser.on('data', (temp) => { 
      today = new Date();
      console.log((today.getHours())+"h:"+(today.getMinutes())+"m:"+today.getSeconds()+"s");
      io.sockets.emit('temp', {date: today.getDate()+"/"+(today.getMonth()+1)+"/"+today.getFullYear(), time: (today.getHours())+":"+(today.getMinutes()), temp:temp}); 
        });
io.on('connection', (socket) => {
            console.log("Someone connected.");
        });
