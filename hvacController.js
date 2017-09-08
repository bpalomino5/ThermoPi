#!/usr/bin/env node
//Author: Brandon Palomino
//Domain: com.bpalomino.ThermoPi
//Date: 6/27/17
//Description: ThermoPi is my homemade programmable hvac-system controller which handles all the core features of a regular wall thermostat.
//             Designed for exploration of skills and advancing my knowledge of circuitry.

/*
Uses the Following:
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
var PIN_FLOW = 27

// Helper Settings
var RELAY_ON = 0        // GPIO.LOW 0
var RELAY_OFF = 1    // GPIO.HIGH 1

// Blynk Cloud Token
var blynkToken = '9a5060fcbcc0434481fb75eb8cbc036a';    // BLYNK TOKEN

// Helper vars
var temp = 0
var stepValue = 0
var tempOffset = 3

// checks for auto feature
var auto = -1   //-1 = not on
var targetTemp = 0
var flowSetting = 0 //  0=cold
var isPowered = false
var autoTimer;

// Dht sensor module
var sensor = require('node-dht-sensor');

// These are for the physical pins on the RPi
// Fan, Compressor, Flow Direction, Temp sensor
var Gpio = require('pigpio').Gpio,
  FAN = new Gpio(PIN_FAN, {mode: Gpio.OUTPUT}),
  COMPRESSOR = new Gpio(PIN_COMPRESSOR, {mode: Gpio.OUTPUT}),
  FLOW = new Gpio(PIN_FLOW, {mode: Gpio.OUTPUT});

//Setup blynk
var Blynk = require('blynk-library');
var blynk = new Blynk.Blynk(blynkToken);

//Setup Virtual Pins
var v0 = new blynk.VirtualPin(0);   // Power
var v1 = new blynk.VirtualPin(1);   // Hot/Cold
var v2 = new blynk.VirtualPin(2);   // Set
var v3 = new blynk.VirtualPin(3);   // Stepper
var v4 = new blynk.VirtualPin(4);   // Temperature Scale
var v5 = new blynk.WidgetLCD(5);    // LCD

blynk.on('connect', function() { 
    console.log("Blynk ready.\n\nSettings Log:");
    displayReset() // if thermoPi resets because of wifi problems and reconnects then display in blynk will be reset to cooling    
});

blynk.on('disconnect', function() { console.log("DISCONNECT"); });

function getTemp(){
  //get temp from DHT22
  var t = sensor.read(22, PIN_TEMPERATURE).temperature.toFixed(1);
  // convert from C to F
  t = t * 9/5.0 + 32;
  return Math.round(t);
}

v0.on('write', function(param) {
    if (param[0] === '1') {
        PowerOn()
        process.stdout.write(getTimestamp())
        console.log("A/C On");
        displayOn()
    } else {
        PowerOff()
        process.stdout.write(getTimestamp())
        console.log("A/C Off");
        displayOff()
        setAutoOff()
    }
});

v1.on('write', function(param) {
    if (param[0] === '1') {
        blynk.setProperty(1,"color",BLYNK_RED);
        process.stdout.write(getTimestamp())
        console.log("*** FLOW: HOT ***");
        v5.clear();
        v5.print(0,0, "A/C: Heating");
        setCompHot()
    } else { 
        blynk.setProperty(1,"color",BLYNK_BLUE);
        process.stdout.write(getTimestamp())
        console.log("*** FLOW: COLD ***");
        v5.clear();
        v5.print(0,0, "A/C: Cooling");
        setCompCold()
    }

    //if auto running set off
    if(auto >= 0) setAutoOff();
});

v2.on('write', function(param) {
    if(param[0] === '1'){
        if(isPowered) setAuto();
    }
});

v3.on('write', function(param) {
    if(isPowered){
        stepValue = Number(param[0]);
        v5.clear();
        v5.print(0,0, "Set to: " + stepValue +" F")
    }
});

v4.on('read', function(val) {
    //get temp from dht
    process.stdout.write(getTimestamp())
    console.log("*** Reading Temperature ***");
    temp = getTemp();
    v4.write(temp);

    //writing temp value to stepper
    stepValue = temp;
    v3.write(temp);
});

/// Functions for Power
function PowerOn() {
    FAN.digitalWrite(RELAY_ON);
    COMPRESSOR.digitalWrite(RELAY_ON);
    FLOW.digitalWrite(RELAY_ON);
    flowSetting = 0
    isPowered = true;
}

function PowerOff() {
    FAN.digitalWrite(RELAY_OFF);
    COMPRESSOR.digitalWrite(RELAY_OFF);
    FLOW.digitalWrite(RELAY_OFF);
    isPowered = false;
}

/// Functions for Compressor Air Flow
function setCompHot() {     // R+G+Y active sets hot air
    FLOW.digitalWrite(RELAY_OFF);
    flowSetting = 1
}

function setCompCold() {    // R+G+Y+O active sets cold air
    FLOW.digitalWrite(RELAY_ON);
    flowSetting = 0
}

/// Function for Auto feature
function setAuto() {
    if(stepValue > temp){
        // v5.clear()
        // v5.print(0,0, "A/C Auto:")
        // v5.print(0,1, "Heating to " + stepValue + " F")
        displayMessage("Heating to", stepValue)
        auto = 1
    }
    else if(stepValue < temp){
        // v5.clear()
        // v5.print(0,0, "A/C Auto:")
        // v5.print(0,1, "Cooling to " + stepValue + " F")
        displayMessage("Cooling to", stepValue)
        auto = 0
    }
    else{ // stepValue and temp are equal
        // v5.clear()
        // v5.print(0,0, "A/C Auto:")
        // v5.print(0,1, "Fixed at " + temp + " F")
        displayMessage("Fixed at", temp)
        auto = 2
    }
    //target is set to stepValue
    targetTemp = stepValue

    //set timer to run callback every minute
    if(!autoTimer){
        autoTimer = setInterval(runAuto, (1000 * 60));
    }
}

function runAuto() {
    temp = getTemp();
    process.stdout.write(getTimestamp())
    console.log("*** Auto: " + auto)

    if(auto == 1){  //heating
        //make sure compressor is heating
        if(!isPowered) PowerOn();
        if(flowSetting != 1) setCompHot();
        //check temp with target temp
        if(temp >= targetTemp) auto = 2;
    }
    if(auto == 0){  //cooling
        if(!isPowered) PowerOn();
        if(flowSetting != 0) setCompCold();
        if(temp <= targetTemp) auto = 2;
    }
    if(auto == 2){  //fixed
        //power saving feature,
        //turn off a/c
        if(isPowered) PowerOff();
        displayMessage("Fixed at", targetTemp);
        // displayFixed();
        //fix temp after offset degree difference
        if(temp <= (targetTemp-tempOffset)) auto = 1;   //heating
        if(temp >= (targetTemp+tempOffset)) auto = 0;   //cooling
    }
}

function setAutoOff() {
    auto = -1
    clearInterval(autoTimer);
    autoTimer = false;
}

function displayMessage(message, temp) {
    v5.clear()
    v5.print(0,0, "A/C Auto:")
    v5.print(0,1, message + " " + temp + " F")
}

// function displayFixed() {
//     v5.clear()
//     v5.print(0,0, "A/C Auto:")
//     v5.print(0,1, "Fixed at " + targetTemp + " F")
// }

function displayReset() {
    v5.clear();
    v5.print(0,0, "A/C: Unsure");
    v5.print(0,1, "Repower Me!");
}

function displayOn() {
    process.stdout.write(getTimestamp())
    console.log("*** FLOW: COLD ***");
    v5.clear();
    v5.print(0,0, "A/C: Cooling");
}

function displayOff() {
    v5.clear()
    v5.print(0,0, "A/C Off");
}

function getTimestamp(){
    d = new Date()
    return d.getMonth()+"/"+d.getDate()+"/"+d.getFullYear()+" "+ d.getHours()+':'+d.getMinutes()+':'+d.getSeconds()+" : ";
}