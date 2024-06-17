/*
* Author: Philip Xu
* Date: 2024/06/06
* Description: Code that power the RoboVac. The RoboVac is a automated vacuuming robot with an obstacle avoidance feature. 
*              The RoboVac also offers a manual control feature available as a Python GUI app using Bluetooth.
*/

#include <NewPing.h>
#define MAX_SONAR_DISTANCE 400

const int DIRECTION_PIN_1 = 12;
const int PWM_PIN_1 = 3;
const int BRAKE_PIN_1 = 9;
const int CURRENT_SENSING_PIN_1 = A0;
const int DIRECTION_PIN_2 = 13;
const int PWM_PIN_2 = 11;
const int BRAKE_PIN_2 = 8;
const int CURRENT_SENSING_PIN_2 = A1;

const int TRIG_PIN_1 = 5;
const int TRIG_PIN_2 = 6;
const int TRIG_PIN_3 = 7;
const int ECHO_PIN_1= A3;
const int ECHO_PIN_2 = A4;
const int ECHO_PIN_3 = A5;

const int FAN_PWM_PIN = 10;
const int BATTERY_READ_PIN = A2;

// Create ultrasonic sensor NewPing objects
NewPing sonarLeft(TRIG_PIN_1, ECHO_PIN_1, MAX_SONAR_DISTANCE);
NewPing sonarCenter(TRIG_PIN_2, ECHO_PIN_2, MAX_SONAR_DISTANCE);
NewPing sonarRight(TRIG_PIN_3, ECHO_PIN_3, MAX_SONAR_DISTANCE);

unsigned long prevMillis = 0;
unsigned long lastStuckMillis = 0;
unsigned long lastBatteryMillis = 0;
unsigned long lastCmdMillis = 0;

String command;
String prevCommand = "";

void setup(){
  setPWMPrescaler(1024); // PWM prescaler of 1024, gives a PWM frequency ~30.52Hz (31250Hz/1024). Helps mitigate motor noise
  Serial.begin(9600);

  // Pin setups
  pinMode(DIRECTION_PIN_1, OUTPUT);
  pinMode(PWM_PIN_1, OUTPUT);
  pinMode(BRAKE_PIN_1, OUTPUT);
  pinMode(DIRECTION_PIN_2, OUTPUT);
  pinMode(PWM_PIN_2, OUTPUT);
  pinMode(BRAKE_PIN_2, OUTPUT);

  pinMode(CURRENT_SENSING_PIN_1, INPUT);
  pinMode(CURRENT_SENSING_PIN_2, INPUT);

  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(BATTERY_READ_PIN, INPUT);

  analogWrite(FAN_PWM_PIN, 255);
}

// Variables setup for loop()
int distanceLeft, distanceCenter, distanceRight, leftMotorCurrent, rightMotorCurrent;
int prevLeftMotorCurrent = 0;
int prevRightMotorCurrent = 0;
String messageIn = "";
char c;

String mode = "auto";
String motorCmd = "brake";
float motorMultiplier = 1;
float fanMultiplier = 1;
float voltage, percent;

void loop(){
  /** Get Serial input from the Bluetooth module **/
  if(Serial.available() > 0){
    c = (char)(Serial.read());
    if(c == '/'){ // Slash means message is finished
      if(messageIn.substring(0, 4) == "mode"){ // When the mode is switched between manual/auto
        mode = messageIn.substring(6, messageIn.length());
        Serial.println(mode);
        resetMotors();
        prevCommand = "";

        if(mode == "auto"){ // If mode is auto, reset all multipliers 
          motorMultiplier = 1;
          fanMultiplier = 1;
          analogWrite(FAN_PWM_PIN, 255*fanMultiplier);
        }
      }
      else{ // All other messages that end with slashes will be commands issued from the manual mode
        manualMode();
      }

      messageIn = "";
    }
    else{ // Add character to unfinished message
      messageIn += c;
    }
  }

  /** On mode **/
  if(mode == "auto"){
    autoMode();
  }
  else{
    // Every 500 ms, send battery stats through Bluetooth
    if(millis() - lastBatteryMillis > 500){
      voltage = analogRead(A2)/1023.0*5.0;
      percent = voltage/4.0*100;

      if(percent > 120){
        Serial.println("voltage: 0");
        Serial.println("battery: 0/");
      }
      else{
        Serial.println("voltage: " + (String)(voltage));
        Serial.println("battery: " + (String)(percent) + "/");
      } 
      lastBatteryMillis = millis();
    }
  }
}

