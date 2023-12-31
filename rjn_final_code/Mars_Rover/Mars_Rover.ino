/*
 *    Mars Rover - Arduino Code
 *    by Dejan, www.HowToMechatronics.com
 * 
 *   Libraries:
 *   ServoEasing: https://github.com/ArminJo/ServoEasing
 *   IBusBM: https://github.com/bmellink/IBusBM
 *   AccelStepper:http://www.airspayce.com/mikem/arduino/AccelStepper/index.html
 */

 /*

 Edits by RobertJN64 under the MIT license.

 */

// #define DEBUG_PRINT_CHANNELS //enable serial plotter of RC transmitter channel values
#define ENABLE_NORMAL_CONTROL //enables normal motor and steering control - disable for debug
//#define FORCE_MOTORS_ON //set motor speed to ch2_motor_speed regardless of other cmds

#include <Servo.h>
#include <ServoEasing.hpp> //RJN EDIT - changed to .hpp b/c compiler errs on Arduino IDE 2.0
#include <IBusBM.h>
#include <AccelStepper.h>

#define motorW1_IN1 6
#define motorW1_IN2 7
#define motorW2_IN1 4
#define motorW2_IN2 5
#define motorW3_IN1 2
#define motorW3_IN2 3
#define motorW4_IN1 13
#define motorW4_IN2 10
#define motorW5_IN1 8
#define motorW5_IN2 9
#define motorW6_IN1 11
#define motorW6_IN2 12

ServoEasing servoW1;
ServoEasing servoW3;
ServoEasing servoW4;
ServoEasing servoW6;
ServoEasing servoCamTilt;

AccelStepper camPanStepper(1, 46, 45);  //(Type:driver, STEP, DIR) - Stepper1

IBusBM IBus;
IBusBM IBusSensor;

int angle = 0;   // servo position in degrees
int ch0_rover_steer, ch1_cam_tilt, ch2_motor_speed, ch3_cam_pan, ch6_drive_direction = 0;
int servo1Angle = 90;
int servo3Angle = 90;
int servo4Angle = 90;
int servo6Angle = 90;
int s = 0; // rover speed
int r = 0; // turning radius
int m1, m2, m3, m4, m5, m6;
int camTilt = 90;
int camPan = 0;
float speed1, speed2, speed3 = 0;
float speed1PWM, speed2PWM, speed3PWM = 0;
float thetaInnerFront, thetaInnerBack, thetaOuterFront, thetaOuterBack = 0;


float d1 = 271; // distance in mm
float d2 = 278;
float d3 = 301;
float d4 = 304;


void setup() {

  /*
     Use this if you need to change the frequency of the PWM signals
    TCCR4B = TCCR4B & B11111000 | B00000101;     // D6,D7,D8 PWM frequency of 30.64 Hz
    TCCR2B = TCCR2B & B11111000 | B00000111;   // D9, D10 PWM frequency of 30.64 Hz
    TCCR1B = TCCR1B & B11111000 | B00000101;   // D11, D12  PWM frequency of 30.64 Hz
    TCCR5B = TCCR5B & B11111000 | B00000101; // D4, D13 PWM frequency of 30.64 Hz
    TCCR3B = TCCR3B & B11111000 | B00000101;    // D2, D3, D5 PWM frequency of 30.64 Hz
  */
  
  Serial.begin(115200);
  IBus.begin(Serial1, IBUSBM_NOTIMER); // Servo iBUS
  IBusSensor.begin(Serial2, IBUSBM_NOTIMER); // Sensor iBUS

  IBusSensor.addSensor(IBUSS_INTV); // add voltage sensor

  servoW1.attach(22);
  servoW3.attach(23);
  servoW4.attach(24);
  servoW6.attach(25);
  servoCamTilt.attach(26);

  servoW1.write(90);
  servoW3.write(90);
  servoW4.write(90);
  servoW6.write(90);
  servoCamTilt.write(90);

  servoW1.setSpeed(550);
  servoW3.setSpeed(550);
  servoW4.setSpeed(550);
  servoW6.setSpeed(550);
  servoCamTilt.setSpeed(200);

  camPanStepper.setMaxSpeed(1000);
  camPan = 0;
  camTilt = 90;


  // DC Motors
  // Motor Wheel 1 - Left Front
  digitalWrite(motorW1_IN1, LOW);   // PWM value
  digitalWrite(motorW1_IN2, LOW); // Forward
  // Motor Wheel 2 - Left Middle
  digitalWrite(motorW2_IN1, LOW);
  digitalWrite(motorW2_IN2, LOW);
  // Motor Wheel 3 - Left Back
  digitalWrite(motorW3_IN1, LOW);
  digitalWrite(motorW3_IN2, LOW);
  // right side motors move in opposite direction
  // Motor Wheel 4 - Right Front
  digitalWrite(motorW4_IN1, LOW);
  digitalWrite(motorW4_IN2, LOW);
  // Motor Wheel 5 - Right Middle
  digitalWrite(motorW5_IN1, LOW);
  digitalWrite(motorW5_IN2, LOW);
  // Motor Wheel 6 - Right Back
  digitalWrite(motorW6_IN1, LOW);
  digitalWrite(motorW6_IN2, LOW);


}

