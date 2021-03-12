boolean toggle1 = 0;

//Note: This process takes enough time that it maxes at ~117kHz interupt frequency

void setup(){
  
  //set pins as outputs
  pinMode(8, OUTPUT);

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

}//end setup

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1Hz toggles pin 13 (LED)
//generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
  if (toggle1){
    digitalWrite(8,HIGH);
    toggle1 = 0;
  }
  else{
    digitalWrite(8,LOW);
    toggle1 = 1;
  }
}

void loop() {
  

}
