/*
  Register your LTE Shield/SIM combo on a mobile network operator
  By: Jim Lindblom
  SparkFun Electronics
  Date: October 23, 2018
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14997

  This example demonstrates how to initialize your Cat M1/NB-IoT shield, and
  connect it to a mobile network operator (Verizon, AT&T, T-Mobile, etc.).

  Before beginning, you may need to adjust the mobile network operator (MNO)
  setting on line 83. See comments above that line to help select either
  Verizon, T-Mobile, AT&T or others.
  
  Hardware Connections:
  Attach the SparkFun LTE Cat M1/NB-IoT Shield to your Arduino
  Power the shield with your Arduino -- ensure the PWR_SEL switch is in
    the "ARDUINO" position.
*/

//Click here to get the library: http://librarymanager/All#SparkFun_LTE_Shield_Arduino_Library
#include <SparkFun_LTE_Shield_Arduino_Library.h>

// We need to pass a Serial or SoftwareSerial object to the LTE Shield 
// library. Below creates a SoftwareSerial object on the standard LTE
// Shield RX/TX pins:
// Note: if you're using an Arduino board with a dedicated hardware
// serial port, comment out the line below. (Also see note in setup.)
SoftwareSerial lteSerial(8, 9);

// Create a LTE_Shield object to be used throughout the sketch:
LTE_Shield lte;

// Map registration status messages to more readable strings
String registrationString[] = {
  "Not registered",                         // 0
  "Registered, home",                       // 1
  "Searching for operator",                 // 2
  "Registration denied",                    // 3
  "Registration unknown",                   // 4
  "Registrered, roaming",                   // 5
  "Registered, home (SMS only)",            // 6
  "Registered, roaming (SMS only)",         // 7
  "Registered, home, CSFB not preferred",   // 8
  "Registered, roaming, CSFB not prefered"  // 9
};

// Network operator can be set to either:
// MNO_ATT -- AT&T 
// MNO_VERIZON -- Verizon
// MNO_TELSTRA -- Telstra
// MNO_TMO -- T-Mobile
const mobile_network_operator_t MOBILE_NETWORK_OPERATOR = MNO_VERIZON;

void setup() {
  Serial.begin(9600);

  Serial.println(F("Initializing the LTE Shield..."));
  Serial.println(F("...this may take ~25 seconds if the shield is off."));
  Serial.println(F("...it may take ~5 seconds if it just turned on."));

  // Call lte.begin and pass it your Serial/SoftwareSerial object to 
  // communicate with the LTE Shield.
  // Note: If you're using an Arduino with a dedicated hardware serial
  // poert, you may instead slide "Serial" into this begin call.
  if ( lte.begin(lteSerial) ) {
    Serial.println(F("LTE Shield connected!"));
  } else {
    Serial.println(F("Unable to communicate with the shield."));
    Serial.println(F("Make sure the serial switch is in the correct position."));
    Serial.println(F("Manually power-on (hold POWER for 3 seconds) on and try again."));
    while (1) ; // Loop forever on fail
  }
  Serial.println();

  Serial.println(F("Getting device/SIM info:"));
  // Print device information:
  String imei, imsi, ccid;
  // IMEI: International Mobile Equipment Identity -- Unique number to identify phone
  imei = lte.imei();
  Serial.println("IMEI: " + imei);
  // IMSI: International Mobile Subscriber Identity -- Unique number to identify
  // user of a cellular network.
  imsi = lte.imsi();
  if (imsi.length() < 10) {
    Serial.println(F("Unable to read the IMSI -- there may be an error."));
    Serial.println(F("Is your SIM card inserted?"));
  } else {
    Serial.println("IMSI: " + imsi);
  }
  // ICCID: Integrated circuit card identifier -- Unique SIM card serial number.
  ccid = lte.ccid();
  if (imsi.length() < 10) {
    Serial.println(F("Unable to read the CCID -- there may be an error."));
    Serial.println(F("Is your SIM card inserted?"));
  } else {
    Serial.println("ICCID: " + ccid);
  }
  Serial.println();

  if (!lte.setNetwork(MOBILE_NETWORK_OPERATOR)) {
    Serial.println(F("Error setting network. Try cycling power on your Arduino/shield."));
    while (1) ;
  }

  Serial.println(F("Network set. Ready to go!"));
  // RSSI: Received signal strength:
  Serial.println("RSSI: " + String(lte.rssi()));
  // Registration Status
  int regStatus = lte.registration();
  if ((regStatus >= 0) && (regStatus <= 9)) {
    Serial.println("Network registration: " + registrationString[regStatus]);
  }
  if (regStatus > 0) {
    Serial.println(F("All set. Go to the next example!"));
  }
}

void loop() {
  // Do nothing. Now that we're registered move on to the next example.
}