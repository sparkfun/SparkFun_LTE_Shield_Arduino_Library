/*
  Send AT commands to the SARA-R4 via the Serial Port
  By: Jim Lindblom
  SparkFun Electronics
  Date: October 26, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example sketch provides a direct interface to the SARA-R4 module's
  AT command port. It will initialize the device, and then let you run free!
  Consult the SARA-R4 AT command set for a list of commands.

  After uploading the code, open your Serial Monitor, set the baud rate to 
  9600 baud. We also recommend setting the line-ending setting to
  "Carriage Return", so your AT commands will be read in by the SARA module.

  Hardware Connections:
  Attach the SparkFun LTE Cat M1/NB-IoT Shield to your Arduino
  Power the shield with your Arduino -- ensure the PWR_SEL switch is in
    the "ARDUINO" position.
*/

//Click here to get the library: http://librarymanager/All#SparkFun_LTE_Shield_Arduino_Library
#include <SparkFun_LTE_Shield_Arduino_Library.h>

// Create a SoftwareSerial object to pass to the LTE_Shield library
SoftwareSerial lteSerial(8, 9);
// Create a LTE_Shield object to use throughout the sketch
LTE_Shield lte;

void setup() {
  Serial.begin(9600);

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  Serial.println(F("Ready to passthrough!\r\n"));
}

void loop() {
  if (Serial.available()) {
    lteSerial.write((char) Serial.read());
  }
  if (lteSerial.available()) {
    Serial.write((char) lteSerial.read());
  }
}