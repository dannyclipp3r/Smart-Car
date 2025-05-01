/*
  Line Follower Demo Final (Fast)
  By: Emil, Daniel, Vaiva, Peter
  Written: May 8, 2023
  I/O Pins
  A0: Middle Line following Sensor
  A1: Left Line following Sensor
  A2: Right Line following sensor
  A3: RS LCD
  A4: PIN 5 LCD
  A5:
  D0:
  D1:
  D2:
  D3: Right forward PWM
  D4: 
  D5: Left forward PWM
  D6: Left reverse PWM
  D7: Right Wheel encoder
  D8: Left Wheel encoder
  D9: E LCD
  D10: PIN 4 LCD
  D11: Right reverse PWM
  D12: PIN 6 LCD 
  D13: PIN 7 LCD
*/

// include the library that contains all of the LCD functions and constants
#include "hd44780.h"

void setup() {
  cli();
  DDRB = 0x08;    //Sets output for PWM pin
  DDRD = 0x68;    //Sets output for PWM pin
  TCCR0A = 0xA1;  //TC in non-inverting mode, phase correct, counting to OCR0A, Prescaler 1
  TCCR0B = 0x01;  //TC in non-inverting mode, phase correct, counting to OCR0B, Prescaler 1
  TCCR2A = 0xA1;  //TC in non-inverting mode, phase correct, counting to OCR2A, Prescaler 1
  TCCR2B = 0x01;  //TC in non-inverting mode, phase correct, counting to OCR2B, Prescaler 1
  ADCSRA = 0xEF;
  ADCSRB = 0x00;
  ADMUX = 0x40;   //initializes A0
  PCICR = 0x05;   // PORTB, PORTD pin change interrupts
  PCMSK0 = 0x01;  //pin D8 pin change interrrupt
  PCMSK2 = 0x80;  //pin D7 pin change interrupt
  PORTB |= 0x01;  // internal pull ups for wheel encoders
  PORTD |= 0x80;
  lcd_init();
  sei();
  Serial.begin(9600);  // Debugging Purposes
}

volatile unsigned int MiddleSensor = 0;                    //A0
volatile unsigned int LeftSensor = 0;                      //A1
volatile unsigned int RightSensor = 0;                     //A2
volatile unsigned char X = 0;                              //Dummy variable for array position
volatile unsigned char MuxArray[] = { 0x40, 0x41, 0x42 };  // Creates an array for ADC values
volatile unsigned long leftWheel = 0;
volatile unsigned long rightWheel = 0;
volatile unsigned long distance = 0;
volatile unsigned long avg = 0;
volatile unsigned long turns = 0;

void loop() {
  static unsigned char Y = 0;  //Dummy variable

  lcd_clrscr();
  char DistanceLCD[11];
  ltoa(distance, DistanceLCD, 10);
  lcd_puts("Distance in MM");
  lcd_goto(0x40);
  lcd_puts(DistanceLCD);

  if ((MiddleSensor > 900) & (LeftSensor < 900) & (RightSensor < 900)) {  // is if the line is straight
    Y = 0;
  }
  if (LeftSensor > 900) {  // if only the left sensor on the line (Softer turn)
    Y = 1;
  }
  if (RightSensor > 900) {  // if only the right sensor on the line (Softer turn)
    Y = 2;
  }
  if ((RightSensor < 900) & (MiddleSensor < 900)) {  //If the right and middle sensor are off the line (Sharper turn)
    Y = 3;
  }
  if ((LeftSensor < 900) & (MiddleSensor < 900)) {  //If the left and middle sensor are off the line   (Sharper turn)
    Y = 4;
  }
  if ((RightSensor < 900) & (LeftSensor < 900) & (MiddleSensor < 900)) {  //if no line is sensed, on white background, car will stop
    Y = 5;
  }

  switch (Y) {
    default:  //default is going straight
      OCR0A = 0;
      OCR0B = 255;  // Left wheel forward
      OCR2A = 0;
      OCR2B = 255;  // Right wheel forward
      break;

    case 1:  //left adjust
      OCR0A = 0;
      OCR0B = 180;
      OCR2A = 0;
      OCR2B = 225;  // Right wheel forward
      Y = 0;
      break;

    case 2:  //right adjust
      OCR0A = 0;
      OCR2B = 180;
      OCR2A = 0;
      OCR0B = 225;  // Left wheel forward
      Y = 0;
      break;

    case 3:                         // hard turn left
      while (MiddleSensor < 900) {  // car will keep turning left until middle sensor on the line
        OCR0B = 0;                // Left wheel forward
        OCR2B = 255;                // Right wheel forward
        OCR0A = 20;
        OCR2A = 0;
      }
      Y = 0;
      break;

    case 4:                         // hard turn right
      while (MiddleSensor < 900) {  // car will keep turning right until middle sensor on the line
        OCR0B = 255;                // Left wheel forward
        OCR2B = 0;                // Right wheel forward
        OCR0A = 0;
        OCR2A = 20;
      }
      Y = 0;
      break;

    case 5:  // no line detected (Car stops)
      OCR0B = 0;
      OCR2B = 0;
      OCR0A = 0;
      OCR2A = 0;
      Y = 0;
      break;
  }
}

ISR(ADC_vect) {  //ISR changes value of admux and assigns the ADC conversion into the correct variable
  ADMUX = MuxArray[X];
  if (ADMUX == 0x40) {
    MiddleSensor = ADC;
  } else if (ADMUX == 0x41) {
    LeftSensor = ADC;
  } else if (ADMUX == 0x42) {
    RightSensor = ADC;
  }
  X++;
  if (X == 3) {
    X = 0;
  }
}

ISR(PCINT0_vect) {  //ISR for left wheel encoder on pin D8
  if (!(PINB & 0x01)) {
    leftWheel += 1;
  }
  avg = ((leftWheel + rightWheel) / 2);  //Adds both wheel values and divides to 2 to find avg
  turns = ((avg * 100L) / 192);          //converts average into percentage of wheel turned
  distance = (turns * 2015L / 1000);     //finds the distance the car traveled in MM
}

ISR(PCINT2_vect) {  //ISR for right wheel encoder D7
  if (!(PIND & 0x80)) {
    rightWheel += 1;
  }
  avg = ((leftWheel + rightWheel) / 2);  //Adds both wheel values and divides to 2 to find avg
  turns = ((avg * 100L) / 192);          //converts average into percentage of wheel turned
  distance = (turns * 2015L / 1000);     //finds the distance the car traveled in MM
}
