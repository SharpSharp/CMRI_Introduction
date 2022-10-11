/**
 * CMRI module   
 * ======================================
 * 
 * Set to board 1
 * 
 * CMRI inputs and outputs in the 1000 range
 * 
 * Turnouts are set up in JMRI as 2 bit and pulsed output
 */
#include <Auto485.h>
#include <CMRI.h>
#include <Servo.h>

Auto485 bus(3);             // Pin 3 in for MAX485 (2–DE). Used along with Pins 0–RX and 1–TX.

CMRI cmri (1, 24, 48, bus); // 1 is node address specific to this Arduino added to the system

const byte NUMBER_OF_TURNOUTS = 3;
#define TURNOUT_MOVE_SPEED 10              // [ms] lower number is faster
bool servoMoved;

#define sidingButtonPin    4
#define crossoverButtonPin 5

#define sidingTurnout    0
#define crossoverTurnout 1

struct ServoTurnout
{
  int cmriBit;
  int servoPin;
  int closedPosition;
  int thrownPosition;
  int linkedTurnout;
  Servo servo;
  int targetPosition;
  int currentPosition;
  unsigned long moveDelay;
  bool isClosed;

  void setup()
  {
    targetPosition = closedPosition;       // setup targets as closed positiion
    currentPosition = closedPosition;      // setup position to closed
    isClosed = true;                       // setup closed is true
    cmri.set_bit(cmriBit,   LOW);
    cmri.set_bit(cmriBit+1, HIGH);

    servo.attach(servoPin);                // attach all the turnout servos
    servo.write(currentPosition);          // write the servos poistions

    delay(200);
  }

  void moveServo();

  void setClosedTarget()
  {
    targetPosition = closedPosition;
    cmri.set_bit(cmriBit, LOW);            // Change sensor as turnout is no longer thrown
  }

  void setThrownTarget()
  {
    targetPosition = thrownPosition;
    cmri.set_bit(cmriBit+1, LOW);            // Change sensor as turnout is no longer thrown
  }

  void toggleTarget()
  {
    if (isClosed) setThrownTarget(); // set turnout target to thrown if it is closed
    else          setClosedTarget(); // set turnout target to closed if it is thrown
  }
};

// define the turnouts with 
// CMRI, pin, closed, thrown, linked, 
ServoTurnout turnouts[NUMBER_OF_TURNOUTS] = {
    {0, 11, 70, 110},
    {2, 6,  60, 100, 2},
    {4, 7,  80, 120}
};

void ServoTurnout::moveServo()
{
  if (currentPosition != targetPosition) { // check that they are not at there taregt position
    servoMoved = true;
    if (millis() > moveDelay) {            // check that enough delay has passed
      moveDelay = millis() + TURNOUT_MOVE_SPEED;
      if (currentPosition < targetPosition) currentPosition++;
      if (currentPosition > targetPosition) currentPosition--;
      servo.write(currentPosition);    
      if (currentPosition == closedPosition) 
      {          
        isClosed = true;
        cmri.set_bit(cmriBit+1, HIGH);
      }     
      if (currentPosition == thrownPosition) 
      {
        isClosed = false;
        cmri.set_bit(cmriBit, HIGH);
      }

      if (linkedTurnout > 0) {
        if (currentPosition == closedPosition) {
          turnouts[linkedTurnout].setClosedTarget();
        }
        if (currentPosition == thrownPosition) {
          turnouts[linkedTurnout].setThrownTarget();
        }
      }
    }
  }
}

void setup() 
{
  bus.begin(9600, SERIAL_8N2); // start talking at 9600bps

  pinMode(sidingButtonPin,    INPUT_PULLUP);
  pinMode(crossoverButtonPin, INPUT_PULLUP);
  
  for (int i = 0; i < NUMBER_OF_TURNOUTS; i++) turnouts[i].setup();
}

void loop() 
{
  // 1: build up a packet
  cmri.process();
  
/*--------------------------------------------------------------------*/     
  // 2: update outputs from CMRI
  // check to see if CMRI is sending a turnout bit
  if (cmri.get_bit(0)) turnouts[sidingTurnout].setClosedTarget();      // 1001 siding turnout closed
  if (cmri.get_bit(1)) turnouts[sidingTurnout].setThrownTarget();      // 1002 siding turnout thorwn
  if (cmri.get_bit(2)) turnouts[crossoverTurnout].setClosedTarget();   // 1003 crossing turnout closed
  if (cmri.get_bit(3)) turnouts[crossoverTurnout].setThrownTarget();   // 1003 crossing turnout thrown
  
/*--------------------------------------------------------------------*/
  // 3: check "BOB" buttons for either push button
  if (digitalRead(sidingButtonPin) == LOW)    turnouts[sidingTurnout].toggleTarget();
  if (digitalRead(crossoverButtonPin) == LOW) turnouts[crossoverTurnout].toggleTarget();


/*--------------------------------------------------------------------*/
  // 4: move turnouts
  servoMoved = false;
  for (int i = 0; i < NUMBER_OF_TURNOUTS; i++)  
  {
    if (!servoMoved) turnouts[i].moveServo();
  }
}
