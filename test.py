#! /usr/bin/env python

import spidev
from time import sleep

def carrierDetect(spi):
	spi.xfer([0xCD]) # Ask for carrier detect
	sleep(.2)
	ret = spi.xfer([0x00])[0]

	if ret != 0xA5:
		print('Got unexpected respoonse: ', ret)

	return ret == 0xA5

def readEEPROM(spi):
	print('Retreiving EEPROM data:')
	spi.xfer([0xE0]) # Ask for address reset, and read first byte

	data = []

	for i in range(0,64):
		sleep(.25) # EEPROM read takes longer carrier detect command.
		ret = spi.xfer([0xE1])[0]
		data.append(ret)
		print('.', end='', flush=True)

	print('')
	return data


if __name__ == '__main__':
	spi = spidev.SpiDev()

	spi.open(0,0)
	spi.max_speed_hz=5000

	if carrierDetect(spi):
		print('ATtiny13 is alive and well')
		data = readEEPROM(spi)
		print(data)
		data = data[:data.index(0xff)]
		byteStr = bytes(data)
		decoded = byteStr.decode('utf-8')
		print(decoded)
	else:
		print('Could not contact ATtiny13')

	spi.close()
