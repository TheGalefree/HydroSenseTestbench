/*
   FT1.ino

    Created on: Mar 19, 2021
        Author: Julian Dubeau

        This code is meant for an Arduino Mega 2560 and uses an interupt sequence at a specified frequency to continuously
        take sample inputs through a MUX passing multiple measurement instruments. It uses SPI to communicate with an
        A/D converter chip (MCP3202) for each interupt and sends a flag to the main loop when a package is ready to be sent
        containing a sequence number followed by a specified number of samples encapsulated in a custom protocol defined below.
        The output package is sent using UART.
*/

#include "SPI.h"

//SPI pins for Arduino Mega 2560
int CS = 53;
//MOSI = 51;
//MISO = 50;
//CK = 52;

//UART1 pins for Arduino Mega 2560
int RX = 19;
int TX = 18;

//GPIO pins for MUX select
int S0 = 24;
int S1 = 25;
int S2 = 26;

#define BAUD_RATE 115200
#define NUM_SAMPLES 4 //number of measuring instruments to cycle sample inputs through the MUX
#define ESC 27
#define BEG 1
#define END 4
#define ZERO 0
typedef uint16_t sample_t;

typedef struct { //package for a sequence # + 1 measurement for each instrument
  uint16_t sequence;
  sample_t samples[NUM_SAMPLES];
} adc_data_t;

static uint16_t currentSequence = 0; //tracks the sequence # of the package being sent
static uint16_t sampleNum = 0; //tracks the current measurement instrument from which the sample is being taken (and place in samples array)
static volatile bool send_flag = false; //flag for when package is ready
static adc_data_t adc_data;
static const adc_data_t *adc_ptr = &adc_data; //const pointer to pass to the sendPack function
static byte firstHalf = 0; //in MSB mode first half of the 12 bit sample will contain the most significant 4 bits
static byte secondHalf = 0; //last 8 bits of sample

//Note: The UART receiving end on the BeagleBone uses a read function that is set to receive 2 bytes in least significant byte format.
//For this reason we must send each byte individually and command bytes are followed by a zero byte.
void sendPack(const adc_data_t* data) {
  Serial1.write(ESC); //begin protocol
  Serial1.write(ZERO);
  Serial1.write(BEG);
  Serial1.write(ZERO); //begin protocol

  Serial1.write(data->sequence); //send sequence number
  Serial1.write(data->sequence >> 8);
  if (data->sequence == ESC) {
    Serial1.write(ESC); //send another ESC if sequece is ESC
    Serial1.write(ZERO);
  }
  for (size_t i = 0; i < NUM_SAMPLES; i++) { //loop to send each sample
    if (data->samples[i] != ESC) {
      Serial1.write(data->samples[i]);
      Serial1.write(data->samples[i] >> 8 & 0x0f);
    }
    else {
      Serial1.write(ESC); //send ESC before data if data == ESC
      Serial1.write(ZERO);
      Serial1.write(data->samples[i]);
      Serial1.write(data->samples[i] >> 8 & 0x0f);
    }
  }

  Serial1.write(ESC); //end protocol
  Serial1.write(ZERO);
  Serial1.write(END);
  Serial1.write(ZERO); //end protocol
}

