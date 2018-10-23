/*
  Send an SMS with the SparkFun LTE Cat M1/NB-IoT Shield
  By: Jim Lindblom
  SparkFun Electronics
  Date: October 23, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to send an SMS message with the LTE Shield.

  Before beginning, you should have your shield connected on a MNO.
  See example 00 for help with that.

  Before uploading, set the DESTINATION_NUMBER constant to the 
  phone number you want to send a text message to.
  (Don't forget to add an internation code (e.g. 1 for US)).

  Once programmed, open the serial monitor, set the baud rate to 9600,
  and type a message to be sent via SMS.
  
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

// Set the cell phone number to be texted
String DESTINATION_NUMBER = "11234567890";

void setup() {
  Serial.begin(9600);

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  Serial.println(F("Type a message. Send a Newline (\\n) to send it..."));
}

void loop() {
  static String message = "";
  if (Serial.available())
  {
    char c = Serial.read();
    // Read a message until a \n (newline) is received
    if (c == '\n') {
      // Once we receive a newline. send the text.
      Serial.println("Sending: " + String(message));
      // Call lte.sendSMS(String number, String message) to send an SMS
      // message.
      lte.sendSMS(DESTINATION_NUMBER, message);
      message = ""; // Clear message string
    } else {
      message += c; // Add last character to message
    }
  }
}