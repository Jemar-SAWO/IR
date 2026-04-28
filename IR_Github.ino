#include <EEPROM.h>

#include <ICSC.h>


byte sendTheVersion = 1;
const int numReadings = 20;

int readings[numReadings];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
long total = 0;             // the running total
int average = 0;            // the average

int readings2[numReadings];  // the readings from the analog input
int readIndex2 = 0;          // the index of the current reading
long total2 = 0;             // the running total
int average2 = 0;            // the average

unsigned long sendingMillis;

uint16_t totalTime = 0;
uint16_t onTime = 0;
uint16_t offTime = 0;

volatile byte pwmState = LOW;

byte safetyPWM = 0;
byte uPWM = 0;
byte vPWM = 0;
byte wPWM = 0;
byte lastwPWM = 0;

byte systemIsOn_data = 0;

unsigned long wPWMmillis;
unsigned long safetymillis;
byte delayRelay = 0;

unsigned long ledmillis;
byte ledstate = 0;
unsigned long intervalblinking = 2000;
byte error = 0;
byte tempSet_data = 50;

byte ninetySeconds = 90;
unsigned long onesecondmillis;


byte errorNumber = 0;
byte col1, col2, col3, col4;

unsigned long blink4leds;
byte led4state = 0;
byte lasterror = 0;
unsigned long delayerrormillis;

byte fromUI_data = 0;
byte nineSecondsToErrorSeven = 9;
byte sendTheVersionOnce = 1;
unsigned long versionmillis;

byte light_data = 0;
byte fan_data = 0;
byte saveSMD = 0;
int smdSave;

int timeToChangeRelay = 0;
byte changeRelay = 0;
byte lastsafetyPWM = 0;
long delayRelaymillis = 1000;
byte powerSet_data = 50;

unsigned long forONandOFFTimeMillis = 0;
unsigned long timeIntervalForONandOFFTime = 50000;
unsigned long timeDutyCycle = 50000;
unsigned long dutyCycle = 0;
byte reset90Seconds = 0;
byte gikanSaOn = 0;

byte always_ON_until_set_temp_is_reached = 0;
unsigned long delayRead;
boolean turnOnRelay = true;
unsigned long delaySendMillis;
boolean timeToSend = false;
byte versionSend = 0;
byte increment;
unsigned long every100millis;
byte dataReceive = 0;
unsigned long everySecondSend;


