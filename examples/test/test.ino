#include <SparkFun_LTE_Shield_Arduino_Library.h>
#include <Arduino.h>

SoftwareSerial lteSerial(8, 9);

#define POWER_PIN 5
#define RESET_PIN 6

LTE_Shield lte(POWER_PIN, RESET_PIN);

const char deviceKey[] = "LC2tG5cf";
const char message[] = "hello, world";
const char tag[] = "HELLO";

const char HOLOGRAM_URL[] = "cloudsocket.hologram.io";
const unsigned int HOLOGRAM_PORT = 9999;
const unsigned int HOLOGRAM_LISTEN_PORT = 4010;
const char testString[] = "{\"k\":\"LC2tG5cf\",\"d\":\"goodnight moon!\", \"t\":[\"GOODBYE\"]}";

int loopMode = 0;

void processSocketRead(String response) {
  Serial.println("Read: " + response);
  Serial.print("Remote IP: ");
  Serial.println(lte.lastRemoteIP());
}

void processSocketClose(int socket) {
  Serial.println("Socket " + String(socket) + " closed");
}

void processUnknownRead(String unknown) {

}

void setup() {
  Serial.begin(9600);

  Serial.println(F("Press any key to begin"));
  while (!Serial.available()) ;
  while (Serial.available()) Serial.read();

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  lte.setSocketReadCallback(&processSocketRead);
  lte.setSocketCloseCallback(&processSocketClose);
  
  //lte.setUnknownRead(&processUnknownRead);
  //lte.setNetwork(VERIZON);

  Serial.println("RSSI: " + String(lte.rssi()));
  Serial.println("Reg status: " + String(lte.registration()));
  Serial.println("IMEI: " + lte.imei());
  Serial.println("IMSI: " + lte.imsi());
  Serial.println("CCID: " + lte.ccid());
  
  uint8_t hour, minute, second, year, day, month, timezone;
  lte.clock(&year, &month, &day, &hour, &minute, &second, &timezone);
  Serial.println(String(month) + "/" + String(day) + "/" + String(year));
  Serial.println(String(hour) + ":" + String(minute) + ":" + 
    String(second) + "-" + String(timezone));
}

void loop() {
  if (loopMode == 0) {
    passthrough();
  } else if (loopMode == 1) {
    poll();
  }
}

void poll() {
  lte.poll();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '!') {
      sendHologramMessage();
    }
    if (c == '@') {
      listenHologramMessage();
    }
    if (c == '#') {
      sendSMS();
    }
    if (c == ')') {
      Serial.println(F("Passthrough Mode"));
      loopMode = 0;
    }
  }
}

void passthrough() {

  if (Serial.available()) {
    char c = Serial.read();
    String toPrint = "";
    switch (c) {
    case '!':
      sendHologramMessage();
      break;
    case '@':
      listenHologramMessage();
      break;
    case '#':
      sendSMS();
      break;
    case ')':
      loopMode = 1;
      Serial.println(F("Poll mode"));
      break;
    default:
      lteSerial.write(c);
      break;
    }
    //lteSerial.write(c);
  }
  if (lteSerial.available()) {
    Serial.write(lteSerial.read());
  }
}

void sendHologramMessage()
{
  int s = -1;
  LTE_Shield_error_t err;

  s = lte.socketOpen(LTE_SHIELD_TCP);
  if (s >= 0) {
    //sendingMessage = true;
    if (lte.socketConnect(s, HOLOGRAM_URL, HOLOGRAM_PORT) == LTE_SHIELD_ERROR_SUCCESS) {
      if (lte.socketWrite(s, testString) == LTE_SHIELD_ERROR_SUCCESS)
      {
        //while (sendingMessage) ;
        //lte.socketClose(s);
      }
    }
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
      Serial.println("Unable to listen on socket");
    }
  }
  else {
    Serial.println("Unable to open socket");
  }
}

void sendSMS() {
  Serial.println(F("Sending a text message!"));
  //Serial.print("Err: ");
  lte.sendSMS("11234567890", "Hello, this is a text!");
}