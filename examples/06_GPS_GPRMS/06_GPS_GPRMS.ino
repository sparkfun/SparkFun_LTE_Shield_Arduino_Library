/*
  Use the GPS RMC sentence read feature to get position, speed, and clock data
  By: Jim Lindblom
  SparkFun Electronics
  Date: November 2, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to use the gpsGetRmc feature.

  Before beginning, you should have your shield connected to a supported u-blox
  GPS module via the I2C (DDC) port.
  Supported GPS modules include:
  https://www.sparkfun.com/products/15005

  Once programmed, open the serial monitor, set the baud rate to 9600,
  and hit enter to watch GPS data begin to stream by.
  
  Hardware Connections:
  Attach the SparkFun LTE Cat M1/NB-IoT Shield to your Arduino
  Power the shield with your Arduino -- ensure the PWR_SEL switch is in
    the "ARDUINO" gpsition.
*/

//Click here to get the library: http://librarymanager/All#SparkFun_LTE_Shield_Arduino_Library
#include <SparkFun_LTE_Shield_Arduino_Library.h>

// Create a SoftwareSerial object to pass to the LTE_Shield library
SoftwareSerial lteSerial(8, 9);
// Create a LTE_Shield object to use throughout the sketch
LTE_Shield lte;

PositionData gps;
SpeedData spd;
ClockData clk;
boolean valid;

#define GPS_POLL_RATE 5000 // Read GPS every 2 seconds
unsigned long lastGpsPoll = 0;

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

  // Enable the GPS's RMC sentence output. This will also turn the
  // GPS module on (lte.gpsPower(true)) if it's not already
  if (lte.gpsEnableRmc(true) != LTE_SHIELD_SUCCESS) {
    Serial.println(F("Error initializing GPS."));
    while (1) ;
  }
}

void loop() {
  if ((lastGpsPoll == 0) || (lastGpsPoll + GPS_POLL_RATE < millis())) {
    // Call lte.gpsGetRmc to get coordinate, speed, and timing data
    // from the GPS module. Valid can be used to check if the GPS is
    // reporting valid data
    if (lte.gpsGetRmc(&gps, &spd, &clk, &valid) == LTE_SHIELD_SUCCESS) {
      printGPS();
      lastGpsPoll = millis();
    } else {
      delay(1000); // If RMC read fails, wait a second and try again
    }
  }
}

void printGPS(void) {
  Serial.println();
  Serial.println("UTC: " + String(gps.utc));
  Serial.print("Time: ");
  if (clk.time.hour < 10) Serial.print('0'); // Print leading 0
  Serial.print(String(clk.time.hour) + ":");
  if (clk.time.minute < 10) Serial.print('0'); // Print leading 0
  Serial.print(String(clk.time.minute) + ":");
  if (clk.time.second < 10) Serial.print('0'); // Print leading 0
  Serial.println(clk.time.second);
  Serial.println("Latitude: " + String(gps.lat, 7) + " " + String(gps.latDir));
  Serial.println("Longitude: " + String(gps.lon, 7) + " " + String(gps.lonDir));
  Serial.println("Speed: " + String(spd.speed, 4) + " @ " + String(spd.track, 4));
  Serial.println("Date: " + String(clk.date.month) + "/" + 
    String(clk.date.day) + "/" + String(clk.date.year));
  Serial.println("Magnetic variation: " + String(spd.magVar) + " " + spd.magVarDir);
  Serial.println("Status: " + String(gps.status));
  Serial.println("Mode: " + String(gps.mode));
  Serial.println();
}