void setup() {
  Serial1.begin(BAUD_RATE); //set baud rate
  pinMode(RX, INPUT); //set UART1 pins
  pinMode(TX, OUTPUT);

  pinMode(S0, OUTPUT); //set MUX pins
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  digitalWrite(24, LOW); //initialize to 000
  digitalWrite(25, LOW);
  digitalWrite(26, LOW);

  SPI.begin(); //enable SPI
  pinMode(CS, OUTPUT); //chip select for SPI -> low for enable
  digitalWrite(CS, HIGH); //disable CS for now

  cli(); //stop interrupts

  //Here we adjust the registers for the interupt sequence. For details visit the Arduino ATmega datasheet (page references included):
  //https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf
  //TCCR1A Register (Page 140) used to adjust timer MODE - we will use CTC mode which is 00 for WGM01:0 bits, and WGM02 in TCCR1B is set to 1
  TCCR1A = 0; // reset TCCR1A register to all 0s

  //TCCR1B Register (Page 142) used to set prescaler (1/8/64/256/1024) or external/no clock source
  TCCR1B = 0;// reset TCCR1B register to all 0s
  TCCR1B |= (1 << WGM12); // turn on CTC mode which is set in TCCR1B for timer 1
  TCCR1B |= (1 << CS10); //set to prescaler of 1 for max resolution

  //TCNT1 Register (Page 143) is a combined 2 x 8-bit registers (TCNT1H and TCNT1L) that is constantly compared with OCR1A for CTC mode
  // and resets (and sends the interupt) as soon as the values match
  TCNT1  = 0; //initialize counter value to 0
  //OCR1A Register (Page 144) is a combined 2 x 8-bit registers (OCR1AH and OCR1AL) that is the limit for TCNT1 at which point it will reset
  //Use formula: OCR1A = [16,000,000Hz/(prescaler*desired interrupt frequency)]-1
  OCR1A = 39999; //(must be <65536 for timer1) 39999 for 400Hz at prescaler of 1

  //TIMSK1 Register (page 144) chooses which function call to use for the interupt - COMPA or COMPB
  //Enable COMPA interupt function
  TIMSK1 |= (1 << OCIE1A);

  sei(); //allow interrupts
}

ISR(TIMER1_COMPA_vect) { //timer1 interrupt
  SPI.beginTransaction (SPISettings (1500000, MSBFIRST, SPI_MODE0)); //settings for A/D converter chip MCP3202 max frequency of 1.8MHz at 5V power
  digitalWrite(CS, LOW); //enable
  SPI.transfer(0x01); // send command byte
  firstHalf = SPI.transfer(0xE0); //1110 0000 will return 4 bits of 0 and then 4 numerical bits
  secondHalf = SPI.transfer(0x00); //will return the next 8 bits
  digitalWrite(CS, HIGH); //disable
  SPI.endTransaction();
  adc_data.samples[sampleNum] = secondHalf | (uint16_t)firstHalf << 8; //concatenate bytes and save to samples array in package
  sampleNum++;
  if (sampleNum == NUM_SAMPLES) {
    send_flag = true;
    sampleNum = 0;
  }
  switch (sampleNum) { //switch MUX to sampleNum with GPIO
    case 0:
      digitalWrite(24, LOW);
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
      break;
    case 1:
      digitalWrite(24, HIGH);
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
      break;
    case 2:
      digitalWrite(24, LOW);
      digitalWrite(25, HIGH);
      digitalWrite(26, LOW);
      break;
    case 3:
      digitalWrite(24, HIGH);
      digitalWrite(25, HIGH);
      digitalWrite(26, LOW);
      break;
    case 4:
      digitalWrite(24, LOW);
      digitalWrite(25, LOW);
      digitalWrite(26, HIGH);
      break;
    case 5:
      digitalWrite(24, HIGH);
      digitalWrite(25, LOW);
      digitalWrite(26, HIGH);
      break;
    case 6:
      digitalWrite(24, LOW);
      digitalWrite(25, HIGH);
      digitalWrite(26, HIGH);
      break;
    case 7:
      digitalWrite(24, HIGH);
      digitalWrite(25, HIGH);
      digitalWrite(26, HIGH);
      break;
    default:
      //unrecognized value, it should never reach this.
      break;
  }
}

void loop() {
  while (send_flag == false) {
    //do nothing
  }
  //once send flag is true, send package
  adc_data.sequence = currentSequence; //save sequence number to package
  sendPack(adc_ptr); //send package
  currentSequence++; //increment sequence
  send_flag = false; //reset flag
}
