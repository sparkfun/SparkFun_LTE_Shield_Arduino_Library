/*
  Use the GPS Request feature to request GPS data and receive it in a callback
  By: Jim Lindblom
  SparkFun Electronics
  Date: November 1, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to use the gpsRequest feature

  Before beginning, you should have your shield connected to a supported u-blox
  GPS module via the I2C (DDC) port.
  Supported GPS modules include:
  https://www.sparkfun.com/products/15005

  Once programmed, open the serial monitor, set the baud rate to 9600,
  and hit enter to watch GPS data begin to stream by.
  
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

boolean requestingGPS = false;
unsigned long lastRequest = 0;
#define MIN_REQUEST_INTERVAL 60000 // How often we'll get GPS in loop (in ms)

#define GPS_REQUEST_TIMEOUT 30 // Time to turn on GPS module and get a fix (in s)
#define GPS_REQUEST_ACCURACY 1  // Desired accuracy from GPS module (in meters)

// processGpsRead is provided to the LTE_Shield library via a 
// callback setter -- setGpsReadCallback. (See end of setup())
void processGpsRead(ClockData clck, PositionData gps, 
  SpeedData spd, unsigned long uncertainty) {

  Serial.println();
  Serial.println();
  Serial.println(F("GPS Data Received"));
  Serial.println(F("================="));
  Serial.println("Date: " + String(clck.date.month) + "/" + 
    String(clck.date.day) + "/" + String(clck.date.year));
  Serial.println("Time: " + String(clck.time.hour) + ":" + 
    String(clck.time.minute) + ":" + String(clck.time.second) + "." + String(clck.time.ms));
  Serial.println("Lat/Lon: " + String(gps.lat, 7) + "/" + String(gps.lon, 7));
  Serial.println("Alt: " + String(gps.alt));
  Serial.println("Uncertainty: " + String(uncertainty));
  Serial.println("Speed: " + String(spd.speed) + " @ " + String(spd.track));
  Serial.println();

  requestingGPS = false;
}

void setup() {
  Serial.begin(9600);

  // Wait for user to press key in terminal to begin
  Serial.println("Press any key to begin GPS'ing");
  while (!Serial.available()) ;
  while (Serial.available()) Serial.read();

  // Initialize the LTE Shield
  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  // Set a callback to return GPS data once requested
  lte.setGpsReadCallback(&processGpsRead);
}

void loop() {
  // Poll as often as possible
  lte.poll();

  if (!requestingGPS) {
    if ((lastRequest == 0) || (lastRequest + MIN_REQUEST_INTERVAL < millis())) {
        Serial.println(F("Requesting GPS data...this can take up to 10 seconds"));
        if (lte.gpsRequest(GPS_REQUEST_TIMEOUT, GPS_REQUEST_ACCURACY) == LTE_SHIELD_SUCCESS) {
          Serial.println(F("GPS data requested."));
          Serial.println("Wait up to " + String(GPS_REQUEST_TIMEOUT) + " seconds");
          requestingGPS = true;
          lastRequest = millis();
        } else {
          Serial.println(F("Error requesting GPS"));
        }
      }
  } else {
    // Print a '.' every ~1 second if requesting GPS data
    // (Hopefully this doesn't mess with poll too much)
    if ((millis() % 1000) == 0) {
      Serial.print('.');
      delay(1);
    }
  }


}