// Code for manual mode inputs from the control app
void manualMode(){
  if(messageIn.substring(0, 22) == "motor-speed-multiplier"){
    motorMultiplier = messageIn.substring(24, messageIn.length()).toFloat();
    Serial.println(motorMultiplier);
    
    analogWrite(PWM_PIN_1, 30*motorMultiplier);
    analogWrite(PWM_PIN_2, 30*motorMultiplier);
  }
  else if(messageIn.substring(0, 20) == "fan-speed-multiplier"){
    fanMultiplier = messageIn.substring(22, messageIn.length()).toFloat();
    Serial.println("Current fan PWM output: " + (String)(255*fanMultiplier));
    analogWrite(FAN_PWM_PIN, 255*fanMultiplier);
  }
  else{
    motorCmd = messageIn;
    onCommand(motorCmd);
    prevCommand = motorCmd;
  }
}

// Code for automatic mode
void autoMode(){
  if(millis() - prevMillis >= 100){
    prevMillis = millis();
    
    distanceLeft = sonarLeft.ping()*0.034/2;
    distanceCenter = sonarCenter.ping()*0.034/2;
    distanceRight = sonarRight.ping()*0.034/2;

    leftMotorCurrent = analogRead(CURRENT_SENSING_PIN_1);
    rightMotorCurrent = analogRead(CURRENT_SENSING_PIN_2);

    if(leftMotorCurrent != 0 || rightMotorCurrent != 0){
      Serial.println(String(leftMotorCurrent) + " " + String(rightMotorCurrent));
    }

    /** Obstacle avoidance algorithm **/
    // CHANGE HERE?
    if((leftMotorCurrent+prevLeftMotorCurrent)/2 > 300 || (rightMotorCurrent+prevRightMotorCurrent)/2 > 300 && prevCommand == "forward"){ // If the current readings exceed 340, it means the motors are considerably stalled
      command = "backward";
      lastStuckMillis = millis();
    }
    else if(prevCommand == "backward"){
      if(millis() - lastStuckMillis > 2000){ // Back off for 2 seconds, then choose random direction to turn away to
        int randInt = random(0, 2);
        if(randInt == 0){
          command = "turn-left-random-duration";
        }
        else{
          command = "turn-right-random-duration";
        }
      }
    }
    else{ // Obstacle avoidance
      if((distanceCenter > 0 && distanceCenter < 10) || (distanceLeft > 0 && distanceLeft < 10) || (distanceRight > 0 && distanceRight < 10)){ // When an obstacle is too close
        if(distanceLeft == 0 && prevCommand != "turn-right-random-duration"){ // distance of 0 means no obstacle
          command = "turn-left-random-duration";
        }
        if(distanceRight == 0 && prevCommand != "turn-left-random-duration"){ // distance of 0 means no obstacle
          command = "turn-right-random-duration";
        }
        else if(distanceRight >= distanceLeft && prevCommand != "turn-left-random-duration"){
          command = "turn-right-random-duration";
        }
        else if(distanceLeft > distanceRight && prevCommand != " turn-right-random-duration"){
          command = "turn-left-random-duration";
        }
      }
      else if(distanceCenter >= 10 || distanceCenter == 0) {  // When no obstacle is in front
        if(prevCommand == "forward" || prevCommand == ""){
          lastCmdMillis = millis();
          command = "forward";
        }
        else if(prevCommand == "turn-left-random-duration" || prevCommand == "turn-right-random-duration"){
          if(distanceCenter >= 20 || distanceCenter == 0){ // Obstacle avoidance distance of 20 instead of 10cm so the robot will not be stuck on flat walls, making minute turns then moving forward repeatedly
            lastCmdMillis = millis();
            command = "forward";
          }
        }
      }
    }
  
    onCommand(command); // Use the command 
    prevCommand = command;
    prevLeftMotorCurrent = leftMotorCurrent;
    prevRightMotorCurrent = rightMotorCurrent;
  }
}

