/*
 * Project ThermoPi
 * Description: Iot Thermostat for Home Automation System
 * Author: Brandon Palomino
 * Date: 9/3/18
 */

#include "Adafruit_DHT.h"
#include <blynk.h>
#include <math.h>

// Blynk Setup
char auth[] = "9a5060fcbcc0434481fb75eb8cbc036a";

// Settings
// V0 - Power
// V1 - Hot/Cold
// V2 - Set
// V3 - Stepper
// V4 - Temperature Reader
WidgetLCD lcd(V5); // V5 - LCD

// Setup Pins
#define FAN 0
#define COMPRESSOR 1
#define FLOW 2

// Helper Settings
#define RELAY_ON 0
#define RELAY_OFF 1
int acSetting = 1;
int temp = 0;  // used throughout logic
bool isPowered = false;
// bool isRunning = false;


// DHT Setup
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN,DHTTYPE);

// Auto Feature settings
unsigned long currentTime = 0;
int stepValue = 0;
int Auto = -1; // -1 = not on
int targetTemp = 0; // updated when set button is used
int tempOffSet = 2; // for power saving
BlynkTimer autoTimer;
int autoID;
int setID;

void setup() {
    // Particle.function("setTemp", setTemperature);
    // Particle.function("toggleDevice", toggleDevice);
    dht.begin();
    Blynk.begin(auth);
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
	pinMode(D2, OUTPUT);

    //setup up autotimer
    autoID = autoTimer.setInterval(60000, runAuto);
    autoTimer.disable(autoID);

    // start in off mode
	PowerOff();
    displayMessage("A/C: Off","");
}

// Power Button
BLYNK_WRITE(V0){
    if(param.asInt() == 1){
        PowerOn();
        // isRunning = true;
    }
    else {
        PowerOff();
        displayMessage("A/C: Off","");
        // isRunning = false;
        setAuto(0);
    }
}

// Segmented Switch
BLYNK_WRITE(V1){
    switch (param.asInt()) {
        case 1:
            if(isPowered){
                displayMessage("A/C: Fan only", "");
                setFanOnly();
            }
            acSetting = 1;
            break;
        case 2:
            if(isPowered){
                displayMessage("A/C: Cooling","");
                setCooling();
            }
            acSetting = 2;
            break;
        case 3:
            if(isPowered){
                displayMessage("A/C: Heating","");
                setHeating();
            }
            acSetting = 3;
            break;
    }
    if (Auto >= 1) setAuto(0);
}

// Set Auto Button
// BLYNK_WRITE(V2){
//     if (param.asInt() == 1){
//         if (isPowered && acSetting != 1) setAuto(1);
//     }
// }

// Stepper buttons
BLYNK_WRITE(V3){
    if (isPowered && acSetting != 1){
        stepValue = param.asInt();
        displayMessage("Set to: " + String(stepValue) + " F", "");
        if (!autoTimer.isEnabled(setID))
            setID = autoTimer.setTimeout(3000, updateAutoTemp);
        else{ 
            autoTimer.deleteTimer(setID);
            setID = autoTimer.setTimeout(3000, updateAutoTemp);
        }
    }
}

// Temperature Reader with Sensor
BLYNK_READ(V4){
    temp = floor(dht.getHeatIndex() * 9 / 5 + 32);
    Blynk.virtualWrite(V4, temp);
    stepValue = temp;
    Blynk.virtualWrite(V3, temp);
}


void setFanOnly(){
    digitalWrite(FAN,RELAY_ON);
    digitalWrite(COMPRESSOR,RELAY_OFF);
    digitalWrite(FLOW,RELAY_OFF);
    isPowered = true;
}

void setCooling(){
    digitalWrite(FAN,RELAY_ON);
    digitalWrite(COMPRESSOR,RELAY_ON);
    digitalWrite(FLOW,RELAY_OFF);
    isPowered = true;
}

void setHeating(){
    digitalWrite(FAN,RELAY_ON);
    digitalWrite(COMPRESSOR,RELAY_ON);
    digitalWrite(FLOW,RELAY_ON);
    isPowered = true;
}

void displayMessage(String setting, String message){
    lcd.clear();
    lcd.print(0,0,setting);
    lcd.print(0,1,message);
}

void PowerOn(){
    if(acSetting == 1){
        displayMessage("A/C: Fan only", "");
        setFanOnly();
    }
    else if(acSetting == 2){
        displayMessage("A/C: Cooling", "");
        setCooling();
    }
    else if(acSetting == 3){
        displayMessage("A/C: Heating", "");
        setHeating();
    }
    // isPowered = true;
}

void PowerOff(){
    digitalWrite(FAN,RELAY_OFF);
    digitalWrite(COMPRESSOR,RELAY_OFF);
    digitalWrite(FLOW,RELAY_OFF);
    isPowered = false;
}

void setAuto(int setting){
    if (setting == 1){
        if (Auto != 1){ // don't call auto twice
            Auto = 1;
            autoTimer.enable(autoID);
            targetTemp = stepValue;
        }
    }
    else{
        autoTimer.disable(autoID);
        Auto = -1;
    }
}

void updateAutoTemp(){
    // turn on auto else keep going
    if (Auto < 1){
        Auto = 1;
        autoTimer.enable(autoID);
    } 
    targetTemp = stepValue;
}

// algorithm for efficient auto temperature management
void runAuto(){
    temp = floor(dht.getHeatIndex() * 9 / 5 + 32); // calc & update current temp var
    if (Auto == 1){ // ac running (w/Compressor)
        if (acSetting == 2){ // cooling
            displayMessage("A/C Auto", "Cooling to " + String(targetTemp) + " F");
            if (!isPowered) setCooling();
            if (temp-2 <= targetTemp){
                setFanOnly();
                Auto = 2;
            }
            // if (temp <= targetTemp) Auto = 3;
        }
        if (acSetting == 3){ // heating
            displayMessage("A/C Auto", "Heating to " + String(targetTemp) + " F");
            if (!isPowered) setHeating();
            if (temp+2 >= targetTemp){
                setFanOnly();
                Auto = 2;
            }
            // if (temp >= targetTemp) Auto = 3;
        }
    }
    if (Auto == 2){ // ac running (w/o Compressor)
        if (acSetting == 2){ // cooling
            if (temp > targetTemp+2) Auto = 1;
            if (temp <= targetTemp) Auto = 3;
        }
        if (acSetting == 3){ // heating
            if (temp < targetTemp-2) Auto = 1;
            if (temp >= targetTemp) Auto = 3;
        }
    }
    if (Auto == 3){ // ac fixed (off)
        displayMessage("A/C: Auto", "Fixed at " + String(targetTemp) + " F");
        if (isPowered) PowerOff(); // power saving
        if (temp <= (targetTemp-tempOffSet) || temp >= (targetTemp+tempOffSet)) Auto = 1;
    }
}

// int setTemperature(String temp){
//     int value = temp.toInt();
//     if (value != 0){
//         if (!isPowered) PowerOn();
//         stepValue = value;
//         setAuto(1);
//         return 0;
//     }
//     return -1;
// }

// int toggleDevice(String setting){
//     if (setting == "on"){
//         if (!isPowered) PowerOn();
//         return 1;
//     }
//     else if (setting == "off") {
//         if (isPowered) PowerOff();
//         return 0;
//     }
//     else {
//         return -1;
//     }
// }

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    Blynk.run();
    autoTimer.run();
}