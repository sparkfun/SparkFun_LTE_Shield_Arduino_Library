

#include <SparkFun_LTE_Shield_Arduino_Library.h>

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;

#define SEND_INTERVAL 60000

String HOLOGRAM_DEVICE_KEY = "LC2tG5cf";
String HOLOGRAM_ANALOG_TOPIC = "ANALOG";

const char HOLOGRAM_URL[] = "cloudsocket.hologram.io";
const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;

#define ANALOG_PINS_TO_READ 6
int readPins[ANALOG_PINS_TO_READ] = {A0, A1, A2, A3, A4, A5};
String pinNames[ANALOG_PINS_TO_READ] = {"A0", "A1", "A2", "A3", "A4", "A5"};

boolean doSendAnalog = false;
boolean doServerListen = true;
int listenSocket = -1;

void processSocketRead(int socket, String response) {
    if (response == "read_analog") {
        doSendAnalog = true;
    } else {
        Serial.println("Server read: " + String(response));
    }
}

void processSocketClose(int socket) {
    Serial.println("Socket " + String(socket) + " closed");
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

  lte.setSocketReadCallback(&processSocketRead);
  lte.setSocketCloseCallback(&processSocketClose);
}

void loop() {
    lte.poll();

    if (doSendAnalog) {
        Serial.println("Sending analog values");
        sendAnalogValues();
        doSendAnalog = false;
    }

    if (doServerListen) {
        listenHologramMessage();
        doServerListen = false;
    }
}

void listenHologramMessage()
{
  int sock = -1;
  LTE_Shield_error_t err;

  sock = lte.socketOpen(LTE_SHIELD_TCP);
  if (sock >= 0) {
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
