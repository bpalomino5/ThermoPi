#!/usr/bin/env node
/*
Needs:
Modules : On/off, pigpio, blynk-library
Blynk Items:
Power Button
Temp Setting Button
Temperature value inc/dec buttons
Temp reader
Segment Display
Set Temp Button


*/
var buttonPin = 18    //PWM GPIO PIN
var blynkToken = '360f02cc70e5449dbc5f60fee3ca6931';    // BLYNK TOKEN

var Gpio = require('pigpio').Gpio,
  button = new Gpio(buttonPin, {mode: Gpio.OUTPUT});

//Setup blynk
var Blynk = require('blynk-library');
var blynk = new Blynk.Blynk(blynkToken);
var v0 = new blynk.VirtualPin(0);

v0.on('write', function(param) {
    console.log('V0:', param);
    if (param[0] === '0') { 
      lightON()
    } else { 
      lightOFF()
    }
});

blynk.on('connect', function() { console.log("Blynk ready."); });
blynk.on('disconnect', function() { console.log("DISCONNECT"); });

function lightON() {
  button.digitalWrite(0);
  // switched = true
}

function lightOFF() {
  button.digitalWrite(1);
  // switched = false
}