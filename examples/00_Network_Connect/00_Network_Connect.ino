#include <SparkFun_LTE_Shield_Arduino_Library.h>

SoftwareSerial lteSerial(8, 9);
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

void setup() {
    Serial.begin(9600);

    Serial.println(F("Initializing the LTE Shield..."));
    Serial.println(F("...this may take ~25 seconds if the shield is off."));
    Serial.println(F("...it may take ~5 seconds if it just turned on."));

  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  Serial.println();

  Serial.println("Getting device/SIM info:");
  // Print device information:
  // IMEI: International Mobile Equipment Identity -- Unique number to identify phone
  Serial.println("IMEI: " + lte.imei());
  // IMSI: International Mobile Subscriber Identity -- Unique number to identify
  // user of a cellular network.
  Serial.println("IMSI: " + lte.imsi());
  // ICCID: Integrated circuit card identifier -- Unique SIM card serial number.
  Serial.println("ICCID: " + lte.ccid());
    Serial.println();

    // Network operator can be set to either:
    // MNO_ATT -- AT&T 
    // MNO_VERIZON -- Verizon
    // MNO_TELSTRA -- Telstra
    // MNO_TMO -- T-Mobile
  if (!lte.setNetwork(MNO_VERIZON)) {
      Serial.println(F("Error setting network. Try cycling power on your Arduino/shield."));
      while (1) ;
  }

  Serial.println("Network set. Ready to go!");
  // RSSI: Received signal strength:
  Serial.println("RSSI: " + String(lte.rssi()));
  // Registration Status
  int regStatus = lte.registration();
  if ((regStatus >= 0) && (regStatus <= 9)) {
    Serial.println("Network registration: " + registrationString[regStatus]);
  }
  if (regStatus > 0) {
      Serial.println("All set. Go to the next example!");
  }
}

void loop() {
}