void setup() {

  Serial.begin(57600);  // Change to 57600 baud

  // Use the same style as UI controller:
  ICSC.begin(7, 57600, 5);  // Station 'A', 57600 baud
  // put your setup code here, to run once:

  ICSC.registerCommand('T', &tempSet);
  ICSC.registerCommand('C', &fromUI);
  ICSC.registerCommand('L', &DATA);
  //  ICSC.registerCommand('U', &smdReset);
  //  ICSC.registerCommand('N', &systemIsOn);
  //  ICSC.registerCommand('F', &fan);
  //  ICSC.registerCommand('P', &powerSet);//not used for now
  ICSC.process();

  DDRD |= (1 << 4);  //u
  DDRD |= (1 << 3);  //v
  DDRD |= (1 << 2);  //w
  DDRD |= (1 << 6);  //safety
 
  pinMode(7, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(4, INPUT);
  pinMode(25, OUTPUT);
  pinMode(24, OUTPUT);

  totalTime = 99;  //13500Hz 74micro second PERIOD
  offTime = totalTime * 0.25;
  onTime = totalTime * 0.75;
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = offTime;
  TCCR1B |= (1 << CS11) | (1 << WGM12);  //prescaler 8 , CTC mode
  TIMSK |= (1 << OCIE1A);                //enable OCR1A interrupt

  smdSave = EEPROM.read(2);
  if (smdSave == 255) smdSave = 30;
}

ISR(TIMER1_COMPA_vect) {

  pwmState = !pwmState;
  if (pwmState == LOW) {  //on sa relay
    OCR1A = onTime;
  } else {  //off sa relay
    OCR1A = offTime;
  }

  if (safetyPWM == 1) {
    if (pwmState == 1) {
      PORTD &= ~(1 << 6);
    } else {
      PORTD |= (1 << 6);
    }
  } else {
    PORTD &= ~(1 << 6);
  }


  if (uPWM == 1) {
    if (pwmState == 1) {
      PORTD &= ~(1 << 4);
    } else {
      PORTD |= (1 << 4);
    }
  } else {
    PORTD &= ~(1 << 4);
  }


  if (vPWM == 1) {
    if (pwmState == 1) {
      PORTD &= ~(1 << 3);
    } else {
      PORTD |= (1 << 3);
    }
  } else {
    PORTD &= ~(1 << 3);
  }


  if (wPWM == 1) {
    if (pwmState == 1) {
      PORTD &= ~(1 << 2);
    } else {
      PORTD |= (1 << 2);
    }
  } else {
    PORTD &= ~(1 << 2);
  }
}



void systemIsOn(unsigned char src, char command, unsigned char len, char *data) {
  systemIsOn_data = *data;
  if (systemIsOn_data == 1) {
    changeRelay = !changeRelay;
  } else {
    ledmillis = millis();
    intervalblinking = 2000;
    ledstate = 0;
  }
}

void DATA(unsigned char src, char command, unsigned char len, char *data) {
  dataReceive = *data;
  if (dataReceive == 0) {
    light_data = 0;
  } else if (dataReceive == 1) {
    light_data = 1;
  } else if (dataReceive == 2) {
    fan_data = 0;
  } else if (dataReceive == 3) {
    fan_data = 1;
  } else if (dataReceive == 4) {  //off
    systemIsOn_data = 0;
  } else if (dataReceive == 5) {  //on
    systemIsOn_data = 1;
  } else if (dataReceive == 6) {  //error
    systemIsOn_data = 3;
  } else if (dataReceive == 7) {  //smd reset
    smdSave = 0;
    EEPROM.write(2, smdSave);
  }

  if (dataReceive == 4 || dataReceive == 5 || dataReceive == 6) {
    if (systemIsOn_data == 1) {
      changeRelay = !changeRelay;
    } else {
      ledmillis = millis();
      intervalblinking = 0;
    }
  }
}

void powerSet(unsigned char src, char command, unsigned char len, char *data) {
  powerSet_data = *data;
  switch (powerSet_data) {
    case 20:
      timeDutyCycle = 80000;  //off time
      break;
    case 25:
      timeDutyCycle = 75000;
      break;
    case 30:
      timeDutyCycle = 70000;
      break;
    case 35:
      timeDutyCycle = 65000;
      break;
    case 40:
      timeDutyCycle = 60000;
      break;
    case 45:
      timeDutyCycle = 55000;
      break;
    case 50:
      timeDutyCycle = 50000;
      break;
    case 55:
      timeDutyCycle = 45000;
      break;
    case 60:
      timeDutyCycle = 40000;
      break;
    case 65:
      timeDutyCycle = 35000;
      break;
    case 70:
      timeDutyCycle = 30000;
      break;
    case 75:
      timeDutyCycle = 25000;
      break;
    case 80:
      timeDutyCycle = 20000;
      break;
    case 85:
      timeDutyCycle = 15000;
      break;
    case 90:
      timeDutyCycle = 10000;
      break;
    case 95:
      timeDutyCycle = 5000;
      break;
    case 100:
      timeDutyCycle = 0;
      break;
  }
}


//void fan(unsigned char src, char command, unsigned char len, char *data)
//{
//  fan_data = *data;
//}


void tempSet(unsigned char src, char command, unsigned char len, char *data) {
  tempSet_data = *data;
}
void fromUI(unsigned char src, char command, unsigned char len, char *data) {
  nineSecondsToErrorSeven = 9;
  versionSend = *data;
}

void smdReset(unsigned char src, char command, unsigned char len, char *data) {
}




void loop() {
  // put your main code here, to run repeatedly:

  ICSC.process();
  if (versionSend == 2) {
    light_data = 0;
    fan_data = 0;
    ICSC.send(8, 'V', 2, (char *)&sendTheVersion);
    delay(10);
    ICSC.send(8, 'M', 2, (char *)&smdSave);
    if (error == 1) {
      delay(10);
      ICSC.send(8, 'e', 2, (char *)&errorNumber);
    }
    versionSend = 0;
  }



  total = total - readings[readIndex];
  readings[readIndex] = analogRead(2);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  average = total / numReadings;

  float ts1 = Thermister(average);



  total2 = total2 - readings2[readIndex2];
  readings2[readIndex2] = analogRead(4);
  total2 = total2 + readings2[readIndex2];
  readIndex2 = readIndex2 + 1;
  if (readIndex2 >= numReadings) {
    readIndex2 = 0;
  }
  average2 = total2 / numReadings;

  float smd = Thermister(average2);

  byte ts1data = (int)ts1;
  byte smddata = (int)smd;

  if (millis() - delaySendMillis > 1000) {
    if (millis() - every100millis > 100) {
      increment++;
      every100millis = millis();
      switch (increment) {
        case 1:
          ICSC.send(8, 'x', 2, (char *)&ts1data);
          break;
        case 2:
          ICSC.send(8, 'x', 2, (char *)&smddata);
          break;
        case 3:
          ICSC.send(8, 'U', 2, (char *)&systemIsOn_data);
          break;
      }
    }
    if (increment >= 3) {
      delaySendMillis = millis();
      increment = 0;
    }
  }


  if (systemIsOn_data == 1) {  //---------------------------------------------------on
    ICSC.process();
    ledstate = 1;
    if (delayRelay == 0) {
      if (changeRelay == 0)
        safetyPWM = 1;
      else
        wPWM = 1;
      wPWMmillis = millis();
      turnOnRelay = true;
      delayRelay = 1;
    }
    if (millis() - wPWMmillis > delayRelaymillis) {
      if (tempSet_data > ts1) {
        if (turnOnRelay == true) {
          if (changeRelay == 0)
            wPWM = 1;
          else
            safetyPWM = 1;

          ninetySeconds = 90;
        }
      } else {
        if (changeRelay == 0)
          wPWM = 0;
        else
          safetyPWM = 0;

        turnOnRelay = false;
      }
    }
  } else if (systemIsOn_data == 0) {  //-----------------------------------------------------off
    ICSC.process();
    always_ON_until_set_temp_is_reached = 1;
    timeToChangeRelay = 0;
    ninetySeconds = 0;
    if (delayRelay == 1) {

      if (changeRelay == 0) wPWM = 0;
      else safetyPWM = 0;


      safetymillis = millis();
      delayRelay = 0;
    }
    if (millis() - safetymillis > delayRelaymillis) {
      if (changeRelay == 0) safetyPWM = 0;
      else wPWM = 0;
    }

    if (error == 0) {
      if (millis() - ledmillis > intervalblinking) {
        ledstate = !ledstate;
        if (ledstate == 0) {
          intervalblinking = 2000;
        } else {
          intervalblinking = 200;
        }
        ledmillis = millis();
      }
    } else {
      if (millis() - ledmillis > 100) {
        ledstate = !ledstate;
        ledmillis = millis();
      }
    }

  } else {
    error = 0;
    systemIsOn_data = 0;
    errorNumber = 0;
    delayerrormillis = millis();
  }

  if (millis() - delayerrormillis > 500) {  //delay for half a second after turning On
    if (systemIsOn_data == 1) {
      if (digitalRead(4) == 0) {  //fuse defect
        error = 1;
        errorNumber = 3;
        systemIsOn_data = 0;
      }

      if (average <= 10) {  //short circuit
        error = 1;
        errorNumber = 2;
        systemIsOn_data = 0;
      }

      if (average > 990) {  //open circuit
        error = 1;
        errorNumber = 1;
        systemIsOn_data = 0;
      }

      if (ts1 >= 80) {  // maximum temp reached
        error = 1;
        errorNumber = 8;
        systemIsOn_data = 0;
      }

      if (smd > 95) {
        error = 1;
        errorNumber = 10;
        systemIsOn_data = 0;
        if (saveSMD == 0) {
          smdSave = (int)smd;
          EEPROM.write(2, smdSave);
          delay(10);
          ICSC.send(8, 'M', 2, (char *)&smdSave);
          saveSMD = 1;
        }
      } else {
        saveSMD = 0;
      }
    }
  }

  if (millis() - onesecondmillis > 999) {
    if (systemIsOn_data == 1) {
      if (turnOnRelay == false) {

        if (ninetySeconds == 0) {
          turnOnRelay = true;
        } else {
          ninetySeconds--;
        }
      }

      timeToChangeRelay++;
      if (timeToChangeRelay >= 3600) {
        timeToChangeRelay = 0;
        wPWM = 1;
        safetyPWM = 1;
        changeRelay != changeRelay;
      }
    }

    if (nineSecondsToErrorSeven > 0) {
      nineSecondsToErrorSeven--;
    }

    if (nineSecondsToErrorSeven == 0) {
      error = 1;
      errorNumber = 7;
      systemIsOn_data = 0;
      fromUI_data = 0;
    }

    onesecondmillis = millis();
  }
  ICSC.process();
  errorLed(errorNumber);
  digitalWrite(7, ledstate);
  digitalWrite(25, light_data);
  digitalWrite(24, fan_data);


  if (error != lasterror) {
    if (error == 1)
      ICSC.send(8, 'e', 2, (char *)&errorNumber);
    fan_data = 0;
  }
  lasterror = error;
}

double Thermister(double RawADC) {
  double Temp;
  Temp = 10000 * (RawADC * (5 / 1023.0)) / (5 - (RawADC * (5 / 1023.0)));
  Temp = 1 / (0.001138177396 + 0.0002326061337 * log(Temp) + 0.0000000961782205 * log(Temp) * log(Temp) * log(Temp));
  Temp = Temp - 273;
  return Temp;
}



void errorLed(byte j) {
  switch (j) {
    case 1:
      col1 = 1;
      col2 = 0;
      col3 = 0;
      col4 = 0;
      break;
    case 2:
      col1 = 0;
      col2 = 1;
      col3 = 0;
      col4 = 0;
      break;
    case 3:
      col1 = 1;
      col2 = 1;
      col3 = 0;
      col4 = 0;
      break;
    case 7:
      col1 = 1;
      col2 = 1;
      col3 = 1;
      col4 = 0;
      break;
    case 8:
      col1 = 0;
      col2 = 0;
      col3 = 0;
      col4 = 1;
      break;
    case 10:
      col1 = 0;
      col2 = 1;
      col3 = 0;
      col4 = 1;
      break;
    case 0:
      col1 = 0;
      col2 = 0;
      col3 = 0;
      col4 = 0;
      break;
  }



  if (millis() - blink4leds > 999) {
    led4state = !led4state;
    blink4leds = millis();
  }

  if (led4state == 1) {
    digitalWrite(21, col1);
    digitalWrite(20, col2);
    digitalWrite(19, col3);
    digitalWrite(18, col4);
  } else {
    digitalWrite(18, 0);
    digitalWrite(19, 0);
    digitalWrite(20, 0);
    digitalWrite(21, 0);
  }
}
