/*********************************************************************
  This is an example for our nRF51822 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "TimeLib.h"

#define REDPIN 6
#define GREENPIN 10
#define BLUEPIN 9

#include "BluefruitConfig.h"
#include <AlertNodeLib.h>

AlertNode node;

const String name = "Smart Lamp";

const int pResistor = A0;

// Color settings present
int currentRed = 0;
int currentGreen = 255;
int currentBlue = 0;

// Stored previous color settings
int prevRed = 255;
int prevGreen = 255;
int prevBlue = 255;

int defaultRed = 0;
int defaultGreen = 255;
int defaultBlue = 0;

bool timerOn = false;
bool solarOn = false;
bool lightOn = false;

int onHour = 0;
int onMinute = 0;

int offHour = 0;
int offMinute = 0;

/*=========================================================================
    APPLICATION SETTINGS

      FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
     
                                Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                                running this at least once is a good idea.
     
                                When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                                Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
         
                                Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
/*
  SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

  Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  pinMode(pResistor, INPUT);
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  analogWrite(REDPIN, currentRed);
  analogWrite(BLUEPIN, currentBlue);
  analogWrite(GREENPIN, currentGreen);

  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Command <-> Data Mode Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in Command mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  //  while (! ble.isConnected()) {
  //    delay(500);
  //  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));

  node.setDebug(false);

}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  
  checkAlert();
  //checkTimer();
  //checkSolar();
  analogWrite(REDPIN, currentRed);
  analogWrite(BLUEPIN, currentBlue);
  analogWrite(GREENPIN, currentGreen);
  ble.println("test");
  ble.waitForOK();

    while (ble.available()) {
      ble.print("Select an option:\n");
      ble.print("1) Turn lamp off\n");
      ble.print("2) Turn lamp on or select mode\n");
      ble.print("3) Select lamp color\n");
      int c = ble.readline();
      ble.println(c);

      String input = ble.buffer;
      ble.println(input);

      if (c == '1' - '0') {
        deactivate();
//        done = true;
      }
      else if (c == '2' - '0') {
        selectMode();
//        done = true;
      } else if (c == '3' - '0') {
        chooseColor();
//        done = true;
      } else {
        ble.println("Unknown command");
//        done = false;
      }
    }
  }

//  while (ble.available()) {

//    bool done = false;
//    while (!done) {
//      checkAlert();
//
//      bool received = false;
//      while (!received) {
//        char command[BUFSIZE + 1];
//        getUserInput(command, BUFSIZE);
//        received = true;
//        if (strcmp(ble.buffer, "OK") == 0) {
//          received = false;
//          return;
//        } else if (strcmp(ble.buffer, "1") == 0) {
//          deactivate();
//          done = true;
//        } else if (strcmp(ble.buffer, "2") == 0) {
//          selectMode();
//          done = true;
//        } else if (strcmp(ble.buffer, "3") == 0) {
//          chooseColor();
//          done = true;
//        } else {
//          ble.println("Unknown command");
//          done = false;
//        }
//      }
//    }
//  }
//ble.waitForOK();
//}
}


void activate() {
  lightOn = true;
  if (prevRed == 255 && prevGreen == 255 && prevBlue == 255) {
    setColor(defaultRed, defaultGreen, defaultBlue);
  } else {
    setColor(prevRed, prevGreen, prevBlue);
  }
}

void deactivate() {
  setColor(255, 255, 255);
  lightOn = false;
}

void selectMode() {
  bool done = false;
  ble.print("Current mode: ");
  if (!solarOn && !timerOn) {
    ble.println("Manual operation");
  } else if (solarOn) {
    ble.println("Light sensor mode");
  } else {
    ble.println("Timer mode");
  }
  while (!done) {
    checkAlert();
    ble.println("Select light mode: ");
    ble.println("\t1) On");
    ble.println("\t2) Light sensor mode. Lamp will turn on when room is dark and turn off when it isn't.");
    ble.println("\t3) Timer mode. Lamp will activate and deactivate at set times.");
    int c = ble.read();
    done = true;
    switch (c) {
      case 1:
        activate();
        break;
      case 2:
        solarOn = true;
        timerOn = false;
        break;
      case 3:
        setTimer();
        timerOn = true;
        solarOn = false;
        break;
      default:
        ble.println("Unknown command");
        done = false;
        break;
    }
  }


}

void chooseColor() {
  bool done = false;
  while (!done) {
    checkAlert();
    ble.println("Select a color option:");
    ble.println("\t1) Red");
    ble.println("\t2) Green");
    ble.println("\t3) Blue");
    ble.println("\t4) Create color ");
    ble.println("\t5) Randomly generate color");
    ble.println("\t6) Use previous color");
    int c = ble.read();
    done = true;
    switch (c) {
      case 1:
        setColor(0, 255, 255);
        break;
      case 2:
        setColor(255, 0, 255);
        break;
      case 3:
        setColor(255, 255, 0);
        break;
      case 4:
        createColor();
        break;
      case 5:
        createRandomColor();
        break;
      case 6:
        setColor(prevRed, prevGreen, prevBlue);
        break;
      default:
        ble.println("Unknown command");
        done = false;
        break;
    }
  }
}

void setColor(int red, int green, int blue) {
  if (currentRed != 255 && currentGreen != 255 && currentBlue != 255) {
    prevRed = currentRed;
    prevGreen = currentGreen;
    prevBlue = currentBlue;
  }

  currentRed = red;
  currentGreen = green;
  currentBlue = blue;
}

void createColor() {
  bool done = false;
  int red;
  int green;
  int blue;
  while (!done) {
    checkAlert();
    done = true;
    ble.print("Select a red value (0 - 255):");
    red = ble.read();
    if (red < 0 || red > 255) {
      ble.println("Invalid entry");
      done = false;
    }
  }
  done = false;
  while (!done) {
    checkAlert();
    done = true;
    ble.print("Select a green value (0 - 255):");
    green = ble.read();
    if (green < 0 || green > 255) {
      ble.println("Invalid entry");
      done = false;
    }
  }
  done = false;
  while (!done) {
    checkAlert();
    done = true;
    ble.print("Select a blue value (0 - 255):");
    blue = ble.read();
    if (blue < 0 || blue > 255) {
      ble.println("Invalid entry");
      done = false;
    }
  }

  ble.print("Select a blue value (0 - 255):");
  blue = ble.read();
  setColor(255 - red, 255 - green, 255 - blue);
}

void createRandomColor() {
  randomSeed(analogRead(0));
  int red = random(256);
  randomSeed(analogRead(0));
  int green = random(256);
  randomSeed(analogRead(0));
  int blue = random(256);
  setColor(red, green, blue);
}

void setTimer() {
  bool done = false;
  ble.println("Setting activation time.");

  //Set on hour
  while (!done) {
    checkAlert();
    ble.println("Please set activation hour (0 - 23)");
    int hour = ble.read();
    if (hour < 0 || hour > 23) {
      ble.println("Illegal value");
    } else {
      onHour = hour;
      done = true;
    }
  }
  done = false;
  while (!done) {
    checkAlert();
    ble.println("Please set activation minute (0 - 59)");
    int minute = ble.read();
    if (minute < 0 || minute > 59) {
      ble.println("Illegal value");
    } else {
      onMinute = minute;
      done = true;
    }
  }
  done = false;
  while (!done) {
    checkAlert();
    ble.println("Please set deactivation hour (0 - 23)");
    int hour = ble.read();
    if (hour < 0 || hour > 23) {
      ble.println("Illegal value");
    } else {
      onHour = hour;
      done = true;
    }
  }
  done = false;
  while (!done) {
    checkAlert();
    ble.println("Please set deactivation minute (0 - 59)");
    int minute = ble.read();
    if (minute < 0 || minute > 59) {
      ble.println("Illegal value");
    } else {
      onMinute = minute;
      done = true;
    }
  }
  ble.println("Times set!");
}

void checkTimer() {
  if (timerOn) {
    if (hour() == onHour && minute() == onMinute) {
      lightOn = true;
      activate();
    } else if (hour() == offHour && minute() == offMinute) {
      lightOn = false;
      deactivate();
    }
  }
}

void checkSolar() {
  if (solarOn) {
    int value = analogRead(pResistor);
    if (value <= 400 && !lightOn) {
      activate();
    } else if (value > 400 && lightOn) {
      deactivate();
    }
  }
}

void checkAlert() {
  int alert = node.alertReceived();
  if (alert != AlertNode::NO_ALERT) {
    switch (alert) {
      case AlertNode::FIRE:
        displayAlert(0, 255, 255);
        break;
      case AlertNode::GAS:
        displayAlert(127, 0, 255);
        break;
      case AlertNode::FLOOD:
        displayAlert(255, 255, 0);
        break;
      case AlertNode::BURGLARY:
        for (int i = 0; i < 30; i++) {
          analogWrite(REDPIN, 0);
          analogWrite(BLUEPIN, 255);
          analogWrite(GREENPIN, 255);
          delay(1000);
          analogWrite(REDPIN, 255);
          analogWrite(BLUEPIN, 0);
          analogWrite(GREENPIN, 255);
          delay(1000);
        }
        break;
      case AlertNode::EARTHQUAKE:
        displayAlert(102, 179, 255);
        break;
      case AlertNode::ZOMBIE:
        displayAlert(95, 95, 95);
        break;
      case AlertNode::APOCALYPSE:
        displayAlert(128, 255, 0);
        break;
      case AlertNode::WAKE_MODE:
        solarOn = false;
        timerOn = false;
        activate();
        break;
      case AlertNode::SLEEPING_MODE:
        solarOn = false;
        timerOn = false;
        deactivate();
        break;
      default:
        displayAlert(0, 0, 0);
        break;
    }
  }
}

void displayAlert(int red, int green, int blue) {
  for (int i = 0; i < 30; i++) {
    analogWrite(REDPIN, red);
    analogWrite(BLUEPIN, blue);
    analogWrite(GREENPIN, green);
    delay(750);
    analogWrite(REDPIN, 255);
    analogWrite(BLUEPIN, 255);
    analogWrite(GREENPIN, 255);
    delay(250);
  }
}

void getUserInput(char buffer[], uint8_t maxSize)
{
  memset(buffer, 0, maxSize);
  while ( Serial.available() == 0 ) {
    delay(1);
  }

  uint8_t count = 0;

  do
  {
    count += Serial.readBytes(buffer + count, maxSize);
    delay(2);
  } while ( (count < maxSize) && !(Serial.available() == 0) );
}