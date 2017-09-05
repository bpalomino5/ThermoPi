# ThermoPi
Home Programmable Thermostat designed with Raspberry Pi

## Description
- Using Adafruit DHT Library
- Python

### Instructions
To use enter the following into the raspberry pi:
sudo forever start -l forever.log -o out.log --minUptime=3000 --spinSleepTime=10000 hvacController.js

If restarting again:
- clean logs with: sudo forever cleanlogs