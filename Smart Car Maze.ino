/*
  Final Maze Demo
  Code that is used to guide car through final maze
  By: Emil, Danny, Vaiva, Peter
  Written: May 1, 2023
  Edited: May 8, 2023
  I/O Pins
  A0: Left bumper
  A1: Right bumper
  A2: 
  A3: 
  A4:
  A5:
  D0:  
  D1:  
  D2:  
  D3: right forward PWM
  D4:
  D5: Left forward PWM 
  D6: Left reverse PWM
  D7: Right wheel encoder
  D8: Left wheel encoder
  D9: 
  D10: 
  D11: right reverse PWM
  D12: 
  D13:
*/


void setup() {
  cli(); // disables intterupts
  DDRB = 0x08;    //Sets output for PWM pin
  DDRD = 0x68;    //Sets output for PWM pin
  DDRC = 0x30;    //Sets output for LED blinkers
  TCCR0A = 0xA1;  //TC in non-inverting mode, phase correct, counting to OCR0A, Prescaler 1
  TCCR0B = 0x01;  //TC in non-inverting mode, phase correct, counting to OCR0B, Prescaler 1
  TCCR2A = 0xA1;  //TC in non-inverting mode, phase correct, counting to OCR2A, Prescaler 1
  TCCR2B = 0x01;  //TC in non-inverting mode, phase correct, counting to OCR2B, Prescaler 1
  PCICR = 0x07;   // PORTB, PORTC, PORTD pin change interrupts
  PCMSK1 = 0x03;  //pin change interrupt A0, A1
  PCMSK0 = 0x01;  //pin D8 pin change interrrupt
  PCMSK2 = 0x80;  //pin D7 pin change interrupt
  PORTB |= 0x01;  // internal pull ups for wheel encoders
  PORTD |= 0x80;
  WDTCSR |= 0x58; // sets up watchdog timer register
  WDTCSR = 0x5E; //sets prescalar for 1 second
  sei(); // enables interrupts
}

volatile unsigned char X = 0; // case variables
volatile unsigned char Y = 0; // variable to for logic between 90 and 180 degrees 
volatile unsigned long olddist = 0;  // used to calculate distdiff
volatile unsigned long newdist = 0;   // used to calculate distdiff
volatile unsigned long distdiff = 0;  //variable used to reset logic for 90/180 degree turn scenario when needed
volatile unsigned long leftWheel = 0; // used with wheel encoders to count left wheel rotations
volatile unsigned long rightWheel = 0;  // used with wheel encoders to count right wheel rotations
volatile unsigned long distance = 0; // variable to calculate distance
volatile unsigned long avg = 0; // variable to calculate average from wheel encoders
volatile unsigned long turns = 0; // used to calculate distance


void LeftAdjust() {  //right bumper hit function
  unsigned long newA = distance + 37; // variable that tells how much the car should turn left 
  while (distance < newA) { // waits until distance meets newA value (once slight left turn is completed)
    OCR0A = 50;
    OCR0B = 0;
    OCR2A = 0;
    OCR2B = 180;
  }
}

void RightAdjust() {  //left bumper hit function
  unsigned long newB = distance + 37; // variable that tells how much the car should turn right
  while (distance < newB) {  // waits until distance meets newB value (once slight left turn is completed)
    OCR0A = 0;
    OCR2B = 0;
    OCR2A = 50;
    OCR0B = 180;
  }
}

void NinetyTurn() { // 90 degree turn function
  unsigned long newC = distance + 95;  // variable that tells how much the car should turn right
  while (distance < newC) { // waits until distance meets newC value (once 90 degree turn is completed)
    OCR0B = 170;
    OCR2B = 0;
    OCR0A = 0;
    OCR2A = 170;
  }
}

void OneEightTurn() { // 180 degree turn function
  unsigned long newD = distance + 100;  // variable that tells how much the car should turn right
  while (distance < newD) { // waits until distance meets newD value (once 180 degree turn is completed)
    OCR0B = 160;
    OCR2B = 0;
    OCR0A = 0;
    OCR2A = 160;
  }
}

