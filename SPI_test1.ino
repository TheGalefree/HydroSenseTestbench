#include "SPI.h"


int CS = 10;
//MOSI = 11;
//MISO = 12;
//CK = 13;

void setup() {
Serial.begin(19200);
SPI.begin();
pinMode(CS, OUTPUT);
digitalWrite(CS,HIGH);


}

void SPI_getValue()
{
  SPI.beginTransaction (SPISettings (19200, MSBFIRST, SPI_MODE0));
  digitalWrite(CS, LOW);
  SPI.transfer(0x01); // send command byte
  byte firstHalf = SPI.transfer(0xE0);
  byte secondHalf = SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
  //firstHalf -= 0xE0;
  Serial.print(firstHalf, HEX);
  Serial.println(secondHalf, HEX);
  SPI.endTransaction();
}


void loop() {

  SPI_getValue();
  delay(200);

}
