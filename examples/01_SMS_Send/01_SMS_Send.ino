#include <SparkFun_LTE_Shield_Arduino_Library.h>

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;

String destinationNumber = "11234567890";

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
    if (c == '\n') {
        Serial.println("Sending: " + String(message));
        lte.sendSMS(destinationNumber, message);
        message = "";
    } else {
        message += c;
    }
  }
}