void loop() {
  // Reading the data comming from the RC Transmitter
  IBus.loop();
  // ALL CHANNELS ARE +1 from what it says on the flysky transmitter
  ch0_rover_steer = IBus.readChannel(0);  //right stick horizontal
  ch1_cam_tilt = IBus.readChannel(1); // right stick vertical
  ch2_motor_speed = IBus.readChannel(2); // left stick vertical
  ch3_cam_pan = IBus.readChannel(3); // left stick horizontal
  ch6_drive_direction = IBus.readChannel(6); // SWITCH_A (top left switch) - to configure this: follow this video https://www.youtube.com/watch?v=JZQoHmLqZYg
 
  // Convertign the incoming data
  // Steering right
  if (ch0_rover_steer > 1515) {
    r = map(ch0_rover_steer, 1515, 2000, 1400, 600); // turining radius from 1400mm to 600mm
  }
  // Steering left
  else if (ch0_rover_steer < 1485) {
    r = map(ch0_rover_steer, 1485, 1000, 1400, 600); // turining radius from 600mm to 1400mm
  }
  // Rover speed in % from 0 to 100
  s = map(ch2_motor_speed, 1000, 2000, 0, 100); // rover speed from 0% to 100%

  // Camera head steering
  if (ch1_cam_tilt < 1485 ) {
    if (camTilt >= 35) {
      camTilt--;
      delay(20);
    }
  }
  if (ch1_cam_tilt > 1515 ) {
    if (camTilt <= 165) {
      camTilt++;
      delay(20);
    }
  }
  servoCamTilt.startEaseTo(camTilt); // Camera tilt

  if (ch3_cam_pan >= 1000 && ch3_cam_pan < 1485) {
    camPan = map(ch3_cam_pan, 1000, 1485, 400, 0);
  }
  else if (ch3_cam_pan > 1515 && ch3_cam_pan <= 2000) {
    camPan = map(ch3_cam_pan, 1515, 2000, 0, -400);
  }
  else {
    camPan = 0;
  }
  camPanStepper.setSpeed(camPan);    // Camera pan
  camPanStepper.run();


  calculateMotorsSpeed();
  calculateServoAngle();

#ifdef DEBUG_PRINT_CHANNELS
  Serial.print("ch0_rover_steer:");
  Serial.print(ch0_rover_steer);
  Serial.print(",ch1_cam_tilt:");
  Serial.print(ch1_cam_tilt);
  Serial.print(",ch2_motor_speed:");
  Serial.print(ch2_motor_speed);
  Serial.print(",ch3_cam_pan:");
  Serial.print(ch3_cam_pan);
  Serial.print(",ch6_drive_direction:");
  Serial.println(ch6_drive_direction);
#endif

#ifdef FORCE_MOTORS_ON
  // Motor Wheel 1 - Left Front
  digitalWrite(motorW1_IN1, LOW);
  analogWrite(motorW1_IN2, speed1PWM);
  // Motor Wheel 2 - Left Middle
  digitalWrite(motorW2_IN1, LOW);
  analogWrite(motorW2_IN2, speed1PWM);
  // Motor Wheel 3 - Left Back
  digitalWrite(motorW3_IN1, LOW);
  analogWrite(motorW3_IN2, speed1PWM);
  // Motor Wheel 4 - Right Front
  digitalWrite(motorW4_IN1, LOW);
  analogWrite(motorW4_IN2, speed1PWM);
  // Motor Wheel 5 - Right Middle
  digitalWrite(motorW5_IN1, LOW);
  analogWrite(motorW5_IN2, speed1PWM);
  // Motor Wheel 6 - Right Back
  digitalWrite(motorW6_IN1, LOW);
  analogWrite(motorW6_IN2, speed1PWM);
#endif


#ifdef ENABLE_NORMAL_CONTROL
  // Steer right
  if (ch0_rover_steer > 1515) {
    // Servo motors
    // Outer wheels
    servoW1.startEaseTo(97 + thetaInnerFront); // front wheel steer right
    servoW3.startEaseTo(97 - thetaInnerBack); // back wheel steer left for overall steering to the right of the rover
    // Inner wheels
    servoW4.startEaseTo(94 + thetaOuterFront);
    servoW6.startEaseTo(96 - thetaOuterBack);

    // DC Motors
    if (ch6_drive_direction < 1500) { // Move forward
      // Motor Wheel 1 - Left Front
      analogWrite(motorW1_IN1, speed1PWM);   // Outer wheels running at speed1 - max speed
      digitalWrite(motorW1_IN2, LOW);
      // Motor Wheel 2 - Left Middle
      analogWrite(motorW2_IN1, speed1PWM);
      digitalWrite(motorW2_IN2, LOW);
      // Motor Wheel 3 - Left Back
      analogWrite(motorW3_IN1, speed1PWM);
      digitalWrite(motorW3_IN2, LOW);
      // right side motors move in opposite direction
      // Motor Wheel 4 - Right Front
      digitalWrite(motorW4_IN1, LOW);
      analogWrite(motorW4_IN2, speed2PWM); // Inner front wheel running at speed2 - lower speed
      // Motor Wheel 5 - Right Middle
      digitalWrite(motorW5_IN1, LOW);
      analogWrite(motorW5_IN2, speed3PWM); // Inner middle wheel running at speed3 - lowest speed
      // Motor Wheel 6 - Right Back
      digitalWrite(motorW6_IN1, LOW);
      analogWrite(motorW6_IN2, speed2PWM); // Inner back wheel running at speed2 - lower speed
    }
    else if (ch6_drive_direction > 1500) {
      // Motor Wheel 1 - Left Front
      digitalWrite(motorW1_IN1, LOW);   // Outer wheels running at speed1 - max speed
      analogWrite(motorW1_IN2, speed1PWM);
      // Motor Wheel 2 - Left Middle
      digitalWrite(motorW2_IN1, LOW);
      analogWrite(motorW2_IN2, speed1PWM);
      // Motor Wheel 3 - Left Back
      digitalWrite(motorW3_IN1, LOW);
      analogWrite(motorW3_IN2, speed1PWM);
      // right side motors move in opposite direction
      // Motor Wheel 4 - Right Front
      analogWrite(motorW4_IN1, speed2PWM);
      digitalWrite(motorW4_IN2, LOW); // Inner front wheel running at speed2 - lower speed
      // Motor Wheel 5 - Right Middle
      analogWrite(motorW5_IN1, speed3PWM);
      digitalWrite(motorW5_IN2, LOW); // Inner middle wheel running at speed3 - lowest speed
      // Motor Wheel 6 - Right Back
      analogWrite(motorW6_IN1, speed2PWM);
      digitalWrite(motorW6_IN2, LOW); // Inner back wheel running at speed2 - lower speed
    }
  }

  // Steer left
  else if (ch0_rover_steer < 1485) {
    // Servo motors
    servoW1.startEaseTo(97 - thetaOuterFront);
    servoW3.startEaseTo(97 + thetaOuterBack);
    servoW4.startEaseTo(94 - thetaInnerFront);
    servoW6.startEaseTo(96 + thetaInnerBack);

    // DC Motors
    if (ch6_drive_direction < 1500) { // Move forward
      // Motor Wheel 1 - Left Front
      analogWrite(motorW1_IN1, speed2PWM);   // PWM value
      digitalWrite(motorW1_IN2, LOW); // Forward
      // Motor Wheel 2 - Left Middle
      analogWrite(motorW2_IN1, speed3PWM);
      digitalWrite(motorW2_IN2, LOW);
      // Motor Wheel 3 - Left Back
      analogWrite(motorW3_IN1, speed2PWM);
      digitalWrite(motorW3_IN2, LOW);
      // Motor Wheel 4 - Right Front
      // right side motors move in opposite direction
      digitalWrite(motorW4_IN1, LOW);
      analogWrite(motorW4_IN2, speed1PWM);
      // Motor Wheel 5 - Right Middle
      digitalWrite(motorW5_IN1, LOW);
      analogWrite(motorW5_IN2, speed1PWM);
      // Motor Wheel 6 - Right Back
      digitalWrite(motorW6_IN1, LOW);
      analogWrite(motorW6_IN2, speed1PWM);
    }
    else if (ch6_drive_direction > 1500) { // Move backward
      // Motor Wheel 1 - Left Front
      digitalWrite(motorW1_IN1, LOW);   // PWM value
      analogWrite(motorW1_IN2, speed2PWM); // Forward
      // Motor Wheel 2 - Left Middle
      digitalWrite(motorW2_IN1, LOW);
      analogWrite(motorW2_IN2, speed3PWM);
      // Motor Wheel 3 - Left Back
      digitalWrite(motorW3_IN1, LOW);
      analogWrite(motorW3_IN2, speed2PWM);
      // Motor Wheel 4 - Right Front
      // right side motors move in opposite direction
      analogWrite(motorW4_IN1, speed1PWM);
      digitalWrite(motorW4_IN2, LOW);
      // Motor Wheel 5 - Right Middle
      analogWrite(motorW5_IN1, speed1PWM);
      digitalWrite(motorW5_IN2, LOW);
      // Motor Wheel 6 - Right Back
      analogWrite(motorW6_IN1, speed1PWM);
      digitalWrite(motorW6_IN2, LOW);
    }
  }

  // Move straight
  else {
    servoW1.startEaseTo(97);
    servoW3.startEaseTo(97);
    servoW4.startEaseTo(94);
    servoW6.startEaseTo(96);

    // DC Motors
    if (ch6_drive_direction < 1500) {
      // Motor Wheel 1 - Left Front
      analogWrite(motorW1_IN1, speed1PWM);  // all wheels move at the same speed
      digitalWrite(motorW1_IN2, LOW); // Forward
      // Motor Wheel 2 - Left Middle
      analogWrite(motorW2_IN1, speed1PWM);
      digitalWrite(motorW2_IN2, LOW);
      // Motor Wheel 3 - Left Back
      analogWrite(motorW3_IN1, speed1PWM);
      digitalWrite(motorW3_IN2, LOW);
      // right side motors move in opposite direction
      // Motor Wheel 4 - Right Front
      digitalWrite(motorW4_IN1, LOW);
      analogWrite(motorW4_IN2, speed1PWM);
      // Motor Wheel 5 - Right Middle
      digitalWrite(motorW5_IN1, LOW);
      analogWrite(motorW5_IN2, speed1PWM);
      // Motor Wheel 6 - Right Back
      digitalWrite(motorW6_IN1, LOW);
      analogWrite(motorW6_IN2, speed1PWM);
    }
    else if (ch6_drive_direction > 1500) {
      // Motor Wheel 1 - Left Front
      digitalWrite(motorW1_IN1, LOW);  // all wheels move at the same speed
      analogWrite(motorW1_IN2, speed1PWM); // Forward
      // Motor Wheel 2 - Left Middle
      digitalWrite(motorW2_IN1, LOW);
      analogWrite(motorW2_IN2, speed1PWM);
      // Motor Wheel 3 - Left Back
      digitalWrite(motorW3_IN1, LOW);
      analogWrite(motorW3_IN2, speed1PWM);
      // right side motors move in opposite direction
      // Motor Wheel 4 - Right Front
      analogWrite(motorW4_IN1, speed1PWM);
      digitalWrite(motorW4_IN2, LOW);
      // Motor Wheel 5 - Right Middle
      analogWrite(motorW5_IN1, speed1PWM);
      digitalWrite(motorW5_IN2, LOW);
      // Motor Wheel 6 - Right Back
      analogWrite(motorW6_IN1, speed1PWM);
      digitalWrite(motorW6_IN2, LOW);
    }
  }
#endif

  // Monitor the battery voltage
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.00 / 1023.00) * 3.02; // Convert the reading values from 5v to suitable 12V
  // Send battery voltage value to transmitter
  IBusSensor.loop();
  IBusSensor.setSensorMeasurement(1, voltage * 100);
}

