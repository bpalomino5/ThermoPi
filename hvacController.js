#!/usr/bin/env node
/*
Needs:
Modules : On/off, pigpio, blynk-library
Blynk Items:
Power Button: v0
Temp Setting Button: v1
Temperature value buttons: v3
Temp reader: v4
Segment Display: v5
Set Temp Button: v2
*/

/*
colors:
BLYNK_GREEN     "#23C48E"
BLYNK_BLUE      "#04C0F8"
BLYNK_YELLOW    "#ED9D00"
BLYNK_RED       "#D3435C"
BLYNK_DARK_BLUE "#5F7CD8"
*/

//Setup Colors
var BLYNK_BLUE = "#04C0F8"
var BLYNK_RED = "#D3435C"

//Setup Pins
var PIN_TEMPERATURE = 4
var PIN_FAN = 17
var PIN_COMPRESSOR = 18
var PIN_COOL = 27

var RELAY_ON = 0        // GPIO.LOW 0
var RELAY_OFF = 1    // GPIO.HIGH 1

var blynkToken = '9a5060fcbcc0434481fb75eb8cbc036a';    // BLYNK TOKEN


var sensor = require('node-dht-sensor');

// These are for the physical pins on the RPi
// Fan, Compressor, Flow Direction, Temp sensor
var Gpio = require('pigpio').Gpio,
  FAN = new Gpio(PIN_FAN, {mode: Gpio.OUTPUT}),
  COMPRESSOR = new Gpio(PIN_COMPRESSOR, {mode: Gpio.OUTPUT}),
  COOL = new Gpio(PIN_COOL, {mode: Gpio.OUTPUT});

//Setup blynk
var Blynk = require('blynk-library');
var blynk = new Blynk.Blynk(blynkToken);

//Setup Virtual Pins
var v0 = new blynk.VirtualPin(0);
var v1 = new blynk.VirtualPin(1);
// var v2 = new blynk.VirtualPin(2)
var v4 = new blynk.VirtualPin(4);

blynk.on('connect', function() { console.log("Blynk ready.\n\nSettings Log:"); });
blynk.on('disconnect', function() { console.log("DISCONNECT"); });



function getTemp(){
  //get temp from DHT22
  var t = sensor.read(22, PIN_TEMPERATURE).temperature.toFixed(1);
  // convert from C to F
  t = t * 9/5.0 + 32;
  return t;
}

v0.on('write', function(param) {
    if (param[0] === '1') {
      PowerOn()
      console.log("A/C On");
    } else { 
      PowerOff()
      console.log("A/C Off");
    }
});

v1.on('write', function(param) {
    if (param[0] === '1') {
      blynk.setProperty(1,"color",BLYNK_RED);
      console.log("*** PUMP: HOT ***");

    } else { 
      blynk.setProperty(1,"color",BLYNK_BLUE);
      console.log("*** PUMP: COLD ***");
    }
});

v4.on('read', function(val) {
  console.log("*** Reading Temperature ***");
  v4.write(getTemp());
});

function PowerOn() {
  FAN.digitalWrite(RELAY_ON);
  COMPRESSOR.digitalWrite(RELAY_ON);
  COOL.digitalWrite(RELAY_ON);
}

function PowerOff() {
  FAN.digitalWrite(RELAY_OFF);
  COMPRESSOR.digitalWrite(RELAY_OFF);
  COOL.digitalWrite(RELAY_OFF);
}

