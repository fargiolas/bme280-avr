# bme280-avr
## AVR driver for Bosch BME280

A super simple AVR port for Bosch BME280 driver. Reads temperature, humidity and pressure in forced-mode and streams to UART.

Calling it port is probably too much, Bosch library basically works out of the box, you just need to provide platform specific delay and i2c r/w functions.

Makefile for atmega328/arduino. Tested on Arduino Nano with a sensor breakout board from Adafruit.


Sensor driver from Bosch: https://github.com/BoschSensortec/BME280_driver

I2C Master Library (TWI implementation) from Peter Fleury: http://www.peterfleury.epizy.com/avr-software.html