// Motor control based on the command
void onCommand(String cmd){
  if(millis() - lastCmdMillis > 1000 && cmd == "forward"){ // Give motor a boost after a second - sometimes stalls
    digitalWrite(DIRECTION_PIN_1, HIGH);
    digitalWrite(DIRECTION_PIN_2, HIGH);
    digitalWrite(BRAKE_PIN_1, LOW);
    digitalWrite(BRAKE_PIN_2, LOW);
    
    analogWrite(PWM_PIN_1, 255);
    analogWrite(PWM_PIN_2, 255);
    delay(10);
    analogWrite(PWM_PIN_1, 35*motorMultiplier);
    analogWrite(PWM_PIN_2, 35*motorMultiplier);

    lastCmdMillis = millis();
    return;
  }
  if(cmd != prevCommand){
    resetMotors();
    if(cmd == "forward"){
      digitalWrite(DIRECTION_PIN_1, HIGH);
      digitalWrite(DIRECTION_PIN_2, HIGH);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 35*motorMultiplier);
      analogWrite(PWM_PIN_2, 35*motorMultiplier);
      return;
    }
    if(cmd == "backward"){
      digitalWrite(DIRECTION_PIN_1, LOW);
      digitalWrite(DIRECTION_PIN_2, LOW);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 35*motorMultiplier);
      analogWrite(PWM_PIN_2, 35*motorMultiplier);
      return;
    }
    if(cmd == "turn-left-random-duration"){
      digitalWrite(DIRECTION_PIN_1, HIGH);
      digitalWrite(DIRECTION_PIN_2, LOW);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 70);
      analogWrite(PWM_PIN_2, 70);

      delay(random(3000, 5000));
      return;
    }
    if(cmd == "turn-left"){
      digitalWrite(DIRECTION_PIN_1, HIGH);
      digitalWrite(DIRECTION_PIN_2, LOW);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 70*motorMultiplier);
      analogWrite(PWM_PIN_2, 70*motorMultiplier);

      return;
    }
    if(cmd == "turn-right-random-duration"){
      digitalWrite(DIRECTION_PIN_1, LOW);
      digitalWrite(DIRECTION_PIN_2, HIGH);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 70);
      analogWrite(PWM_PIN_2, 70);

      delay(random(3000, 5000));
      return;
    }
    if(cmd == "turn-right"){
      digitalWrite(DIRECTION_PIN_1, LOW);
      digitalWrite(DIRECTION_PIN_2, HIGH);
      digitalWrite(BRAKE_PIN_1, LOW);
      digitalWrite(BRAKE_PIN_2, LOW);

      analogWrite(PWM_PIN_1, 255);
      analogWrite(PWM_PIN_2, 255);
      delay(10);
      analogWrite(PWM_PIN_1, 70*motorMultiplier);
      analogWrite(PWM_PIN_2, 70*motorMultiplier);

      return;
    }
    if(cmd == "brake"){
      return;
    }
    Serial.println("COMMAND INVALID: " + cmd);
  }
}

// Reset the motors
void resetMotors(){
  digitalWrite(BRAKE_PIN_1, HIGH);
  digitalWrite(BRAKE_PIN_2, HIGH);
  analogWrite(PWM_PIN_1, 0);
  analogWrite(PWM_PIN_2, 0);
  digitalWrite(DIRECTION_PIN_1, LOW);
  digitalWrite(DIRECTION_PIN_2, LOW);
  delay(500);
}

// Changes the PWM frequency of Timer2 (PWM timer for Pins 3 and 11)
void setPWMPrescaler(int prescaler){
  byte mode;

  switch(prescaler) {
    case 1: 
      mode = 0b001; 
      break;
    case 8: 
      mode = 0b010; 
      break;
    case 32: 
      mode = 0b011; 
      break; 
    case 64: 
      mode = 0b100; 
      break; 
    case 128: 
      mode = 0b101; 
      break;
    case 256: 
      mode = 0b110; 
      break;
    case 1024: 
      mode = 0b111; 
      break;
    default:
      return;
  }

  TCCR2B = TCCR2B & 0b11111000 | mode;
}
