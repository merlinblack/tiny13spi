# Experiment in bitbanged SPI client for ATtiny13.

 * Based on my 'blinky' code so it does something while waiting for SPI commands.
 * Using same SPI pins as used to program the chip.
 * No chip select as we have few pins and nothing else is connected.
 * No buffering as RAM is limited.
 * At higher SPI Clock Hz, you need to wait between bytes to allow 
 * commands to be processed.
 * Talks to a Raspberry Pi with bread board breakout attached, which also does the programming (AVRDUDE)
 
## SPI commands

### 0xCD - Carrier detect.
On the next byte transfer 0xA5 will be returned, signalling a hopefully good connection and clock rate.
 
### 0xE0 - Start reading EEPROM.
Address is set to the start of EEPROM, and first byte will be returned on the next transfer.
  
### 0xE1 - Read next byte from EEPROM
Next byte from EEPROM will be returned next transfer, and address will be incremented.
  
## Other command possibilties:

PWM, ADC, or just reading or writing to pins.