void calculateMotorsSpeed() {
  // if no steering, all wheels speed is the same - straight move
  if (ch0_rover_steer > 1485 && ch0_rover_steer < 1515) {
    speed1 = speed2 = speed3 = s;
  }
  // when steering, wheels speed depend on the turning radius value
  else {
    // Outer wheels, furthest wheels from turning point, have max speed
    // Due to the rover geometry, all three outer wheels should rotate almost with the same speed. They differe only 1% so we asume they are the same.
    speed1 = s;
    // Inner front and back wheels are closer to the turing point and have lower speeds compared to the outer speeds
    speed2 = s * sqrt(pow(d3, 2) + pow((r - d1), 2)) / (r + d4);
    // Inner middle wheel is closest to the turning point, has the lowest speed
    speed3 = s * (r - d4) / (r + d4);
  }

  // speed value from 0 to 100% to PWM value from 0 to 255
  speed1PWM = map(round(speed1), 0, 100, 0, 255);
  speed2PWM = map(round(speed2), 0, 100, 0, 255);
  speed3PWM = map(round(speed3), 0, 100, 0, 255);
}

void calculateServoAngle() {
  // Calculate the angle for each servo for the input turning radius "r"
  thetaInnerFront = round((atan((d3 / (r + d1)))) * 180 / PI);
  thetaInnerBack = round((atan((d2 / (r + d1)))) * 180 / PI);
  thetaOuterFront = round((atan((d3 / (r - d1)))) * 180 / PI);
  thetaOuterBack = round((atan((d2 / (r - d1)))) * 180 / PI);

}
