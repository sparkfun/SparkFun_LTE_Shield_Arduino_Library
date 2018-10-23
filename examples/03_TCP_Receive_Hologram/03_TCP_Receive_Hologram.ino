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

  Once programmed, open the serial monitor, set the baud rate to 9600.
  Use the Hologram dashboard to send a message (via Cloud data) to your device.
  You should see the message relayed to your serial monitor.
  Make sure the port your sending to matches HOLOGRAM_LISTEN_PORT.
  
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

// Hologram server constants, these shouldn't change:
const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;

int listeningSocket = -1;

// Callback to process data coming into the LTE module
void processSocketRead(int socket, String response) {
  // Print message received, and the IP address it was received from.
  // Also print socket received on.
  Serial.println("Read: " + response);
  Serial.println("Socket: " + String(socket));
  Serial.print("Remote IP: ");
  Serial.println(lte.lastRemoteIP());
}

// Callback to process when a socket closes
void processSocketClose(int socket) {
  // If the closed socket is the one we're listening on.
  // Set a flag to re-open the listening socket.
  if (socket == listeningSocket) {
    listeningSocket = -1;
  } else {
    // Otherwise print the closed socket
    Serial.println("Socket " + String(socket) + " closed");
  }
}

void setup() {
  Serial.begin(9600);

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }

  lte.setSocketReadCallback(&processSocketRead);
  lte.setSocketCloseCallback(&processSocketClose);
}

void loop() {
  lte.poll();

  // If a listening socket is not open. Set up a new one.
  if (listeningSocket < 0) {
    listenHologramMessage();
  }
}

void listenHologramMessage()
{
  int sock = -1;
  LTE_Shield_error_t err;

  // Open a new available socket
  listeningSocket = lte.socketOpen(LTE_SHIELD_TCP);
  // If a socket is available it should return a value between 0-5
  if (listeningSocket >= 0) {
    // Listen on the socket on the defined port
    err = lte.socketListen(listeningSocket, HOLOGRAM_LISTEN_PORT);
    if (err == LTE_SHIELD_ERROR_SUCCESS) {
      Serial.print(F("Listening socket open: "));
      Serial.println(listeningSocket);
    }
    else {
      Serial.println("Unable to listen on socket");
    }
  }
  else {
    Serial.println("Unable to open socket");
  }
}