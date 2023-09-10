#! /usr/bin/env python

import spidev
from time import sleep

spi = spidev.SpiDev()

spi.open(0,0)
spi.max_speed_hz=1000

spi.xfer([0xCD]) # Ask for carrier detect
sleep(.5)
print(spi.xfer([0xCD])) # Should print 165 (A5 in decimal)
sleep(.5)
print(spi.xfer([0xCD])) # Should print 165 (A5 in decimal)
sleep(.5)
print(spi.xfer([0xCD])) # Should print 165 (A5 in decimal)

spi.close()
