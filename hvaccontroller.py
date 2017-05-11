#!/usr/bin/env python
import RPi.GPIO as GPIO
import time


PIN_FAN = 11
PIN_COMPRESSOR = 13
PIN_COOL = 15

RELAY_ON = False				# GPIO.LOW 0
RELAY_OFF = (not RELAY_ON)		# GPIO.HIGH 1

def setupSystem():
	# set to read by board
	GPIO.setmode(GPIO.BOARD)

	# init pins to output
	GPIO.setup(PIN_FAN, GPIO.OUT)
	GPIO.setup(PIN_COMPRESSOR, GPIO.OUT)
	GPIO.setup(PIN_COOL, GPIO.OUT)

	# init relays off first
	GPIO.output(PIN_FAN, RELAY_OFF)
	GPIO.output(PIN_COMPRESSOR, RELAY_OFF)
	GPIO.output(PIN_COOL, RELAY_OFF)

def cleanup():
	# turn off all the relays
	GPIO.output(PIN_FAN, RELAY_OFF)
	GPIO.output(PIN_COMPRESSOR, RELAY_OFF)
	GPIO.output(PIN_COOL, RELAY_OFF)

	# clean board pins
	GPIO.cleanup()


def testRelays():
	time.sleep(5)
	# turn on all three relays
	GPIO.output(PIN_FAN, RELAY_ON)
	GPIO.output(PIN_COMPRESSOR, RELAY_ON)
	GPIO.output(PIN_COOL, RELAY_ON)


if __name__ == '__main__':
	setupSystem()
	try:
		testRelays()
	except KeyboardInterrupt:
		cleanup()