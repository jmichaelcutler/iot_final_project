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
const int lightLevel = 300;

const int PRESISTOR = A0;

// Color settings present
int currentRed = 255;
int currentGreen = 255;
int currentBlue = 255;

// Stored previous color settings
int prevRed = 255;
int prevGreen = 255;
int prevBlue = 255;

int defaultRed = 0;
int defaultGreen = 0;
int defaultBlue = 0;

bool solarOn = true;
bool lightOn = false;

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
  pinMode(PRESISTOR, INPUT);
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  analogWrite(REDPIN, defaultRed);
  analogWrite(BLUEPIN, defaultBlue);
  analogWrite(GREENPIN, defaultGreen);

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
  node.begin(name); 

}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  checkAlert();
  if(solarOn) {
    checkSolar();
  }
  analogWrite(REDPIN, currentRed);
  analogWrite(BLUEPIN, currentBlue);
  analogWrite(GREENPIN, currentGreen);
  
  while (ble.available()) {
    checkAlert();
    int c = ble.read();
    if (c == 49) {
      deactivate();
      ble.println("Lamp deactivated!");
    } else if (c == 50) {
      activate();
      ble.println("Lamp activated!");
    } else if (c == 51) {
      setColor(0, 255, 255);
      ble.println("Color set!");
    } else if(c == 52) {
      setColor(0, 0, 255);
      ble.println("Color set!");
    } else if(c == 53) {
      setColor(0, 0, 0);
      ble.println("Color set!");
    } else if(c == 54) {
      createRandomColor();
      ble.println("Color set!");
    } else if(c == 55) {
      solarOn = true;
      ble.println("Solar mode activated!");
    } else if(c == 56) {
      solarOn = false;
      ble.println("Solar mode deactivated!");
    } else if (c == 115) {
      ble.print("Select an option:\n");
      ble.print("1) Turn lamp off\n");
      ble.print("2) Turn lamp on\n");
      ble.print("3) Set lamp color to red\n");
      ble.print("4) Set lamp color to yellow\n");
      ble.print("5) Set lamp color to white\n");
      ble.print("6) Set lamp color to a randomly chosen color");
      ble.print("7) Activate solar mode\n");
    } else {
      ble.println("Unknown command");
    }
  }
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

void createRandomColor() {
  randomSeed(analogRead(0));
  int red = random(256);
  randomSeed(analogRead(0));
  int green = random(256);
  randomSeed(analogRead(0));
  int blue = random(256);
  setColor(red, green, blue);
}

void checkSolar() {
    int value = analogRead(PRESISTOR);
    Serial.println(value);
    if (value <= lightLevel && !lightOn) {
      activate();
    } else if (value > 400 && lightOn) {
      deactivate();
    }
}

void checkAlert() {
  int alert = node.alertReceived();
  if (alert != AlertNode::NO_ALERT) {
    Serial.println("ALERT RECEIVED");
    switch (alert) {
      case AlertNode::FIRE:
        displayAlert(0, 255, 255);
        break;
      case AlertNode::GAS:
        displayAlert(101, 50, 205);
        break;
      case AlertNode::FLOOD:
        displayAlert(255, 255, 0);
        break;
      case AlertNode::BURGLARY:
        for (int i = 0; i < 60; i++) {
          analogWrite(REDPIN, 0);
          analogWrite(BLUEPIN, 255);
          analogWrite(GREENPIN, 255);
          delay(500);
          analogWrite(REDPIN, 255);
          analogWrite(BLUEPIN, 0);
          analogWrite(GREENPIN, 255);
          delay(500);
        }
        break;
      case AlertNode::EARTHQUAKE:
        displayAlert(90, 213, 213);
        break;
      case AlertNode::ZOMBIE:
        displayAlert(86, 86, 86);
        break;
      case AlertNode::APOCALYPSE:
        displayAlert(127, 0, 255);
        break;
      case AlertNode::WAKE_MODE:
        solarOn = true;
        break;
      case AlertNode::OVERRIDE:
        solarOn = false;
        activate();
      case AlertNode::SLEEPING_MODE:
        solarOn = false;
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
