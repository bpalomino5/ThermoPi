/*
 * Project ThermoPi
 * Description: Iot Thermostat for Home Automation System
 * Author: Brandon Palomino
 * Date: 9/15/18
 */

#include "Adafruit_DHT.h"
#include <blynk.h>
#include <math.h>
#include "EchoPhotonBridge.h"

// Echo Setup
EchoPhotonBridge epb;

// Blynk Setup
char auth[] = "9a5060fcbcc0434481fb75eb8cbc036a";

// Settings
// V0 - Power
// V1 - Hot/Cold
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
int temp = 0;  // default room temp
bool isPowered = false;


// DHT Setup
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN,DHTTYPE);

// Auto Feature settings
unsigned long currentTime = 0;
int stepValue = 0;
int Auto = -1; // -1 = not on
int targetTemp = 0; // updated when set button is used
int tempOffSet = 3; // for power saving
BlynkTimer autoTimer;
int autoRounds = 0;
int autoID;
int setID;

// Echo Functions

int functionOnOff(int device, bool onOff, String rawParameters){
    if (onOff) PowerOn();
    else{
        PowerOff();
        displayMessage("A/C: Off","");
        setAutoOff();
    }
    return 0;
}

int functionTemp(int device, bool requestCurrentTemp, bool requestSetTemp, int temperature, int changeAmount, String rawParameters){
    if (requestCurrentTemp) return temp;
    if (requestSetTemp) return targetTemp;
    if (!isPowered) PowerOn();
    targetTemp = temperature;
    return 0;
}

// GA functions

// int setOnOFF(String command){
//     if (command == "1") PowerOn();
//     else{
//         PowerOff();
//         displayMessage("A/C: Off","");
//         setAutoOff();
//     }
//     return 0;
// }

// int setTemperature(String command){
//     if (!isPowered) PowerOn();
//     targetTemp = command.toInt();
//     return 0;
// }


void setup() {
    dht.begin();
    Blynk.begin(auth);
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
	pinMode(D2, OUTPUT);

    //setup up autotimer
    autoID = autoTimer.setInterval(60000, runAuto);
    autoTimer.disable(autoID);

    setID = autoTimer.setInterval(3000, updateAutoTemp);
    autoTimer.disable(setID);

    // start in off mode
	PowerOff();
    displayMessage("A/C: Off","");

    // EPB Setup
    epb.addEchoDeviceV2OnOff("AC", &functionOnOff);
    epb.addEchoDeviceV2Temp("AC", &functionTemp);

    // GA Setup
    // Particle.function("OnOff", setOnOFF);
    // Particle.function("setTemp", setTemperature);
}

// Power Button
BLYNK_WRITE(V0){
    if(param.asInt() == 1){
        PowerOn();
    }
    else {
        PowerOff();
        displayMessage("A/C: Off","");
        setAutoOff();
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
            if (Auto >= 1) setAutoOff();
            break;
        case 2:
            if(isPowered){
                displayMessage("A/C: Cooling","");
                setCooling();
                updateAutoTemp();
            }
            acSetting = 2;
            break;
        case 3:
            if(isPowered){
                displayMessage("A/C: Heating","");
                setHeating();
                updateAutoTemp();
            }
            acSetting = 3;
            break;
    }
}

// Stepper buttons
BLYNK_WRITE(V3){
    if ((isPowered && acSetting != 1) || Auto >= 1){ // either simply powered or auto running
        stepValue = param.asInt();
        displayMessage("Set to: " + String(stepValue) + " F", "");
        if (autoTimer.isEnabled(setID)){
            autoTimer.restartTimer(setID);
        } 
        else {
            autoTimer.enable(setID);
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
        updateAutoTemp();
    }
    else if(acSetting == 3){
        displayMessage("A/C: Heating", "");
        setHeating();
        updateAutoTemp();
    }
    Particle.publish("OnPower", String(isPowered), PRIVATE);
}

void PowerOff(){
    digitalWrite(FAN,RELAY_OFF);
    digitalWrite(COMPRESSOR,RELAY_OFF);
    digitalWrite(FLOW,RELAY_OFF);
    isPowered = false;
    Particle.publish("OnPower", String(isPowered), PRIVATE);
}

void setAutoOff(){
    autoTimer.disable(autoID);
    Auto = -1;
}

void updateAutoTemp(){
    // turn on auto else keep going
    if (Auto < 1){
        Auto = 1;
        autoTimer.enable(autoID);
    }
    targetTemp = stepValue;
    displayMessage("A/C Auto", String(targetTemp) + " F");
    autoTimer.disable(setID);
}

// algorithm for efficient auto temperature management
void runAuto(){
    temp = floor(dht.getHeatIndex() * 9 / 5 + 32); // calc & update current temp var

    // for logging
    Particle.publish("AutoRounds", String(autoRounds));
    Particle.publish("AutoSetting", String(Auto));
    // Particle.publish("Temp", String(temp));
    // Particle.publish("Target", String(targetTemp));
    Particle.publish("AC Setting", String(acSetting));

    if (Auto == 1){ // ac running (w/Compressor)
        if (acSetting == 2){ // cooling
            displayMessage("A/C Auto", "Cooling to " + String(targetTemp) + " F");
            if (!isPowered) setCooling();
            if (temp-2 <= targetTemp){
                setFanOnly();
                Auto = 2;
                autoRounds++;
            }
        }
        if (acSetting == 3){ // heating
            displayMessage("A/C Auto", "Heating to " + String(targetTemp) + " F");
            if (!isPowered) setHeating();
            if (temp+2 >= targetTemp){
                setFanOnly();
                Auto = 2;
                autoRounds++;
            }
        }
    }
    if (Auto == 2){ // ac running (w/o Compressor)
        if (acSetting == 2){ // cooling
            if (temp > targetTemp+2){
                setCooling();
                Auto = 1;
            }
            if (temp <= targetTemp || autoRounds >= 2) Auto = 3;
        }
        if (acSetting == 3){ // heating
            if (temp < targetTemp-2){
                setHeating();
                Auto = 1;
            }
            if (temp >= targetTemp || autoRounds >= 2) Auto = 3;
        }
    }
    if (Auto == 3){ // ac fixed (off)
        displayMessage("A/C: Auto", "Fixed at " + String(targetTemp) + " F");
        if (isPowered) PowerOff(); // power saving
        if (temp < (targetTemp-tempOffSet) || temp > (targetTemp+tempOffSet)){
            autoRounds = 0; // reset rounds
            Auto = 1;
        }
    }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    Blynk.run();
    autoTimer.run();
}