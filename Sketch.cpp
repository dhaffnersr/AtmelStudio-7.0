﻿/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

*/This firmware is related to the AD7667ASTZ (ADC) and the ATMega1284P MCU used in the CCD driver circuit for the TCD1304DG.
*/All details pertaining to programming  peripherals and settings are taken from the appropriate data sheets. 

#include <util/delay_basic.h>

#define RD (1<<2)
#define CNVST (1<<3)
#define BYTESWAP (1<<4)
#define ICG (1<<5)
#define SH (1<<6)
#define MCLK (1<<7)

// Full frame, including dark pixels
// and dead pixels.
#define PIXEL_COUNT 3691

// Ports and pins
#define CLOCKS PORTD
#define CLOCKP PIND
#define CLOCKS_DDR DDRD
#define DATA_PINS PINC
#define DATA_PORT PORTC
#define DATA_DDR DDRC

// 10mS exposure time.
#define EXPOSURE_TIME 10

// Initial clock state.
uint8_t clocks0 = (RD + CNVST + ICG);

// 16-bit pixel buffer
uint16_t pixBuf[PIXEL_COUNT];

char cmdBuffer[16];
int cmdIndex;
int exposureTime = EXPOSURE_TIME;
int cmdRecvd = 0;

/*
 * readLine() Reads all pixels into a buffer.
 */

void readLine() {
  // Get an 8-bit pointer to the 16-bit buffer.
  uint8_t *buf = (uint8_t *) pixBuf;
  int x = 0;
  uint8_t scratch = 0;
  
  // Disable interrupts or the timer will get us.
  cli();
  
  // Synchronize with MCLK and
  // set ICG low and SH high.
  scratch = CLOCKS;
  scratch &= ~ICG;
  scratch |= SH;
  while(!(CLOCKP & MCLK));
  while((CLOCKP & MCLK));
  TCNT2 = 0;
  _delay_loop_1(1);
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  CLOCKS = scratch;
  
  // Wait the remainder of 4uS @ 20MHz.
  _delay_loop_1(22);
  __asm__("nop");
  __asm__("nop");

  // Set SH low.
  CLOCKS ^= SH;

  // Wait the reaminder of 4uS.
  _delay_loop_1(23);

  // Start the readout loop at the first pixel.
  CLOCKS |= (RD + CNVST + ICG + BYTESWAP + SH);
  __asm__("nop");
  
  do {
    // Wait a minimum of 250nS for acquisition.
    _delay_loop_1(2);

    // Start the conversion.
    CLOCKS &= ~CNVST;
    CLOCKS |= CNVST;

    // Wait a minimum of 1uS for conversion.
    _delay_loop_1(4);

    // Read the low byte of the result.
    CLOCKS &= ~RD;
    _delay_loop_1(4);

    *buf++ = DATA_PINS;

    // Setup and read the high byte.
    CLOCKS &= ~(BYTESWAP);
    _delay_loop_1(4);

    *buf++ = DATA_PINS;

    // Set the clocks back to idle state
    CLOCKS |= (RD + BYTESWAP);

    // Toggle SH for the next pixel.
    CLOCKS ^= SH;

  } while (++x < PIXEL_COUNT);
  
  sei();
}

/*
 * clearLine() Clears the CCD.
 */

void clearLine() {
  
  int x = 0;

  // Set ICG low.
  CLOCKS &= ~ICG;
  CLOCKS |= SH;
  _delay_loop_1(14);

  // Set SH low.
  CLOCKS ^= SH;
  _delay_loop_1(10);

  // Reset the timer so the edges line up.
  TCNT2 = 0;
  
  CLOCKS |= (RD + CNVST + ICG + BYTESWAP + MCLK);
  
  do {
    CLOCKS ^= SH;
    _delay_loop_1(10);
    
  } while (++x < PIXEL_COUNT);

}

/*
 * sendLine() Send the line of pixels to the user.
 */
void sendLine() {
  uint16_t x;

  for (x = 0; x < PIXEL_COUNT; ++x) {
    Serial.print(x);
    Serial.print(",");
    Serial.print(pixBuf[x]);
    Serial.print("\n");
  }
}

/*
 * setup()
 * Set the data port to input.
 * Set the clock port to output.
 * Start timer2 generating the Mclk signal
 */

void setup() {
  delay(10);
  CLOCKS_DDR = 0xff;
  CLOCKS = 0; //clocks0;
  DATA_DDR = 0x0;
  Serial.begin(115200);

  // Setup timer2 to generate an 888kHz frequency on D10
  TCCR2A = (0 << COM2A1) | (1 << COM2A0) | (1 << WGM21) | (0 << WGM20);
  TCCR2B = (0 << WGM22) | (1 << CS20);

  OCR2A = 8;
  TCNT2 = 0;
  delay(10);
}

/*
 * loop()
 * Read the CCD continuously.
 * Upload to user on switch press.
 */

void loop() {

  int x;
  char ch;

  // If we got a command last time, execute it now.
  if (cmdRecvd) {
    if (cmdBuffer[0] == 'r') {
      
      // Send the readout to the host.
      sendLine();
    } else if (cmdBuffer[0] == 'e') {
      delay(10);
      Serial.write(cmdBuffer);
      // Set the exposure time.
      sscanf(cmdBuffer + 1, "%d", &exposureTime);
      if (exposureTime > 1000) exposureTime = 1000;
      if (exposureTime < 1) exposureTime = 1;
    }
    
    // Get ready for the next command.
    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    cmdIndex = 0;
    cmdRecvd = 0;
  }
  // Clear the CCD.
  clearLine();

  // Integrate.
  delay(exposureTime);

  // Read it for real.
  readLine();

  // See if the host is talking to us.
  if (Serial.available()) {
    ch = Serial.read();

    // If char is linefeed, it is end of command.
    if (ch == 0x0a) {
      cmdBuffer[cmdIndex++] = '\0';
      cmdRecvd = 1;

    // Otherwise it is a command character.
    } else {
      cmdBuffer[cmdIndex++] = ch;
      cmdRecvd = 0;
    }
  }
}
#include "adc.h"
//Beginning of Auto generated function prototypes by Atmel Studio
void readLine();
void clearLine();
void sendLine();
int main(void );
//End of Auto generated function prototypes by Atmel Studio



#define ADC_INPUT      ADC_MUX_ADC0
#define ADC_VREF           ADC_VREF_AVCC

int main(void)
{
  // set PORTB as output
  DDRB = 0xFF;

  // set prescaler and enable ADC
  adc_init(ADC_PRESCALER_DIV128);

  // output inverted ADC value on PORTB
  while (1) {
    PORTB = ~(adc_read_8bit(ADC_INPUT, ADC_VREF));
  }
}

