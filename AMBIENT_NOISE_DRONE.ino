/*****************************************************************************************************

    PROJECT      : NOISE ENGINE DEMO
    Designed by : Ryan Liston
    Date        : January 9, 2025
    Description : Ambient drone synth.
    Features    : random frequency noise generator with depth, color and density controls
                : lfo with speed control and shape selection    
                : 4 bit audio output for amplitute contol   
    Target      : Arduino Nano
    Note        : This was written for a Nano clone, port designations may vary


EEEEEEEE    NN      NN    NNNNNNNN      NNNN      YY    YY   !!   !!    !!
EE          NNNN    NN        NN      NN    NN    YY    YY   !!   !!    !!
EEEEEE      NN  NN  NN        NN      NN    NN     YYYYYY    !!   !!    !!
EE          NN    NNNN    NN  NN      NN    NN       YY
EEEEEEEE    NN      NN      NN          NNNN         YY      !!   !!    !! 


*****************************************************************************************************/

//  INCLUDE LIBRARIES

#include <avdweb_AnalogReadFast.h>

// I use the analog read fast library. This program should still run fine if you want to use regular analogRead functions.

/****************************************************************************************************/

//  GLOBALS

const uint8_t OUT_PUT = 0b11110000;  // bit mask for audio output

// variables activley shared betwwen main loop and ISR declared as volalitle

volatile uint8_t depth;    //  noise depth (lowest tone)
volatile uint8_t color;    //  noise color (range of tones to the highest tone from the lowest)
volatile uint8_t density;  //  noise demsity (interval density of tones between lowest and highest)
volatile uint8_t volume;   //  volume level

/****************************************************************************************************/

//    TIMER 2 A ISR

ISR(TIMER2_COMPA_vect) {

  //Frequncy count values are listed in an array and accessed by the routine

  static const uint16_t frequencyTable[96]{

    0x0FFF, 0x0F1A, 0x0E41, 0x0D74, 0x0CB2, 0x0BFC, 0x0B50, 0x0AAD, 0x0A14, 0x0983, 0x08FA, 0x0879,
    0x07FF, 0x078D, 0x0720, 0x06BA, 0x0659, 0x05FE, 0x05A8, 0x0556, 0x050A, 0x04C1, 0x047D, 0x043C,
    0x03FF, 0x03C6, 0x0390, 0x035D, 0x032C, 0x02FF, 0x02D4, 0x02AB, 0x0285, 0x0260, 0x023E, 0x021E,
    0x01FF, 0x01E3, 0x01C8, 0x01AE, 0x0196, 0x017F, 0x016A, 0x0155, 0x0142, 0x0130, 0x011F, 0x010F,
    0x00FF, 0x00F1, 0x00E4, 0x00D7, 0x00CB, 0x00BF, 0x00B5, 0x00AA, 0x00A1, 0x0098, 0x008F, 0x0087,
    0x007F, 0x0078, 0x0072, 0x006B, 0x0065, 0x005F, 0x005A, 0x0055, 0x0050, 0x004C, 0x0047, 0x0043,
    0x003F, 0x003C, 0x0039, 0x0035, 0x0032, 0x002F, 0x002D, 0x002A, 0x0028, 0x0026, 0x0023, 0x0021,
    0x001F, 0x001E, 0x001C, 0x001A, 0x0019, 0x0017, 0x0016, 0x0015, 0x0014, 0x0013, 0x0011, 0x0010

  };

  /****************************************************************************************************/

  //  RANDOM FREQUENCY GENERATOR
  //  Frequencies are clocked by counting down everytime the ISR is triggered

  static uint16_t frequencyTimer = 0;              //  clocks freqency
  static uint16_t frequencyFlag = 0;               //  compared to frequency clock to check for new frequency
  static uint8_t frequencyMask = OUT_PUT;          //Inverts against oscMask and ANDs against volume to generate frequency oscilation between 0 and volume
  static const uint8_t portMask = ~OUT_PUT;        // masks out bits from portb to remain unaltered
  static const uint8_t oscillationMask = OUT_PUT;  // oscilation mask

  if (frequencyTimer == frequencyFlag) {  //  compare timer to flag

    static uint8_t frequencyPointer;  // addresses frequency from frequencyTable

    frequencyTimer = 0;                                                //reset timer
    frequencyPointer = depth + ((random(color) / density) * density);  // calculate the next frequency

    if (frequencyPointer > 95) {  //  check frequency pointer

      frequencyPointer = 95;  // limit to a maximum of 95
    }
    frequencyFlag = frequencyTable[frequencyPointer];      //  retrieve next frequency from table
    frequencyMask ^= oscillationMask;                      //  invert frequncy mask
    PORTD = (PIND & portMask) | (volume & frequencyMask);  // mask volume with frequency and push to port

  } else {

    frequencyTimer++;  //increment timer
  }
}

