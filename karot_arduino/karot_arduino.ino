// Arduino is slave on I2C at address #4

#include <Wire.h>


//// PIN NUMBER CONSTANTS

// arduino input pins
const int triggerPin = 2;
const int ballSensorPin = 3;
const int limitSwitchFwdPin = 4;
const int limitSwitchBwdPin = 5;

// h-bridge controller pins
// NOTE: I got these wrong way around when I wired the motor, so swapped them here
const int motorFwdPin = 7;
const int motorBwdPin = 6;

// LED pins
const int ledRedPin = 10;
const int ledGreenPin = 11;
const int ledBluePin = 12;



//// STATE VARIABLES

// arduino input
byte triggerState = 0;
byte ballSensorState = 0;
byte limitSwitchFwdState = 0;
byte limitSwitchBwdState = 0;

// microbit buttons
byte buttonAState = 0;
byte buttonBState = 0;

// other state variables
byte grantedNum = 0;
byte ballsRemaining = 0;
byte ballReleased = 0;
byte winnerState = 0;

int buttonADown = 0;
int buttonBDown = 0;
int triggerDown = 0;
int redPWM = 0;
int greenPWM = 0;
int bluePWM = 0;



//// DEVICE MODES
const byte DEVICE_MODE_SWITCH_MODE = 0;
const byte DEVICE_MODE_NORMAL = 1;
const byte DEVICE_MODE_INPUT_REMAINDER = 2;
const byte DEVICE_MODE_AUTO_DROP = 3;
const byte DEVICE_MODE_MANUAL_MOTOR = 4;
const byte DEVICE_MODE_MANUAL_SOLENOID = 5;
const byte DEVICE_MODE_INPUT_TEST = 6;
const byte DEVICE_MODE_LED_TEST = 7;
const byte DEVICE_MODE_DISPLAY_TEST = 8;
const byte DEVICE_MODE_SOUND_TEST = 9;
const byte DEVICE_MODE_WINNER_SEQUENCE_TEST = 10;
const byte DEVICE_MODE_RESET = 11;
byte deviceMode = DEVICE_MODE_NORMAL;



//// ERROR CODES
const int ERROR_CODE_NO_ERROR = 0;
const int ERROR_CODE_MAIN_LOOP_SWITCH = 1;
int errorCode = ERROR_CODE_NO_ERROR;



void setup() {

  // initialize limit switch pins as input
  pinMode(triggerPin, INPUT);
  pinMode(ballSensorPin, INPUT);
  pinMode(limitSwitchFwdPin, INPUT);
  pinMode(limitSwitchBwdPin, INPUT);

  // motor controller pins as output
  pinMode(motorFwdPin, OUTPUT);
  pinMode(motorBwdPin, OUTPUT);

  // LED pins as output
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);

  // join i2c bus with address #4
  Wire.begin(4);
  // register events
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // start serial for debugging output
  Serial.begin(9600);

  Serial.print("sizeof(int) = ");
  Serial.println(sizeof(int)); // gives 2
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (Wire.available()) {
    // receive three bytes
    deviceMode = Wire.read();
    buttonAState = Wire.read();
    buttonBState = Wire.read();
  }
}

// function that executes whenever data is requested by master
void requestEvent() {
  // respond with message of 7 bytes as expected by master
  Wire.write(triggerState);
  Wire.write(ballSensorState);
  Wire.write(limitSwitchFwdState);
  Wire.write(limitSwitchBwdState);
  Wire.write(grantedNum);
  Wire.write(ballsRemaining);
  Wire.write(winnerState);
}

int ledCycleCounter = 0;

void iterateWinnerState() {
  // cycle LED colours
  switch (ledCycleCounter) {
    case 0:
      // off
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
      break;
    case 1:
      // red
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
      break;
    case 2:
      // yellow
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
      break;
    case 3:
      // green
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
      break;
    case 4:
      // cyan
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, HIGH);
      break;
    case 5:
      // blue
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, HIGH);
      break;
    case 6:
      // magenta
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, HIGH);
      break;
  }
  ledCycleCounter++;
  if (ledCycleCounter > 5) {
    ledCycleCounter = 0;
  }
}

