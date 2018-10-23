#include <SparkFun_LTE_Shield_Arduino_Library.h>

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;

const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;

void processSocketRead(int socket, String response) {
  Serial.println("Read: " + response);
  Serial.println("Socket: " + String(socket));
  Serial.print("Remote IP: ");
  Serial.println(lte.lastRemoteIP());
}

void processSocketClose(int socket) {
    Serial.println("Socket " + String(socket) + " closed");
}

void setup() {
    Serial.begin(9600);

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }

  lte.setSocketReadCallback(&processSocketRead);
  lte.setSocketCloseCallback(&processSocketClose);
  
  listenHologramMessage();
}

void loop() {
    //passthrough();
    lte.poll();
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
      Serial.println("Unable to listen on socket");
    }
  }
  else {
    Serial.println("Unable to open socket");
  }
}

void passthrough() {

  if (Serial.available()) {
    char c = Serial.read();
    lteSerial.write(c);
  }
  if (lteSerial.available()) {
    Serial.write(lteSerial.read());
  }
}