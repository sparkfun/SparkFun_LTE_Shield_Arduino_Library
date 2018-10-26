/*
  Send a TCP message to Hologram
  By: Jim Lindblom
  SparkFun Electronics
  Date: October 23, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to send TCP messages to a Hologram server

  Before beginning, you should have your shield connected on a MNO.
  See example 00 for help with that.

  Before uploading, set your HOLOGRAM_DEVICE_KEY. This string can be found
  in your device's Hologram dashboard.

  Once programmed, open the serial monitor, set the baud rate to 9600,
  and type a message to be sent via TCP to the Hologram message service.
  Make sure your serial monitor's end-of-line setting is set to "newline".
  
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

// Plug in your Hologram device key here:
String HOLOGRAM_DEVICE_KEY = "Ab12CdE4";

// These values should remain the same:
const char HOLOGRAM_URL[] = "cloudsocket.hologram.io";
const unsigned int HOLOGRAM_PORT = 9999;

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
      sendHologramMessage(message);
      message = ""; // Clear message string
    } else {
      message += c; // Add last character to message
    }
  }
  lte.poll();
}

void sendHologramMessage(String message)
{
  int socket = -1;
  String hologramMessage;

  // New lines are not handled well
  message.replace('\r', ' ');
  message.replace('\n', ' ');

  // Construct a JSON-encoded Hologram message string:
  hologramMessage = "{\"k\":\"" + HOLOGRAM_DEVICE_KEY + "\",\"d\":\"" +
    message + "\"}";
  
  // Open a socket
  socket = lte.socketOpen(LTE_SHIELD_TCP);
  // On success, socketOpen will return a value between 0-5. On fail -1.
  if (socket >= 0) {
    // Use the socket to connec to the Hologram server
    Serial.println("Connecting to socket: " + String(socket));
    if (lte.socketConnect(socket, HOLOGRAM_URL, HOLOGRAM_PORT) == LTE_SHIELD_SUCCESS) {
      // Send our message to the server:
      Serial.println("Sending: " + String(hologramMessage));
      if (lte.socketWrite(socket, hologramMessage) == LTE_SHIELD_SUCCESS)
      {
        // On succesful write, close the socket.
        if (lte.socketClose(socket) == LTE_SHIELD_SUCCESS) {
          Serial.println("Socket " + String(socket) + " closed");
        }
      } else {
        Serial.println(F("Failed to write"));
      }
    }
  }
}