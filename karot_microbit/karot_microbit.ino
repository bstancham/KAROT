// Microbit is master on I2C

#include <Wire.h>
#include <Adafruit_Microbit.h>



/// PIN NUMBER CONSTANTS

// built-in pushbuttons
const int buttonA = 5;
const int buttonB = 11;
// secret buttons
const int buttonASecret = 0;
const int buttonBSecret = 1;


//// STATE VARIABLES

// arduino input
byte triggerState = 0;
byte limitSwitchFwdState = 0;
byte limitSwitchBwdState = 0;
byte ballInputState = 0;

// microbit buttons
byte buttonAState = 0;
byte buttonBState = 0;
byte buttonASecretState = 0;
byte buttonBSecretState = 0;
byte eitherAState = 0;
byte eitherBState = 0;

// other state variables
byte grantedNum = 0;
byte ballsRemaining = 0;
byte winnerState = 0;



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



//// OTHER VARIABLES

Adafruit_Microbit_Matrix microbit;



void setup() {
  pinMode(buttonA, INPUT);
  pinMode(buttonB, INPUT);
  pinMode(buttonASecret, INPUT);
  pinMode(buttonBSecret, INPUT);

  Wire.begin(); // join i2c bus (address optional for master)

  microbit.begin();

  // start serial for output
  Serial.begin(9600);
}

byte flashCounter = 0;
byte flashPeriod = 3;

void iterateFlashCounter() {
  flashCounter++;
  if (flashCounter > (2 * flashPeriod)) {
    flashCounter = 0;
  }
}

int modeSwitchDelay = 0;
int modeSwitchDelayAmt = 10;
byte newDeviceMode = DEVICE_MODE_NORMAL;

byte buttonDown = 0;

void iterateModeSwitchMode() {
  iterateFlashCounter();
  if (flashCounter < flashPeriod) {
    microbit.print(newDeviceMode);
  } else {
    microbit.print("");
  }

  if (!eitherAState && !eitherBState) {
    buttonDown = 0;
  }

  if (!buttonDown) {
    if (eitherAState) {
      buttonDown = 1;
      newDeviceMode--;
      if (newDeviceMode < DEVICE_MODE_NORMAL) {
        newDeviceMode = DEVICE_MODE_WINNER_SEQUENCE_TEST;
      }
    }
    if (eitherBState) {
      buttonDown = 1;
      newDeviceMode++;
      if (newDeviceMode > DEVICE_MODE_WINNER_SEQUENCE_TEST) {
        newDeviceMode = DEVICE_MODE_NORMAL;
      }
    }
  }
}

void iterateNormalMode() {
  if (winnerState) {
    microbit.print("WIN!");
  } else {
    microbit.print(grantedNum);
  }
}

void iterateInputRemainderMode() {
  // print only the last digit - otherwise devise has to wait while text scrolls
  microbit.print(ballsRemaining % 10);
  /* microbit.print(ballsRemaining); */
}

void iterateAutoDropMode() {
  if (winnerState) {
    microbit.print("WIN!");
  } else {
    microbit.print("A");
  }
}

void iterateManualMotorMode() {
  microbit.print("M");
}

void iterateManualSolenoidMode() {
  microbit.print("S");
}

void iterateInputTestMode() {
  if (buttonAState) {
    microbit.print("A");
  } else if (buttonBState) {
    microbit.print("B");
  } else if (buttonASecretState) {
    microbit.print("a");
  } else if (buttonBSecretState) {
    microbit.print("b");
  } else if (triggerState) {
    microbit.print("T");
  } else if (limitSwitchFwdState) {
    microbit.print("<");
  } else if (limitSwitchBwdState) {
    microbit.print(">");
  } else if (ballInputState) {
    microbit.print("i");
  } else {
    microbit.print("-");
  }
}

void iterateLedTestMode() {
  microbit.print("L");
}

void iterateDisplayTestMode() {
  microbit.print("D");
}

void iterateSoundTestMode() {
  microbit.print("s");
}

void iterateWinnerSequenceTestMode() {
  microbit.print("W");
}

void iterateResetMode() {
  microbit.print("R");
}

void iterateErrorMode(int code) {
  errorCode = code;
}

void loop() {

  // read input states
  buttonAState = !digitalRead(buttonA);
  buttonBState = !digitalRead(buttonB);
  buttonASecretState = digitalRead(buttonASecret);
  buttonBSecretState = digitalRead(buttonBSecret);
  eitherAState = buttonAState || buttonASecretState;
  eitherBState = buttonBState || buttonBSecretState;
  
  // transmit to arduino
  Wire.beginTransmission(4);
  // mode
  Wire.write(deviceMode); // send one byte
  // buttons state
  Wire.write(eitherAState); // send one byte
  Wire.write(eitherBState); // send one byte
  Wire.endTransmission();

  // read from arduino
  Wire.requestFrom(4, 7); // request from slave device #4, 7 bytes
  // state of inputs (1 byte each = 4 bytes)
  if (Wire.available()) {
    triggerState = Wire.read();
  }
  if (Wire.available()) {
    ballInputState = Wire.read();
  }
  if (Wire.available()) {
    limitSwitchFwdState = Wire.read();
  }
  if (Wire.available()) {
    limitSwitchBwdState = Wire.read();
  }
  // other variables (1 byte each)
  if (Wire.available()) {
    grantedNum = Wire.read();
  }
  if (Wire.available()) {
    ballsRemaining = Wire.read();
  }
  if (Wire.available()) {
    winnerState = Wire.read();
  }
  // just in case extra have been sent...
  while (Wire.available()) {}

  // serial output for debugging
  Serial.print("mode = ");
  Serial.print(deviceMode);
  Serial.print(", mbA = ");
  Serial.print(eitherAState);
  Serial.print(", mbB = ");
  Serial.print(eitherBState);
  Serial.print(", trig = ");
  Serial.print(triggerState);
  Serial.print(", ball-input = ");
  Serial.print(ballInputState);
  Serial.print(", f-limit = ");
  Serial.print(limitSwitchFwdState);
  Serial.print(", b-limit = ");
  Serial.println(limitSwitchBwdState);

  // check for mode switch
  if (modeSwitchDelay > 0) {
    modeSwitchDelay--;
  } else if (eitherAState && eitherBState) {
    modeSwitchDelay = modeSwitchDelayAmt;
    if (deviceMode == DEVICE_MODE_SWITCH_MODE) {
      deviceMode = newDeviceMode;
      // notify
      if (deviceMode == DEVICE_MODE_INPUT_REMAINDER) {
        microbit.print("rem");
        microbit.print(ballsRemaining);
      }
    } else {
      newDeviceMode = deviceMode;
      deviceMode = DEVICE_MODE_SWITCH_MODE;
    }
  }

  // iterate loop for current mode
  switch (deviceMode) {
    case DEVICE_MODE_SWITCH_MODE:
      iterateModeSwitchMode();
      break;
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

  delay(30);
}