int releaseSequenceStage = 0;
int releaseSequencePauseCounter = 0;
int releaseSequencePauseAmt = 3;

void iterateReleaseSequence() {
  if (releaseSequencePauseCounter > 0) {
    releaseSequencePauseCounter--;
  } else {
    if (releaseSequenceStage == 1) {
      // forward
      if (!limitSwitchFwdState) {
        digitalWrite(motorFwdPin, HIGH);
      } else {
        releaseSequenceStage++;
        digitalWrite(motorFwdPin, LOW);
        releaseSequencePauseCounter = releaseSequencePauseAmt;
      }
    } else if (releaseSequenceStage == 2) {
      // backward
      if (!limitSwitchBwdState) {
        digitalWrite(motorBwdPin, HIGH);
      } else {
        releaseSequenceStage++;
        digitalWrite(motorBwdPin, LOW);
        releaseSequencePauseCounter = releaseSequencePauseAmt;
      }
    } else if (releaseSequenceStage == 3) {
      // reset to where backward limit switch is no longer pressed
      if (limitSwitchBwdState) {
        digitalWrite(motorFwdPin, HIGH);
      } else {
        digitalWrite(motorFwdPin, LOW);
        releaseSequencePauseCounter = releaseSequencePauseAmt;
        releaseSequenceStage = 0; // reset to ready
        if (grantedNum > 0) {
          grantedNum--;
        }
        if (ballsRemaining > 0) {
          ballsRemaining--;
          if (ballsRemaining == 0) {
            winnerState = 1;
          }
        }
      }
    }
  }
}

void iterateNormalMode() {

  if (buttonAState) {
    if (!buttonADown) {
      buttonADown = 1;
      if (grantedNum > 0) {
        grantedNum--;
      }
    }
  } else {
    buttonADown = 0;
  }

  if (buttonBState) {
    if (!buttonBDown) {
      buttonBDown = 1;
      grantedNum++;
    }
  } else {
    buttonBDown = 0;
  }

  if (winnerState) {
    iterateWinnerState();
  } else {
    if (grantedNum < 1) {
      // red light
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
    } else {
      // green light
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
    }

    if (triggerState && (grantedNum > 0)) {
      if (releaseSequenceStage == 0) {
        releaseSequenceStage++;
      }
    }

    iterateReleaseSequence();
  }
}

void iterateInputRemainderMode() {

  if (buttonAState) {
    if (!buttonADown) {
      buttonADown = 1;
      if (ballsRemaining > 0) {
        ballsRemaining--;
      }
    }
  } else {
    buttonADown = 0;
  }

  if (buttonBState) {
    if (!buttonBDown) {
      buttonBDown = 1;
      ballsRemaining++;
      winnerState = 0;
    }
  } else {
    buttonBDown = 0;
  }
}

int autoDropping = 0;

void iterateAutoDropMode() {

  if (ballsRemaining == 0) {
    autoDropping = 0;
  }

  if (winnerState){
    iterateWinnerState();
  } else {

    // trigger starts or stops auto-dropping
    if (triggerState && (ballsRemaining > 0)) {
      if (!triggerDown) {
        triggerDown = 1;
        if (!autoDropping) {
          autoDropping = 1;
          greenPWM = 255;
        } else {
          autoDropping = 0;
        }
      } else {
        triggerDown = 0;
      }
    }

    if (autoDropping) {
      // pulsed green
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledBluePin, LOW);
      if (greenPWM < 256) {
        analogWrite(ledGreenPin, greenPWM);
      } else {
        analogWrite(ledGreenPin, 255 - (greenPWM - 255));
      }
      greenPWM += 32;
      if (greenPWM > 510) {
        greenPWM = 0;
      }
    } else {
      // solid green
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
    }

    if (autoDropping && (releaseSequenceStage == 0)) {
      releaseSequenceStage++;
    }

    iterateReleaseSequence();
  }
}

