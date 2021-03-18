//This code will continuously take samples for 4 lines feeding into a MUX, which will be alternated between each sample (each line 100Hz)
//through an SPI input from an ADC chip. It will output

#include "SPI.h"

//pins
int CS = 10;
//MOSI = 11;
//MISO = 12;
//CK = 13;

int RX = 0;
int TX = 1;

#define NUM_SAMPLES 4
typedef uint16_t sample_t;
#define ESC 27
#define BEG 1
#define END 4

typedef struct {
  uint16_t sequence;
  sample_t samples[NUM_SAMPLES];
} adc_data_t;

static uint16_t sequence = 0;
static uint16_t sampleNum = 0;
static volatile bool send_flag = false;
static adc_data_t adc_data;
const adc_data_t *adc_ptr = &adc_data;
byte firstHalf = 0;
byte secondHalf = 0;

int sendPack(const adc_data_t* data) {
  Serial.write(ESC);
  Serial.write(BEG);
  for (size_t i = 0; i < sizeof(adc_data_t); i++) {
    if (data->samples[i] != ESC) {
      Serial.write(data->samples[i]);
    }
    else {
      Serial.write(ESC);
      Serial.write(data->samples[i]);
    }
  }
  Serial.write(ESC);
  Serial.write(END);
}

void setup() {

  Serial.begin(115200);
  pinMode(RX, INPUT); //Serial bus for output
  pinMode(TX, OUTPUT); //Note: For the UNO, it is also connected to the USB, see details above

  SPI.begin(); //enable SPI
  pinMode(CS, OUTPUT); //chip select for SPI
  digitalWrite(CS, HIGH); //disable CS for now

  cli();//stop interrupts

  //TCCR1A Register (Page 140) used to adjust timer MODE - we will use CTC mode which is 00 for WGM01:0 bits, and WGM02 in TCCR1B is set to 1
  TCCR1A = 0; // reset TCCR1A register to all 0s

  //TCCR1B Register (Page 142) used to set prescaler (1/8/64/256/1024) or external/no clock source
  TCCR1B = 0;// reset TCCR1B register to all 0s
  TCCR1B |= (1 << WGM12); // turn on CTC mode which is set in TCCR1B for timer 1
  TCCR1B |= (1 << CS10); //set to 1 prescaler for max resolution

  //TCNT1 Register (Page 143) is a combined 2 x 8-bit registers (TCNT1H and TCNT1L) that is constantly compared with OCR1A for CTC mode
  // and resets (and sends the interupt) as soon as the values match
  TCNT1  = 0; //initialize counter value to 0
  //OCR1A Register (Page 144) is a combined 2 x 8-bit registers (OCR1AH and OCR1AL) that is the limit for TCNT1 at which point it will reset
  //Use formula: OCR1A = [16,000,000Hz/(prescaler*desired interrupt frequency)]-1
  OCR1A = 19999; //(must be <65536 for timer1; 19,999 for 400Hz at prescaler of 1

  //TIMSK1 Register (page 144) chooses which function call to use for the interupt - COMPA or COMPB
  //Enable COMPA interupt function
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts

}

ISR(TIMER1_COMPA_vect) { //timer1 interrupt
  SPI.beginTransaction (SPISettings (1500000, MSBFIRST, SPI_MODE0));//settings for ADC chip in prototype1
  digitalWrite(CS, LOW); //enable
  SPI.transfer(0x01); // send command byte
  firstHalf = SPI.transfer(0xE0); //1110 0000 will return 4 bits of 0 and then 4 numerical bits
  secondHalf = SPI.transfer(0x00); //will return the next 8 bits
  digitalWrite(CS, HIGH); //disable
  SPI.endTransaction();
  adc_data.samples[sampleNum] = firstHalf | (uint16_t)secondHalf << 8;
  sampleNum++;
  if (sampleNum == NUM_SAMPLES) {
    send_flag = true;
    sampleNum = 0;
  }
  //switch MUX with GPIO
}

void loop() {

  while (send_flag == false) {
  }
  sendPack(adc_ptr);
  send_flag = false;

}
