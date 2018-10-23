#include <SparkFun_LTE_Shield_Arduino_Library.h>

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;

String hologramDeviceKey = "LC2tG5cf";

const char HOLOGRAM_URL[] = "cloudsocket.hologram.io";
const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;

void setup() {
    Serial.begin(9600);

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }

  Serial.println(F("Type a message. Send a Newline (\\n) to send it..."));
}

void loop() {
    lte.poll();
}

void sendHologramMessage(String message)
{
  int socket = -1;
  String hologramMessage;

    hologramMessage = "{\"k\":\"" + hologramDeviceKey + "\",\"d\":\"" +
        message + "\"}";

  socket = lte.socketOpen(LTE_SHIELD_TCP);
  if (socket >= 0) {
      Serial.println("Connecting to socket: " + String(socket));
    if (lte.socketConnect(socket, HOLOGRAM_URL, HOLOGRAM_PORT) == LTE_SHIELD_ERROR_SUCCESS) {
        Serial.println("Connected to Hologram.io");
      Serial.println("Sending: " + String(hologramMessage));
      if (lte.socketWrite(socket, hologramMessage) == LTE_SHIELD_ERROR_SUCCESS)
      {
          lte.socketClose(socket);
      }
    }
  }
}