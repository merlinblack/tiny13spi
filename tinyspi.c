#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdbool.h>

// prescaler is set so timer0 ticks every 64 clock cycles, and the overflow handler
// is called every 256 ticks.

#define MICROSECONDS_PER_TIMER0_OVERFLOW \
  (64UL * 256UL * 100000UL / ((F_CPU + 5UL) / 10UL))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000U)

// the fractional number of milliseconds per timer0 overflow
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000U) >> 3)

// shift right to fit in a byte
#define FRACT_MAX (1000U >> 3)

volatile unsigned long timer0_millis = 0;
volatile unsigned char timer0_fract = 0;

#if defined(__AVR_ATtiny13__) || defined(__AVR_ATtiny13A__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny85__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
  // Copy to local vars so they can go in registers
  unsigned long m = timer0_millis;
  unsigned char f = timer0_fract;

  f += FRACT_INC;
  if (f >= FRACT_MAX) {
    f -= FRACT_MAX;
    m += MILLIS_INC + 1;
  }
  else {
    m += MILLIS_INC;
  }

  timer0_millis = m;
  timer0_fract = f;
}


void init_timer()
{
  sei();
  // Set timer0 prescaler to 64
#ifdef TCCR0B
  TCCR0B |= _BV(CS01) | _BV(CS00);
#else
  TCCR0 |= _BV(CS01) | _BV(CS00); // Atmega32a
#endif
  // Set timer0 interrupt mask - TimerOverflowInterruptEnable
#ifdef TCCR0B
  TIMSK0 |= _BV(TOIE0);
#else
  TIMSK |= _BV(TOIE0); // Atmega32a
#endif
}

unsigned long millis()
{
  unsigned long m;
  uint8_t oldSREG = SREG;

  // copy millis into 'm' atomically by disabling interrupts
  cli();
  m = timer0_millis;
  SREG = oldSREG;

  return m;
}

int times[] = { 250, 250, 250, 1000, 50, 50, 50, 50, 50, 50, 50, 2000, 0 };

#define LEDPIN PB3
#define LEDDDR DDB3

#define MOSIPIN PB0
#define MISOPIN PB1
#define SPICLKPIN PB2
#define MOSIDDR DDB0
#define MISODDR DDB1
#define SPICLKDDR DDB2

#define EEPROM_END (uint8_t *)0x40

void init_spi()
{
  DDRB |= _BV(MISODDR);
  DDRB &= ~_BV(MOSIDDR);
  DDRB &= ~_BV(SPICLKDDR);

  PORTB &= ~_BV(MISOPIN);
}

int main(void)
{
  uint8_t spi_in;
  uint8_t spi_out;
  bool spi_data_ready;
  uint8_t spi_bit_count;

  uint8_t *eeprom_read_address;

  init_timer();
  init_spi();
  
  spi_data_ready = false;
  spi_in = 0;
  spi_out = 0;
  spi_bit_count = 8;

  DDRB |= _BV(LEDDDR);
  PORTB &= _BV(LEDPIN);

  unsigned long lastTime = 0;
  uint8_t t_index = 0;
  bool spi_clock_prev = false;

  for (;;) {
    unsigned long currentTime = millis();

    if (currentTime > lastTime + times[t_index]) {
      PORTB ^= _BV(LEDPIN);
      lastTime = currentTime;
      t_index++;
      if (!times[t_index])
        t_index = 0;
    }

    uint8_t spi_clock = PINB & _BV(SPICLKPIN);

    // Check for rising edge of clock
    if (!spi_clock_prev && spi_clock)
    {
      spi_clock_prev = true;
      spi_in |= PINB & _BV(MOSIPIN);
    }

    // falling edge
    if (spi_clock_prev && !spi_clock) {
      spi_clock_prev = false;
      spi_bit_count--;

      if (spi_bit_count > 0) {
        spi_in <<= 1;
        spi_out <<= 1;

        if (spi_out & 0x80) {
          PORTB |= _BV(MISOPIN);
        }
        else {
          PORTB &= ~_BV(MISOPIN);
        }
      }
      else {
        spi_data_ready = true;
        spi_bit_count = 8;
      }
    }

    // work with the data
    if (spi_data_ready) {
      spi_data_ready = false;
      switch (spi_in) {
        case 0xCD: // Carrier detect
          spi_out = 0xA5;
          break;

        case 0xE0: // Start eeprom read from the start
          eeprom_read_address = 0x00;
          // Fall through
        
        case 0xE1: // Read next eeprom byte
          if (eeprom_read_address < EEPROM_END) {
            spi_out = eeprom_read_byte(eeprom_read_address++);
          }
          else {
            spi_out = 0x00;
          }
          break;

        default: // Ignore
          break;
      }
      /* Set MSB on pin *before* first rising clock edge as the master
         will read it then */
      if (spi_out & 0x80) {
        PORTB |= _BV(MISOPIN);
      }
      else {
        PORTB &= ~_BV(MISOPIN);
      }
    }
  }

  return 0;
}
