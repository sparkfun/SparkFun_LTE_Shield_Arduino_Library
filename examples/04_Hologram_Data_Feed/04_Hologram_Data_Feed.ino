/*
  Receive a message from Hologram, then send data on command
  By: Jim Lindblom
  SparkFun Electronics
  Date: October 23, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example combines a bit of example 2 (TCP Send to Hologram) and
  example 3 (TCP receive from Hologram). On a specific message receipt from
  Hologram (read_analog), this sketch will send a JSON-encoded list of
  all six analog measurements.

  After uploading the code, open your Hologram dashboard, select  your
  device and send a message matching "read_analog" to the device. 
  
  Shortly after this the sketch should respond with a JSON-encoded list 
  of all six analog measurements. This JSON string can be used for Hologram 
  routes to send emails, text  messages, or more. (See the LTE Shield hookup 
  guide for more information.)

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

// Hologram device key. Used to send messages:
String HOLOGRAM_DEVICE_KEY = "Ab12CdE4";
// Hologram message topic(s):
String HOLOGRAM_ANALOG_TOPIC = "ANALOG";

// Hologram Server constants. Shouldn't have to change:
const char HOLOGRAM_URL[] = "cloudsocket.hologram.io";
const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;

// Pair analog pins up with a JSON identifier string
#define ANALOG_PINS_TO_READ 6
int readPins[ANALOG_PINS_TO_READ] = {A0, A1, A2, A3, A4, A5};
String pinNames[ANALOG_PINS_TO_READ] = {"A0", "A1", "A2", "A3", "A4", "A5"};

// loop flags to check:
boolean doSendAnalog = false; // Send analog values
boolean doServerListen = true; // Open listening server
int listenSocket = -1; // Listen socket -1 is incactive, 0-5 for active socket

// Callback to process a data read from a socket
void processSocketRead(int socket, String response) {
  // Look for the specified string
  if (response == "read_analog") {
    doSendAnalog = true; // If found set a flag to send analog values
  } else {
    Serial.println("Server read: " + String(response));
  }
}

// Callback to process a closed socket
void processSocketClose(int socket) {
  Serial.println("Socket " + String(socket) + " closed");
  // If the socket was our listening socket, trigger flag to re-open
  if (socket == listenSocket) {
    doServerListen = true;
  }
}

void setup() {
  Serial.begin(9600);

  for (int i=0; i<ANALOG_PINS_TO_READ; i++) {
    pinMode(readPins[i], INPUT);
  }

  if (!lte.begin(lteSerial)) {
    Serial.println("Could not initialize LTE Shield");
    while (1) ;
  }

  // Configure callbacks for data read and socket close
  lte.setSocketReadCallback(&processSocketRead);
  lte.setSocketCloseCallback(&processSocketClose);
}

void loop() {
  // Poll as often as possible. Try to keep other functions
  // in loop as short as possible.
  lte.poll();

  // If sendAnalog flag is set, do it and clear flag
  if (doSendAnalog) {
    Serial.println("Sending analog values");
    sendAnalogValues();
    doSendAnalog = false;
  }

  // If serverlisten flag is set, set up listening server
  // then clear flag
  if (doServerListen) {
    listenHologramMessage();
    doServerListen = false;
  }
}

void listenHologramMessage()
{
  int sock = -1;
  LTE_Shield_error_t err;

  // Open a new available socket
  sock = lte.socketOpen(LTE_SHIELD_TCP);
  // If a socket is available it should return a value between 0-5
  if (sock >= 0) {
    // Listen on the socket on the defined port
    err = lte.socketListen(sock, HOLOGRAM_LISTEN_PORT);
    if (err == LTE_SHIELD_ERROR_SUCCESS) {
      Serial.print(F("Listening socket open: "));
      Serial.println(sock);
    }
    else {
      Serial.println(F("Unable to listen on socket"));
    }
  }
  else {
    Serial.println(F("Unable to open socket"));
  }
}

// Compile analog reads into a JSON-encoded string, then send to Hologram
void sendAnalogValues(void) {
  String toSend = "{";
  for (int i = 0; i < ANALOG_PINS_TO_READ; i++) {
    toSend += "\\\"" + pinNames[i] + "\\\": ";
    toSend += String(analogRead(readPins[i]));
    if (i < 5) toSend += ", ";
  }
  toSend += "}";
  sendHologramMessage(toSend);
}

void sendHologramMessage(String message) {
  int socket = -1;
  String hologramMessage;

    hologramMessage = "{\"k\":\"" + HOLOGRAM_DEVICE_KEY + "\",\"d\":\"" +
        message + "\",\"t\":[\"" + HOLOGRAM_ANALOG_TOPIC + "\"]}";

  socket = lte.socketOpen(LTE_SHIELD_TCP);
  if (socket >= 0) {
      Serial.println("Connecting to socket: " + String(socket));
    if (lte.socketConnect(socket, HOLOGRAM_URL, HOLOGRAM_PORT) == LTE_SHIELD_ERROR_SUCCESS) {
        Serial.println(F("Connected to Hologram.io"));
        Serial.println("Sending: " + hologramMessage);
      if (lte.socketWrite(socket, hologramMessage) == LTE_SHIELD_ERROR_SUCCESS)
      {
          lte.socketClose(socket);
      }
    }
  }
}