/****************************************************************************************************/

//  SETUP

void setup() {

  DDRD = (DDRD & ~OUT_PUT) | OUT_PUT;  // set  port d pins 4,5,6 and 7 to output
  cli();                               //clear interrupts
  TCCR2A = 0b0000010;                  // set timer to compare timer on match (CTC) mode
  TCCR2B = 0b0000001;                  // set timer presccaler to 0
  OCR2A = 127;                         // set compare register clock value
  TIMSK2 = 0b00000010;                 // enable timer 2 A for CTC mode
  sei();                               // start interrupts
}

/****************************************************************************************************/

//  MAIN LOOP

void loop() {

  /****************************************************************************************************/

  //  LFO SHAPE TABLE
  //  values are read left to right by the lfo step counter
  //  input expects values from 0-f (0-15) 0=min , f=max

  static const uint8_t shapes[8][32]{

    { 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },  //  [0] Pulse 1
    { 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },  //  [1] pulse 2
    { 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },  //  [2] Square
    { 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },  //  [3] rectangle 1
    { 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0 },  //  [4] rectangle 2
    { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0x0 },  //  [5] triangle
    { 0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },  //  [6] Reverse Ramp
    { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }   //  [7]  Ramp
  };

  /****************************************************************************************************/

  //INPUT

  //  assign potentiometer controls

  static const uint8_t depthPot = A0;
  static const uint8_t colorPot = A1;
  static const uint8_t densityPot = A2;
  static const uint8_t shapePot = A3;
  static const uint8_t speedPot = A4;

  //  input values
  static uint8_t depthInput;
  static uint8_t colorInput;
  static uint8_t densityInput;
  static uint8_t volumeInput;
  static uint8_t shapeInput;
  static uint8_t speedInput;

  // get input values from potentiometers
  depthInput = analogReadFast(depthPot) / 10.76;         //  depth intput  (0-95)
  colorInput = analogReadFast(colorPot) >> 4;            //  color input   (0-63)
  densityInput = 1 + (analogReadFast(densityPot) >> 5);  // density input (1-128)
  shapeInput = analogReadFast(shapePot) >> 7;            //  shape input (0-7)
  speedInput = analogReadFast(speedPot) >> 2;            //  speed input (0-255)
  /****************************************************************************************************/

  //  LFO ROUTINE

  static uint32_t clock = millis() + speedInput;  // LFO clock timer
  static uint8_t counter = 0;                     //    LFO shape step counter

  if (millis() > clock) {  //Check LFO Timer

    static uint8_t nextVolume;  // used to proccess volume

    clock = millis() + speedInput;  //  set LFO clock

    if (counter > 30) {  // check step counter

      counter = 0;  //  reset step counter
    } else {
      counter++;  //  increment step counter
    }
    nextVolume = shapes[shapeInput][counter];  // retrieve nextVolume from shape table
    volumeInput = (nextVolume << 4);           // set volume and push to top 4 bits for port output
  }

  /****************************************************************************************************/

  // SEND NEW PARAMETERS

  if ((depth != depthInput) || (color != colorInput) || (density != densityInput) || (volume != volumeInput)) {  //  check for parameter change

    cli();  // clear interrupts

    //  send new parameters to ISR

    depth = depthInput;
    color = colorInput;
    density = densityInput;
    volume = volumeInput;

    sei();  //  set interrupts
  }
}

/****************************************************************************************************/