void backwards() { // backwards function
  unsigned long newE = distance + 37;  // variable that tells how much the car go backwards
  while (distance < newE) { // waits until distance meets newE value (once car goes backwards a certain distance)
    OCR0A = 150;
    OCR0B = 0;
    OCR2A = 175;
    OCR2B = 0;
  }
}
void backwardsNinety() { // backwards 90 degree function (less distance than the seperate backwards function)
  unsigned long newF = distance + 23; // variable that tells how much distance the car should move backwards
  while (distance < newF) { // waits until distance meets newF value (once 180 degree turn is completed)
    OCR0A = 170;
    OCR0B = 0;
    OCR2A = 155;
    OCR2B = 0;
  }
}

void Straight() { // straight line function (left and right wheel go forward)
  OCR0A = 0;
  OCR0B = 210; 
  OCR2A = 0;
  OCR2B = 225;
}

void loop() {

  switch (X) {
    default:  //default (going straight)
      Straight();
      break;

    case 1:  //left adjust
      backwards();
      LeftAdjust();
      X = 0;
      break;

    case 2:  //right adjust
      backwards();
      RightAdjust();
      X = 0;
      break;

    case 3:  //90 degree turn
      backwardsNinety();
      NinetyTurn();
      olddist = distance;
      X = 0;
      break;

    case 4:  //180 degree turn
      backwardsNinety();
      OneEightTurn();
      X = 0;
      break;
  }
}

ISR(PCINT1_vect) {
  _delay_ms(120);                         //small delay so car can properly sense bumper values
  unsigned char LeftBump = PINC & 0x01;   //masking Left Bumper into variable
  unsigned char RightBump = PINC & 0x02;  //masking Right Bumper into variable
  RightBump >> 1;                         //shifting to LSB

  newdist = distance;  // current distance for distdiff calculation
  distdiff = newdist - olddist; // calulates distance difference by subtracting olddist from newdist
  if (distdiff > 200) { // resets to 90/180 degree turn logic if distdiff is greater than 200
    Y = 0;
  }

  if (RightBump && !LeftBump) {  //If right bumper is triggered
    X = 1;
  } else if (LeftBump && !RightBump) {  //If left bumper is triggered
    X = 2;
  } else if (LeftBump && RightBump) {  //If both bumpers are triggered
    X = 3;
    Y += 1; // counted up for possible 180 degree turn
  } else {
    X = 0;
  }
  if (Y >= 2) {  //If both bumpers are triggered for a second time (after 180 degree turn)
    X = 4;
    Y = 0; // resets to default (straight line) after
  }
}

ISR(PCINT0_vect) {  //ISR for left wheel encoder on pin D8
  asm volatile("wdr"); // calls watchdog timer reset (1 second)
  if (!(PINB & 0x01)) {
    leftWheel += 1;  // count up left wheel rotations
  }
  avg = ((leftWheel + rightWheel) / 2);  //Adds both wheel values and divides to 2 to find avg
  turns = ((avg * 100L) / 192);          //converts average into percentage of wheel turned
  distance = (turns * 2015L / 1000);     //finds the distance the car traveled in MM
}

ISR(PCINT2_vect) {  //ISR for right wheel encoder D7
  if (!(PIND & 0x80)) {
    rightWheel += 1; // count up right wheel rotations
  }
  avg = ((leftWheel + rightWheel) / 2);  //Adds both wheel values and divides to 2 to find avg
  turns = ((avg * 100L) / 192);          //converts average into percentage of wheel turned
  distance = (turns * 2015L / 1000);     //finds the distance the car traveled in MM
}

ISR(WDT_vect){ // watchdog timer ISR
  unsigned long newG = distance + 17;  // variable that tells how much the car should go backwards
  while (distance < newG) { // waits until distance meets the newG value (once car goes back a certain distance)
    OCR0A = 150;
    OCR0B = 0;
    OCR2A = 175;
    OCR2B = 0;
  }
}