void iterateManualMotorMode() {

  // green light
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledGreenPin, HIGH);
  digitalWrite(ledBluePin, LOW);

  if (buttonAState && !buttonBState && !limitSwitchFwdState) {
    digitalWrite(motorFwdPin, HIGH);
  } else {
    digitalWrite(motorFwdPin, LOW);
  }
  if (buttonBState && !buttonAState && !limitSwitchBwdState) {
    digitalWrite(motorBwdPin, HIGH);
  } else {
    digitalWrite(motorBwdPin, LOW);
  }
}

void iterateManualSolenoidMode() {

}

void iterateInputTestMode() {

}

int ledTestNum = 0;

void iterateLedTestMode() {
  if (buttonAState) {
    if (!buttonADown) {
      buttonADown = 1;
      if (ledTestNum == 0) {
        ledTestNum = 7;
      } else {
        ledTestNum--;
      }
    }
  } else {
    buttonADown = 0;
  }

  if (buttonBState) {
    if (!buttonBDown) {
      buttonBDown = 1;
      ledTestNum++;
      if (ledTestNum > 7) {
        ledTestNum = 0;
      }
    }
  } else {
    buttonBDown = 0;
  }

  switch (ledTestNum) {
    case 0:
      // off
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
      break;
    case 1:
      // red
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
      break;
    case 2:
      // green
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
      break;
    case 3:
      // blue
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, HIGH);
      break;
    case 4:
      // red & green
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, LOW);
      break;
    case 5:
      // red & blue
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, HIGH);
      break;
    case 6:
      // green & blue
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, HIGH);
      break;
    case 7:
      // red & green & blue
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledBluePin, HIGH);
      break;
    default:
      // off
      digitalWrite(ledRedPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledBluePin, LOW);
  }
}

void iterateDisplayTestMode() {

}

void iterateSoundTestMode() {

}

void iterateWinnerSequenceTestMode() {

}

void iterateResetMode() {

}

void iterateErrorMode(int code) {
  errorCode = code;
}

void loop() {

  // read input states
  triggerState = digitalRead(triggerPin);
  ballSensorState = digitalRead(ballSensorPin);
  limitSwitchFwdState = digitalRead(limitSwitchFwdPin);
  limitSwitchBwdState = digitalRead(limitSwitchBwdPin);

  // iterate loop for current mode
  switch (deviceMode) {
    case DEVICE_MODE_NORMAL:
      iterateNormalMode();
      break;
    case DEVICE_MODE_INPUT_REMAINDER:
      iterateInputRemainderMode();
      break;
    case DEVICE_MODE_AUTO_DROP:
      iterateAutoDropMode();
      break;
    case DEVICE_MODE_MANUAL_MOTOR:
      iterateManualMotorMode();
      break;
    case DEVICE_MODE_MANUAL_SOLENOID:
      iterateManualSolenoidMode();
      break;
    case DEVICE_MODE_INPUT_TEST:
      iterateInputTestMode();
      break;
    case DEVICE_MODE_LED_TEST:
      iterateLedTestMode();
      break;
    case DEVICE_MODE_DISPLAY_TEST:
      iterateDisplayTestMode();
      break;
    case DEVICE_MODE_SOUND_TEST:
      iterateSoundTestMode();
      break;
    case DEVICE_MODE_WINNER_SEQUENCE_TEST:
      iterateWinnerSequenceTestMode();
      break;
    case DEVICE_MODE_RESET:
      iterateResetMode();
      break;
    default:
      iterateErrorMode(ERROR_CODE_MAIN_LOOP_SWITCH);
  }

  delay(50);
}
