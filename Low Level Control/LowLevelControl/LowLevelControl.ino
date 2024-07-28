
#define HOVER_SERIAL_BAUD   115200      // [-] Baud rate for HoverSerial (used to communicate with the hoverboard)
#define SERIAL_BAUD         9600      // [-] Baud rate for built-in Serial (used for the Serial Monitor)
#define START_FRAME         0xABCD       // [-] Start frme definition for reliable serial communication
#define TIME_SEND           100         // [ms] Sending time interval
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial HoverSerial(16,17);        // RX, TX

Servo ESC1; // Create a servo object to control the first ESC
Servo ESC2; // Create a servo object to control the second ESC

// Global variables
unsigned long timeNow=0;
unsigned long iTimeSend = 0;
const int threshold = 320; // Assuming the frame width is 640 pixel
int x_old= 300;
int x_new = 300;
int input;
int no_input_old=1;
int steer;
int speed;
int no_input=1;
int flag = 0;
//For BLDC
const int escPin1 = 9; // Digital pin connected to the first ESC
const int escPin2 = 10; // Digital pin connected to the sec ond ESC
const int pulseMin = 1000; // Minimum pulse width
const int pulseMax = 2000; // Maximum pulse width
int minimumSpeed = 7; // Minimum possible speed value just above 0
int repeated_value=0;

typedef struct{
   uint16_t start;
   int16_t  steer;
   int16_t  speed;
   uint16_t checksum;
} SerialCommand;
SerialCommand Command;

// ########################## SEND ##########################
void Send(int16_t uSteer, int16_t uSpeed)
{
  // Create command
  Command.start    = (uint16_t)START_FRAME;
  Command.steer    = (int16_t)uSteer;
  Command.speed    = (int16_t)uSpeed;
  Command.checksum = (uint16_t)(Command.start ^ Command.steer ^ Command.speed);

  // Write to Serial
  Serial2.write((uint8_t *) &Command, sizeof(Command)); 
}

// ########################## CALIBRATE ##########################
void calibrateESCs() {
  Serial.println("Calibrating ESCs...");
  
  // Set both ESCs to full throttle
  Serial.println("Setting full throttle");
  ESC1.write(180); // Full throttle (maximum value)
  ESC2.write(180); // Full throttle (maximum value)
  delay(2000); // Wait for 2 seconds

  // Set both ESCs to minimum throttle
  Serial.println("Setting minimum throttle");
  ESC1.write(0); // Minimum throttle (minimum value)
  ESC2.write(0); // Minimum throttle (minimum value)
  delay(2000); // Wait for 2 seconds

  Serial.println("ESCs Calibrated.");
}

void right(void){
  // Object is to the right of the center 
  if (iTimeSend > timeNow) return;
  iTimeSend = timeNow + TIME_SEND;
  steer = 25;
  speed = -1;
  Send(steer,speed);
  Serial.print("Moving Right\n");
}

void left(void){
  //Object is to the left of the center
  if (iTimeSend > timeNow) return;
  iTimeSend = timeNow + TIME_SEND;
  steer = -25;
  speed = -1;
  Send(steer,speed);
  Serial.print("Moving Left\n");
}

void forward(void){
  //Object is centered
  if (iTimeSend > timeNow) return;
  iTimeSend = timeNow + TIME_SEND;
  steer = 1;
  speed = 25;
  Send(steer,speed);
  Serial.print("Moving Forward\n");
}

void run_rollers(void){
  ESC1.write(minimumSpeed); // Set the first ESC to the minimum possible speed
  ESC2.write(minimumSpeed); // Set the second ESC to the minimum possible speed 
}

void stop_rollers(void){
  ESC1.write(0); // Set the first ESC to the minimum possible speed
  ESC2.write(0); // Set the second ESC to the minimum possible speed 
}
void receive(void){
    if (Serial.available() > 0) {
    input = Serial.parseInt();
    if(input == 0){
      x_new = 300;
      no_input = 1;
    }
    else{
      x_new = input;
      x_old = x_new;
      no_input = 0;
    }
  }
  else {
    x_new = x_old;
    no_input = 1;
  }
  Serial.print("Received x-coordinate: ");
  Serial.println(x_new);
  Serial.print("\n");
  Serial.print("No_input ");
  Serial.println(no_input);
  Serial.print("No_input_old ");
  Serial.println(no_input_old);
}

void normal_code(void){
    if (x_new < threshold - 150 && no_input==0) {
      left();
    }
    else if (x_new > threshold + 150 && no_input==0) {
      right();
    }
    else if(x_new>=170 && x_new<=470 && no_input==0){
      forward();
    }
}

void continue_forward(void){
  for(int i = 0; i<300;i++)
  {
    timeNow = millis();
    forward();
    Serial.print("i = ");
    Serial.println(i);
  }
}
void rotate(void){
  right();
}
// ########################## SETUP ##########################
void setup() 
{
  ESC1.attach(escPin1, pulseMin, pulseMax); // Attach the first ESC to pin 9 with 1000-2000us pulse width range
  ESC2.attach(escPin2, pulseMin, pulseMax); // Attach the second ESC to pin 10 with 1000-2000us pulse width range
  // Calibrate the ESCs
  calibrateESCs();
  delay(100);

  Serial.begin(SERIAL_BAUD);
  Serial.flush();
  Serial.println("Hoverboard Serial v1.0");
  delay(100);
  Serial2.begin(HOVER_SERIAL_BAUD);
  Serial2.flush();

}

void loop()
{
  //return the number of milliseconds at the time, the Arduino board begins running the current program.
  //This number overflows i.e. goes back to zero after approximately 50 days.
  timeNow = millis();

  //receiving the x-coordinate from the raspberry pi through serial communication
  receive();

  if(no_input==1 && no_input_old==1 && flag == 0){
    rotate();
    stop_rollers();
  }
  else if((no_input==1 && no_input_old==0) || flag == 1){
    //delay(500);
    receive();
    flag = 1;
      if(no_input == 1 && no_input_old == 1 && flag == 1){
        run_rollers();
        continue_forward();
        flag = 0;
      }
    }
  else{
    normal_code();
    run_rollers();
  }

  no_input_old=no_input;
}
