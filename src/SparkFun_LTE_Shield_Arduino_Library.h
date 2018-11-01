/*
  Arduino Library for the SparkFun LTE CAT M1/NB-IoT Shield - SARA-R4

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/14997
  Written by Jim Lindblom @ SparkFun Electronics, September 5, 2018
  
  This Arduino library provides mechanisms to initialize and use
  the SARA-R4 module over either a SoftwareSerial or hardware serial port.

  Supported features include:
    * Network registration -- Register your shield on a MNO
    * SMS messaging -- Send an SMS message
    * TCP/IP Messaging -- Sending data to servers or setting the SARA
        module up as a listening socket.
    * u-blox GPS module support -- Plug in a u-blox GPS module via
        I2C to read it's location data.

  Development environment specifics:
  Arduino IDE 1.8.5
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SPARKFUN_LTE_SHIELD_ARDUINO_LIBRARY_H
#define SPARKFUN_LTE_SHIELD_ARDUINO_LIBRARY_H

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#ifdef ARDUINO_ARCH_AVR               // Arduino AVR boards (Uno, Pro Micro, etc.)
#define LTE_SHIELD_SOFTWARE_SERIAL_ENABLED // Enable software serial
#endif

#ifdef ARDUINO_ARCH_SAMD              // Arduino SAMD boards (SAMD21, etc.)
#define LTE_SHIELD_SOFTWARE_SERIAL_ENABLEDx // Disable software serial
#endif

#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
#include <SoftwareSerial.h>
#endif

#include <IPAddress.h>

#define LTE_SHIELD_POWER_PIN 5
#define LTE_SHIELD_RESET_PIN 6

typedef enum {
    MNO_INVALID = -1,
    MNO_SW_DEFAULT = 0,
    MNO_SIM_ICCD = 1,
    MNO_ATT = 2,
    MNO_VERIZON = 3,
    MNO_TELSTRA = 4,
    MNO_TMO = 5,
    MNO_CT = 6
} mobile_network_operator_t;

typedef enum {
    LTE_SHIELD_ERROR_INVALID = -1,         // 1
    LTE_SHIELD_ERROR_SUCCESS = 0,          // 2
    LTE_SHIELD_ERROR_OUT_OF_MEMORY,        // 3
    LTE_SHIELD_ERROR_TIMEOUT,              // 4
    LTE_SHIELD_ERROR_UNEXPECTED_PARAM,     // 5
    LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE,  // 6
    LTE_SHIELD_ERROR_NO_RESPONSE
} LTE_Shield_error_t;
#define LTE_SHIELD_SUCCESS LTE_SHIELD_ERROR_SUCCESS

typedef enum {
    LTE_SHIELD_REGISTRATION_INVALID = -1,
    LTE_SHIELD_REGISTRATION_NOT_REGISTERED = 0,
    LTE_SHIELD_REGISTRATION_HOME = 1,
    LTE_SHIELD_REGISTRATION_SEARCHING = 2,
    LTE_SHIELD_REGISTRATION_DENIED = 3,
    LTE_SHIELD_REGISTRATION_UNKNOWN = 4,
    LTE_SHIELD_REGISTRATION_ROAMING = 5,
    LTE_SHIELD_REGISTRATION_HOME_SMS_ONLY = 6,
    LTE_SHIELD_REGISTRATION_ROAMING_SMS_ONLY = 7,
    LTE_SHIELD_REGISTRATION_HOME_CSFB_NOT_PREFERRED = 8,
    LTE_SHIELD_REGISTRATION_ROAMING_CSFB_NOT_PREFERRED = 9
} LTE_Shield_registration_status_t;

struct DateData {
    uint8_t day;
    uint8_t month;
    unsigned int year;
};

struct TimeData {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    unsigned int ms;
    uint8_t tzh;
    uint8_t tzm;
};

struct ClockData {
    struct DateData date;
    struct TimeData time;
};

struct PositionData {
    float utc;
    float lat;
    float latDir;
    float lon;
    float lonDir;
    float alt;
};

struct SpeedData {
    float speed;
    float tack;
    float magVar;
};

typedef enum {
    LTE_SHIELD_TCP = 6,
    LTE_SHIELD_UDP = 17
} lte_shield_socket_protocol_t;

typedef enum {
    LTE_SHIELD_MESSAGE_FORMAT_PDU = 0,
    LTE_SHIELD_MESSAGE_FORMAT_TEXT = 1
} lte_shield_message_format_t;

class LTE_Shield : public Print {
public:

    //  Constructor
    LTE_Shield(uint8_t powerPin = LTE_SHIELD_POWER_PIN, uint8_t resetPin = LTE_SHIELD_RESET_PIN);

    // Begin -- initialize BT module and ensure it's connected
    boolean begin(SoftwareSerial & softSerial, unsigned long baud = 9600);
    boolean begin(HardwareSerial & hardSerial, unsigned long baud = 9600);

    // Loop polling and polling setup
    boolean poll(void);
    void setSocketReadCallback(void (*socketReadCallback)(int, String));
    void setSocketCloseCallback(void (*socketCloseCallback)(int));
    void setGpsReadCallback(void (*gpsRequestCallback)(ClockData time, 
        PositionData gps, SpeedData spd, unsigned long uncertainty));

    // Direct write/print to cell serial port
    virtual size_t write(uint8_t c);
    virtual size_t write(const char *str);
    virtual size_t write(const char * buffer, size_t size);

// General AT Commands
    LTE_Shield_error_t at(void);
    LTE_Shield_error_t enableEcho(boolean enable = true);
    String imei(void);
    String imsi(void);
    String ccid(void);

// Control and status AT commands
    LTE_Shield_error_t reset(void);
    String clock(void);
    // TODO: Return a clock struct
    LTE_Shield_error_t clock(uint8_t * y, uint8_t * mo, uint8_t * d, 
        uint8_t * h, uint8_t * min, uint8_t * s, uint8_t * tz);
    LTE_Shield_error_t autoTimeZone(boolean enable);

// Network service AT commands
    int8_t rssi(void);
    LTE_Shield_registration_status_t registration(void);
    boolean setNetwork(mobile_network_operator_t mno);
    mobile_network_operator_t getNetwork(void);

// SMS -- Short Messages Service
    LTE_Shield_error_t setSMSMessageFormat(lte_shield_message_format_t textMode 
        = LTE_SHIELD_MESSAGE_FORMAT_TEXT);
    LTE_Shield_error_t sendSMS(String number, String message);

// V24 Control and V25ter (UART interface) AT commands
    LTE_Shield_error_t setBaud(unsigned long baud);

// GPIO
    // GPIO pin map
    typedef enum {
        GPIO1 = 16,
        GPIO2 = 23,
        GPIO3 = 24,
        GPIO4 = 25,
        GPIO5 = 42,
        GPIO6 = 19
    } LTE_Shield_gpio_t;
    // GPIO pin modes
    typedef enum {
        GPIO_MODE_INVALID = -1,
        GPIO_OUTPUT = 0,
        GPIO_INPUT,
        NETWORK_STATUS,
        GNSS_SUPPLY_ENABLE,
        GNSS_DATA_READY,
        GNSS_RTC_SHARING,
        SIM_CARD_DETECTION,
        HEADSET_DETECTION,
        GSM_TX_BURST_INDICATION,
        MODULE_OPERATING_STATUS_INDICATION,
        MODULE_FUNCTIONALITY_STATUS_INDICATION,
        I2S_DIGITAL_AUDIO_INTERFACE,
        SPI_SERIAL_INTERFACE,
        MASTER_CLOCK_GENRATION,
        UART_INTERFACE,
        WIFI_ENABLE,
        RING_INDICATION,
        LAST_GASP_ENABLE,
        PAD_DISABLED = 255
    } LTE_Shield_gpio_mode_t;
    LTE_Shield_error_t setGpioMode(LTE_Shield_gpio_t gpio, LTE_Shield_gpio_mode_t mode);
    LTE_Shield_gpio_mode_t getGpioMode(LTE_Shield_gpio_t gpio);

    // IP Transport Layer
    int socketOpen(lte_shield_socket_protocol_t protocol, unsigned int localPort = 0);
    LTE_Shield_error_t socketClose(int socket, int timeout = 1000);
    LTE_Shield_error_t socketConnect(int socket, const char * address, unsigned int port);
    LTE_Shield_error_t socketWrite(int socket, const char * str);
    LTE_Shield_error_t socketWrite(int socket, String str);
    LTE_Shield_error_t socketRead(int socket, int length, char * readDest);
    LTE_Shield_error_t socketListen(int socket, unsigned int port);
    IPAddress lastRemoteIP(void);

    // GPS
    typedef enum {
        GNSS_SYSTEM_GPS = 1,
        GNSS_SYSTEM_SBAS = 2,
        GNSS_SYSTEM_GALILEO = 4,
        GNSS_SYSTEM_BEIDOU = 8,
        GNSS_SYSTEM_IMES = 16,
        GNSS_SYSTEM_QZSS = 32,
        GNSS_SYSTEM_GLONASS = 64
    } gnss_system_t;
    boolean gpsOn(void);
    LTE_Shield_error_t gpsPower(boolean enable = true,
        gnss_system_t gnss_sys = GNSS_SYSTEM_GPS);
    LTE_Shield_error_t gpsEnableClock(boolean enable = true);
    LTE_Shield_error_t gpsGetClock(struct ClockData * clock);
    LTE_Shield_error_t gpsEnableFix(boolean enable = true);
    LTE_Shield_error_t gpsGetFix(float * lat, float * lon, 
        unsigned int * alt, uint8_t * quality, uint8_t * sat);
    LTE_Shield_error_t gpsGetFix(struct PositionData * pos);
    LTE_Shield_error_t gpsEnablePos(boolean enable = true);
    LTE_Shield_error_t gpsGetPos(struct PositionData * pos);
    LTE_Shield_error_t gpsEnableSat(boolean enable = true);
    LTE_Shield_error_t gpsGetSat(uint8_t * sats);
    LTE_Shield_error_t gpsEnableRmc(boolean enable = true);
    LTE_Shield_error_t gpsGetRmc(struct PositionData * pos, struct SpeedData * speed,
        struct DateData * date, boolean * valid);
    LTE_Shield_error_t gpsEnableSpeed(boolean enable = true);
    LTE_Shield_error_t gpsGetSpeed(struct SpeedData * speed);

    LTE_Shield_error_t gpsRequest(unsigned int timeout, unsigned int accuracy, boolean detailed = true);

private:

    HardwareSerial * _hardSerial;
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    SoftwareSerial * _softSerial;
#endif

    uint8_t _powerPin;
    uint8_t _resetPin;
    unsigned long _baud;
    IPAddress _lastRemoteIP;
    IPAddress _lastLocalIP;
    
    void (*_socketReadCallback)(int, String);
    void (*_socketCloseCallback)(int);
    void (*_gpsRequestCallback)(ClockData, PositionData, SpeedData, unsigned long);

    typedef enum {
        LTE_SHIELD_INIT_STANDARD,
        LTE_SHIELD_INIT_AUTOBAUD,
        LTE_SHIELD_INIT_RESET
    } LTE_Shield_init_type_t;

    typedef enum {
        MINIMUM_FUNCTIONALITY = 0,
        FULL_FUNCTIONALITY = 1,
        SILENT_RESET = 15,
        SILENT_RESET_W_SIM = 16
    } LTE_Shield_functionality_t;

    LTE_Shield_error_t init(unsigned long baud, LTE_Shield_init_type_t initType = LTE_SHIELD_INIT_STANDARD);

    void powerOn(void);

    LTE_Shield_error_t functionality(LTE_Shield_functionality_t function = FULL_FUNCTIONALITY);

    LTE_Shield_error_t setMno(mobile_network_operator_t mno);
    LTE_Shield_error_t getMno(mobile_network_operator_t * mno);

    // Wait for an expected response (don't send a command)
    LTE_Shield_error_t waitForResponse(char * expectedResponse, uint16_t timeout);

    // Send command with an expected (potentially partial) response, store entire response
    LTE_Shield_error_t sendCommandWithResponse(const char * command, 
        char * expectedResponse, char * responseDest, uint16_t commandTimeout, boolean at = true);

    // Send a command -- prepend AT if at is true
    boolean sendCommand(const char * command, boolean at);

    LTE_Shield_error_t parseSocketReadIndication(int socket, int length);
    LTE_Shield_error_t parseSocketListenIndication(IPAddress localIP, IPAddress remoteIP);
    LTE_Shield_error_t parseSocketCloseIndication(String * closeIndication);

// UART Functions
    size_t hwPrint(const char * s);
    int readAvailable(char * inString);
    char readChar(void);
    int hwAvailable(void);
    void beginSerial(unsigned long baud);
    void setTimeout(unsigned long timeout);
    bool find(char * target);

    LTE_Shield_error_t autobaud(unsigned long desiredBaud);

    char * lte_calloc_char(size_t num);
};

#endif SPARKFUN_LTE_SHIELD_ARDUINO_LIBRARY_H /* SPARKFUN_LTE_SHIELD_ARDUINO_LIBRARY_H */