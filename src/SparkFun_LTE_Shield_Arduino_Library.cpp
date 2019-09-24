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

#include <SparkFun_LTE_Shield_Arduino_Library.h>

#define LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT 1000
#define LTE_SHIELD_SET_BAUD_TIMEOUT 500
#define LTE_SHIELD_POWER_PULSE_PERIOD 3200
#define LTE_RESET_PULSE_PERIOD 10000
#define LTE_SHIELD_IP_CONNECT_TIMEOUT 60000
#define LTE_SHIELD_POLL_DELAY 1
#define LTE_SHIELD_SOCKET_WRITE_TIMEOUT 10000

// ## Suported AT Commands
// ### General
const char LTE_SHIELD_COMMAND_AT[] = "AT";      // AT "Test"
const char LTE_SHIELD_COMMAND_ECHO[] = "E";     // Local Echo
const char LTE_SHIELD_COMMAND_IMEI[] = "+CGSN"; // IMEI identification
const char LTE_SHIELD_COMMAND_IMSI[] = "+CIMI"; // IMSI identification
const char LTE_SHIELD_COMMAND_CCID[] = "+CCID"; // SIM CCID
// ### Control and status
const char LTE_SHIELD_COMMAND_FUNC[] = "+CFUN";    // Functionality (reset, etc.)
const char LTE_SHIELD_COMMAND_CLOCK[] = "+CCLK";   // Clock
const char LTE_SHIELD_COMMAND_AUTO_TZ[] = "+CTZU"; // Automatic time zone update
// ### Network service
const char LTE_SHIELD_COMMAND_MNO[] = "+UMNOPROF"; // MNO (mobile network operator) Profile
const char LTE_SHIELD_SIGNAL_QUALITY[] = "+CSQ";
const char LTE_SHIELD_REGISTRATION_STATUS[] = "+CREG";
const char LTE_SHIELD_MESSAGE_PDP_DEF[] = "+CGDCONT";
const char LTE_SHIELD_MESSAGE_ENTER_PPP[] = "D";
const char LTE_SHIELD_OPERATOR_SELECTION[] = "+COPS";
// V24 control and V25ter (UART interface)
const char LTE_SHIELD_COMMAND_BAUD[] = "+IPR"; // Baud rate
// ### GPIO
const char LTE_SHIELD_COMMAND_GPIO[] = "+UGPIOC"; // GPIO Configuration
// ### IP
const char LTE_SHIELD_CREATE_SOCKET[] = "+USOCR";  // Create a new socket
const char LTE_SHIELD_CLOSE_SOCKET[] = "+USOCL";   // Close a socket
const char LTE_SHIELD_CONNECT_SOCKET[] = "+USOCO"; // Connect to server on socket
const char LTE_SHIELD_WRITE_SOCKET[] = "+USOWR";   // Write data to a socket
const char LTE_SHIELD_READ_SOCKET[] = "+USORD";    // Read from a socket
const char LTE_SHIELD_LISTEN_SOCKET[] = "+USOLI";  // Listen for connection on socket
// ### SMS
const char LTE_SHIELD_MESSAGE_FORMAT[] = "+CMGF"; // Set SMS message format
const char LTE_SHIELD_SEND_TEXT[] = "+CMGS";      // Send SMS message
// ### GPS
const char LTE_SHIELD_GPS_POWER[] = "+UGPS";
const char LTE_SHIELD_GPS_REQUEST_LOCATION[] = "+ULOC";
const char LTE_SHIELD_GPS_GPRMC[] = "+UGRMC";

const char LTE_SHIELD_RESPONSE_OK[] = "OK\r\n";

// CTRL+Z and ESC ASCII codes for SMS message sends
const char ASCII_CTRL_Z = 0x1A;
const char ASCII_ESC = 0x1B;

#define NOT_AT_COMMAND false
#define AT_COMMAND true

#define LTE_SHIELD_NUM_SOCKETS 6

#define NUM_SUPPORTED_BAUD 6
const unsigned long LTE_SHIELD_SUPPORTED_BAUD[NUM_SUPPORTED_BAUD] =
    {
        115200,
        9600,
        19200,
        38400,
        57600,
        230400};
#define LTE_SHIELD_DEFAULT_BAUD_RATE 115200

char lteShieldRXBuffer[128];

static boolean parseGPRMCString(char *rmcString, PositionData *pos, ClockData *clk, SpeedData *spd);

LTE_Shield::LTE_Shield(uint8_t powerPin, uint8_t resetPin)
{
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    _softSerial = NULL;
#endif
    _hardSerial = NULL;
    _baud = 0;
    _resetPin = resetPin;
    _powerPin = powerPin;
    _socketReadCallback = NULL;
    _socketCloseCallback = NULL;
    _lastRemoteIP = {0, 0, 0, 0};
    _lastLocalIP = {0, 0, 0, 0};

    memset(lteShieldRXBuffer, 0, 128);
}

#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
boolean LTE_Shield::begin(SoftwareSerial &softSerial, unsigned long baud)
{
    LTE_Shield_error_t err;

    _softSerial = &softSerial;

    err = init(baud);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        return true;
    }
    return false;
}
#endif

boolean LTE_Shield::begin(HardwareSerial &hardSerial, unsigned long baud)
{
    LTE_Shield_error_t err;

    _hardSerial = &hardSerial;

    err = init(baud);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        return true;
    }
    return false;
}

boolean LTE_Shield::poll(void)
{
    int avail = 0;
    char c = 0;
    bool handled = false;

    memset(lteShieldRXBuffer, 0, 128);

    if (hwAvailable())
    {
        while (c != '\n')
        {
            if (hwAvailable())
            {
                c = readChar();
                lteShieldRXBuffer[avail++] = c;
            }
        }
        {
            int socket, length;
            if (sscanf(lteShieldRXBuffer, "+UUSORD: %d,%d", &socket, &length) == 2)
            {
                parseSocketReadIndication(socket, length);
                handled = true;
            }
        }
        {
            int socket, listenSocket;
            unsigned int port, listenPort;
            IPAddress remoteIP, localIP;

            if (sscanf(lteShieldRXBuffer,
                       "+UUSOLI: %d,\"%d.%d.%d.%d\",%u,%d,\"%d.%d.%d.%d\",%u",
                       &socket,
                       &remoteIP[0], &remoteIP[1], &remoteIP[2], &remoteIP[3],
                       &port, &listenSocket,
                       &localIP[0], &localIP[1], &localIP[2], &localIP[3],
                       &listenPort) > 4)
            {
                parseSocketListenIndication(localIP, remoteIP);
                handled = true;
            }
        }
        {
            int socket;

            if (sscanf(lteShieldRXBuffer,
                       "+UUSOCL: %d", &socket) == 1)
            {
                if ((socket >= 0) && (socket <= 6))
                {
                    if (_socketCloseCallback != NULL)
                    {
                        _socketCloseCallback(socket);
                    }
                }
                handled = true;
            }
        }
        {
            ClockData clck;
            PositionData gps;
            SpeedData spd;
            unsigned long uncertainty;
            int scanNum;
            unsigned int latH, lonH, altU, speedU, trackU;
            char latL[10], lonL[10];

            if (strstr(lteShieldRXBuffer, "+UULOC"))
            {
                // Found a Location string!
                scanNum = sscanf(lteShieldRXBuffer,
                                 "+UULOC: %hhu/%hhu/%u,%hhu:%hhu:%hhu.%u,%u.%[^,],%u.%[^,],%u,%lu,%u,%u,*%s",
                                 &clck.date.day, &clck.date.month, &clck.date.year,
                                 &clck.time.hour, &clck.time.minute, &clck.time.second, &clck.time.ms,
                                 &latH, latL, &lonH, lonL, &altU, &uncertainty,
                                 &speedU, &trackU);
                if (scanNum < 13)
                    return false; // Break out if we didn't find enough

                gps.lat = (float)latH + ((float)atol(latL) / pow(10, strlen(latL)));
                gps.lon = (float)lonH + ((float)atol(lonL) / pow(10, strlen(lonL)));
                gps.alt = (float)altU;
                if (scanNum == 15) // If detailed response, get speed data
                {
                    spd.speed = (float)speedU;
                    spd.track = (float)trackU;
                }

                if (_gpsRequestCallback != NULL)
                {
                    _gpsRequestCallback(clck, gps, spd, uncertainty);
                }
            }
        }

        if ((handled == false) && (strlen(lteShieldRXBuffer) > 2))
        {
            //Serial.println("Poll: " + String(lteShieldRXBuffer));
        }
        else
        {
        }

        return handled;
    }
}

void LTE_Shield::setSocketReadCallback(void (*socketReadCallback)(int, String))
{
    _socketReadCallback = socketReadCallback;
}

void LTE_Shield::setSocketCloseCallback(void (*socketCloseCallback)(int))
{
    _socketCloseCallback = socketCloseCallback;
}

void LTE_Shield::setGpsReadCallback(void (*gpsRequestCallback)(ClockData time,
                                                               PositionData gps, SpeedData spd, unsigned long uncertainty))
{
    _gpsRequestCallback = gpsRequestCallback;
}

size_t LTE_Shield::write(uint8_t c)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->write(c);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->write(c);
    }
#endif
    return (size_t)0;
}

size_t LTE_Shield::write(const char *str)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->print(str);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->print(str);
    }
#endif
    return (size_t)0;
}

size_t LTE_Shield::write(const char *buffer, size_t size)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->print(buffer);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->print(buffer);
    }
#endif
    return (size_t)0;
}

LTE_Shield_error_t LTE_Shield::at(void)
{
    LTE_Shield_error_t err;
    char *command;

    err = sendCommandWithResponse(NULL, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    return err;
}

LTE_Shield_error_t LTE_Shield::enableEcho(boolean enable)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_ECHO) + 2);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    if (enable)
    {
        sprintf(command, "%s1", LTE_SHIELD_COMMAND_ECHO);
    }
    else
    {
        sprintf(command, "%s0", LTE_SHIELD_COMMAND_ECHO);
    }
    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);

    return err;
}

String LTE_Shield::imei(void)
{
    char *response;
    char imeiResponse[16];
    LTE_Shield_error_t err;

    response = lte_calloc_char(sizeof(imeiResponse) + 16);

    err = sendCommandWithResponse(LTE_SHIELD_COMMAND_IMEI,
                                  LTE_SHIELD_RESPONSE_OK, response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        if (sscanf(response, "\r\n%s\r\n", imeiResponse) != 1)
        {
            memset(imeiResponse, 0, 16);
        }
    }
    free(response);
    return String(imeiResponse);
    ;
}

String LTE_Shield::imsi(void)
{
    char *response;
    char imsiResponse[16];
    LTE_Shield_error_t err;

    response = lte_calloc_char(sizeof(imsiResponse) + 16);

    err = sendCommandWithResponse(LTE_SHIELD_COMMAND_IMSI,
                                  LTE_SHIELD_RESPONSE_OK, response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        if (sscanf(response, "\r\n%s\r\n", imsiResponse) != 1)
        {
            memset(imsiResponse, 0, 16);
        }
    }
    free(response);
    return String(imsiResponse);
}

String LTE_Shield::ccid(void)
{
    char *response;
    char ccidResponse[21];
    LTE_Shield_error_t err;

    response = lte_calloc_char(sizeof(ccidResponse) + 16);

    err = sendCommandWithResponse(LTE_SHIELD_COMMAND_CCID,
                                  LTE_SHIELD_RESPONSE_OK, response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        if (sscanf(response, "\r\n+CCID: %s", ccidResponse) != 1)
        {
            memset(ccidResponse, 0, 21);
        }
    }
    free(response);
    return String(ccidResponse);
}

LTE_Shield_error_t LTE_Shield::reset(void)
{
    LTE_Shield_error_t err;

    err = functionality(SILENT_RESET);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        // Reset will set the baud rate back to 115200
        //beginSerial(9600);
        err = LTE_SHIELD_ERROR_INVALID;
        while (err != LTE_SHIELD_ERROR_SUCCESS)
        {
            beginSerial(LTE_SHIELD_DEFAULT_BAUD_RATE);
            setBaud(_baud);
            delay(200);
            beginSerial(_baud);
            err = at();
            delay(500);
        }
        return init(_baud);
    }
    return err;
}

String LTE_Shield::clock(void)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char *clockBegin;
    char *clockEnd;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_CLOCK) + 2);
    if (command == NULL)
        return "";
    sprintf(command, "%s?", LTE_SHIELD_COMMAND_CLOCK);

    response = lte_calloc_char(48);
    if (response == NULL)
    {
        free(command);
        return "";
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return "";

    // Response format: \r\n+CCLK: "YY/MM/DD,HH:MM:SS-TZ"\r\n\r\nOK\r\n
    clockBegin = strchr(response, '\"'); // Find first quote
    if (clockBegin == NULL)
    {
        free(command);
        free(response);
        return "";
    }
    clockBegin += 1;                     // Increment pointer to begin at first number
    clockEnd = strchr(clockBegin, '\"'); // Find last quote
    if (clockEnd == NULL)
    {
        free(command);
        free(response);
        return "";
    }
    *(clockEnd) = '\0'; // Set last quote to null char -- end string

    free(command);
    free(response);

    return String(clockBegin);
}

LTE_Shield_error_t LTE_Shield::clock(uint8_t *y, uint8_t *mo, uint8_t *d,
                                     uint8_t *h, uint8_t *min, uint8_t *s, uint8_t *tz)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char *clockBegin;
    char *clockEnd;

    int iy, imo, id, ih, imin, is, itz;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_CLOCK) + 2);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_COMMAND_CLOCK);

    response = lte_calloc_char(48);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    // Response format: \r\n+CCLK: "YY/MM/DD,HH:MM:SS-TZ"\r\n\r\nOK\r\n
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        if (sscanf(response, "\r\n+CCLK: \"%d/%d/%d,%d:%d:%d-%d\"\r\n",
                   &iy, &imo, &id, &ih, &imin, &is, &itz) == 7)
        {
            *y = iy;
            *mo = imo;
            *d = id;
            *h = ih;
            *min = imin;
            *s = is;
            *tz = itz;
        }
    }

    return err;
}

LTE_Shield_error_t LTE_Shield::autoTimeZone(boolean enable)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_AUTO_TZ) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_COMMAND_AUTO_TZ, enable ? 1 : 0);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    return err;
}

int8_t LTE_Shield::rssi(void)
{
    char *command;
    char *response;
    LTE_Shield_error_t err;
    int rssi;

    command = lte_calloc_char(strlen(LTE_SHIELD_SIGNAL_QUALITY) + 1);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s", LTE_SHIELD_SIGNAL_QUALITY);

    response = lte_calloc_char(48);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command,
                                  LTE_SHIELD_RESPONSE_OK, response, 10000, AT_COMMAND);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return -1;

    if (sscanf(response, "\r\n+CSQ: %d,%*d", &rssi) != 1)
    {
        rssi = -1;
    }

    free(command);
    free(response);
    return rssi;
}

LTE_Shield_registration_status_t LTE_Shield::registration(void)
{
    char *command;
    char *response;
    LTE_Shield_error_t err;
    int status;

    command = lte_calloc_char(strlen(LTE_SHIELD_REGISTRATION_STATUS) + 2);
    if (command == NULL)
        return LTE_SHIELD_REGISTRATION_INVALID;
    sprintf(command, "%s?", LTE_SHIELD_REGISTRATION_STATUS);

    response = lte_calloc_char(48);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_REGISTRATION_INVALID;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT, AT_COMMAND);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return LTE_SHIELD_REGISTRATION_INVALID;

    if (sscanf(response, "\r\n+CREG: %*d,%d", &status) != 1)
    {
        status = LTE_SHIELD_REGISTRATION_INVALID;
    }
    free(command);
    free(response);
    return (LTE_Shield_registration_status_t)status;
}

boolean LTE_Shield::setNetwork(mobile_network_operator_t mno)
{
    mobile_network_operator_t currentMno;

    // Check currently set MNO
    if (getMno(&currentMno) != LTE_SHIELD_ERROR_SUCCESS)
    {
        return false;
    }
    if (currentMno == mno)
    {
        return true;
    }

    if (functionality(MINIMUM_FUNCTIONALITY) != LTE_SHIELD_ERROR_SUCCESS)
    {
        return false;
    }

    if (setMno(mno) != LTE_SHIELD_ERROR_SUCCESS)
    {
        return false;
    }

    if (reset() != LTE_SHIELD_ERROR_SUCCESS)
    {
        return false;
    }

    return true;
}

mobile_network_operator_t LTE_Shield::getNetwork(void)
{
    mobile_network_operator_t mno;
    LTE_Shield_error_t err;

    err = getMno(&mno);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
    {
        return MNO_INVALID;
    }
    return mno;
}

LTE_Shield_error_t LTE_Shield::setAPN(String apn, uint8_t cid, LTE_Shield_pdp_type pdpType)
{
    LTE_Shield_error_t err;
    char *command;
    char pdpStr[8];

    memset(pdpStr, 0, 8);

    if (cid >= 8)
        return LTE_SHIELD_ERROR_UNEXPECTED_PARAM;

    command = lte_calloc_char(strlen(LTE_SHIELD_MESSAGE_PDP_DEF) + strlen(apn.c_str()) + 16);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    switch (pdpType)
    {
    case PDP_TYPE_INVALID:
        return LTE_SHIELD_ERROR_UNEXPECTED_PARAM;
        break;
    case PDP_TYPE_IP:
        memcpy(pdpStr, "IP", 2);
        break;
    case PDP_TYPE_NONIP:
        memcpy(pdpStr, "NONIP", 2);
        break;
    case PDP_TYPE_IPV4V6:
        memcpy(pdpStr, "IPV4V6", 2);
        break;
    case PDP_TYPE_IPV6:
        memcpy(pdpStr, "IPV6", 2);
        break;
    }
    sprintf(command, "%s=%d,\"%s\",\"%s\"", LTE_SHIELD_MESSAGE_PDP_DEF,
            cid, pdpStr, apn.c_str());

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);

    return err;
}

LTE_Shield_error_t LTE_Shield::getAPN(String *apn, IPAddress *ip)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char *searchPtr;
    int ipOctets[4];

    command = lte_calloc_char(strlen(LTE_SHIELD_MESSAGE_PDP_DEF) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_MESSAGE_PDP_DEF);

    response = lte_calloc_char(128);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        // Example: +CGDCONT: 1,"IP","hologram","10.170.241.191",0,0,0,0
        searchPtr = strstr(response, "+CGDCONT: ");
        if (searchPtr != NULL)
        {
            searchPtr += strlen("+CGDCONT: ");
            // Search to the third double-quote
            for (int i = 0; i < 3; i++)
            {
                searchPtr = strchr(++searchPtr, '\"');
            }
            if (searchPtr != NULL)
            {
                // Fill in the APN:
                searchPtr = strchr(searchPtr, '\"'); // Move to first quote
                while ((*(++searchPtr) != '\"') && (searchPtr != '\0'))
                {
                    apn->concat(*(searchPtr));
                }
                // Now get the IP:
                if (searchPtr != NULL)
                {
                    int scanned = sscanf(searchPtr, "\",\"%d.%d.%d.%d\"",
                                         &ipOctets[0], &ipOctets[1], &ipOctets[2], &ipOctets[3]);
                    if (scanned == 4)
                    {
                        for (int octet = 0; octet < 4; octet++)
                        {
                            (*ip)[octet] = (uint8_t)ipOctets[octet];
                        }
                    }
                }
            }
        }
        else
        {
            err = LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
        }
    }

    free(command);
    free(response);

    return err;
}

const char *PPP_L2P[5] = {
    "",
    "PPP",
    "M-HEX",
    "M-RAW_IP",
    "M-OPT-PPP",
};

LTE_Shield_error_t LTE_Shield::enterPPP(uint8_t cid, char dialing_type_char,
                                        unsigned long dialNumber, LTE_Shield::LTE_Shield_l2p_t l2p)
{
    LTE_Shield_error_t err;
    char *command;

    if ((dialing_type_char != 0) && (dialing_type_char != 'T') &&
        (dialing_type_char != 'P'))
    {
        return LTE_SHIELD_ERROR_UNEXPECTED_PARAM;
    }

    command = lte_calloc_char(strlen(LTE_SHIELD_MESSAGE_ENTER_PPP) + 32);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    if (dialing_type_char != 0)
    {
        sprintf(command, "%s%c*%lu**%s*%hhu#", LTE_SHIELD_MESSAGE_ENTER_PPP, dialing_type_char,
                dialNumber, PPP_L2P[l2p], cid);
    }
    else
    {
        sprintf(command, "%s*%lu**%s*%hhu#", LTE_SHIELD_MESSAGE_ENTER_PPP,
                dialNumber, PPP_L2P[l2p], cid);
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);
    return err;
}

uint8_t LTE_Shield::getOperators(struct operator_stats *opRet, int maxOps)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    uint8_t opsSeen = 0;

    command = lte_calloc_char(strlen(LTE_SHIELD_OPERATOR_SELECTION) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=?", LTE_SHIELD_OPERATOR_SELECTION);

    response = lte_calloc_char(maxOps * 48 + 16);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    // AT+COPS maximum response time is 3 minutes (180000 ms)
    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response,
                                  180000);

    // Sample responses:
    // +COPS: (3,"Verizon Wireless","VzW","311480",8),,(0,1,2,3,4),(0,1,2)
    // +COPS: (1,"313 100","313 100","313100",8),(2,"AT&T","AT&T","310410",8),(3,"311 480","311 480","311480",8),,(0,1,2,3,4),(0,1,2)

    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        char *opBegin;
        char *opEnd;
        int op = 0;
        int sscanRead = 0;
        int stat;
        char longOp[26];
        char shortOp[11];
        int act;
        unsigned long numOp;

        opBegin = response;

        for (; op < maxOps; op++)
        {
            opBegin = strchr(opBegin, '(');
            if (opBegin == NULL)
                break;
            opEnd = strchr(opBegin, ')');
            if (opEnd == NULL)
                break;

            int sscanRead = sscanf(opBegin, "(%d,\"%[^\"]\",\"%[^\"]\",\"%lu\",%d)%*s",
                                   &stat, longOp, shortOp, &numOp, &act);
            if (sscanRead == 5)
            {
                opRet[op].stat = stat;
                opRet[op].longOp = (String)(longOp);
                opRet[op].shortOp = (String)(shortOp);
                opRet[op].numOp = numOp;
                opRet[op].act = act;
                opsSeen += 1;
            }
            // TODO: Search for other possible patterns here
            else
            {
                break; // Break out if pattern doesn't match.
            }
            opBegin = opEnd + 1; // Move opBegin to beginning of next value
        }
    }

    free(command);
    free(response);

    return opsSeen;
}

LTE_Shield_error_t LTE_Shield::registerOperator(struct operator_stats oper)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_OPERATOR_SELECTION) + 24);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=1,2,\"%lu\"", LTE_SHIELD_OPERATOR_SELECTION, oper.numOp);

    // AT+COPS maximum response time is 3 minutes (180000 ms)
    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  180000);

    return err;
}

LTE_Shield_error_t LTE_Shield::getOperator(String *oper)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char *searchPtr;
    char mode;

    command = lte_calloc_char(strlen(LTE_SHIELD_OPERATOR_SELECTION) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_OPERATOR_SELECTION);

    response = lte_calloc_char(64);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    // AT+COPS maximum response time is 3 minutes (180000 ms)
    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response,
                                  180000);

    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        searchPtr = strstr(response, "+COPS: ");
        if (searchPtr != NULL)
        {
            searchPtr += strlen("+COPS: "); //  Move searchPtr to first char
            mode = *searchPtr;              // Read first char -- should be mode
            if (mode == '2')                // Check for de-register
            {
                err = LTE_SHIELD_ERROR_DEREGISTERED;
            }
            // Otherwise if it's default, manual, set-only, or automatic
            else if ((mode == '0') || (mode == '1') || (mode == '3') || (mode == '4'))
            {
                *oper = "";
                searchPtr = strchr(searchPtr, '\"'); // Move to first quote
                if (searchPtr == NULL)
                {
                    err = LTE_SHIELD_ERROR_DEREGISTERED;
                }
                else
                {
                    while ((*(++searchPtr) != '\"') && (searchPtr != '\0'))
                    {
                        oper->concat(*(searchPtr));
                    }
                }
                //Serial.println("Operator: " + *oper);
                //oper->concat('\0');
            }
        }
    }

    free(response);
    free(command);
    return err;
}

LTE_Shield_error_t LTE_Shield::deregisterOperator(void)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_OPERATOR_SELECTION) + 4);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=2", LTE_SHIELD_OPERATOR_SELECTION);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    return err;
}

LTE_Shield_error_t LTE_Shield::setSMSMessageFormat(lte_shield_message_format_t textMode)
{
    char *command;
    LTE_Shield_error_t err;

    command = lte_calloc_char(strlen(LTE_SHIELD_MESSAGE_FORMAT) + 4);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_MESSAGE_FORMAT,
            (textMode == LTE_SHIELD_MESSAGE_FORMAT_TEXT) ? 1 : 0);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);
    return err;
}

LTE_Shield_error_t LTE_Shield::sendSMS(String number, String message)
{
    char *command;
    char *messageCStr;
    char *numberCStr;
    int messageIndex;
    LTE_Shield_error_t err;

    numberCStr = lte_calloc_char(number.length() + 2);
    if (numberCStr == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    number.toCharArray(numberCStr, number.length() + 1);

    command = lte_calloc_char(strlen(LTE_SHIELD_SEND_TEXT) + strlen(numberCStr) + 8);
    if (command != NULL)
    {
        sprintf(command, "%s=\"%s\"", LTE_SHIELD_SEND_TEXT, numberCStr);

        err = sendCommandWithResponse(command, ">", NULL,
                                      LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
        free(command);
        free(numberCStr);
        if (err != LTE_SHIELD_ERROR_SUCCESS)
            return err;

        messageCStr = lte_calloc_char(message.length() + 1);
        if (messageCStr == NULL)
        {
            hwWrite(ASCII_CTRL_Z);
            return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
        }
        message.toCharArray(messageCStr, message.length() + 1);
        messageCStr[message.length()] = ASCII_CTRL_Z;

        err = sendCommandWithResponse(messageCStr, LTE_SHIELD_RESPONSE_OK,
                                      NULL, 180000, NOT_AT_COMMAND);
    }
    else
    {
        err = LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    free(messageCStr);

    return err;
}

LTE_Shield_error_t LTE_Shield::setBaud(unsigned long baud)
{
    LTE_Shield_error_t err;
    char *command;
    int b = 0;

    // Error check -- ensure supported baud
    for (; b < NUM_SUPPORTED_BAUD; b++)
    {
        if (LTE_SHIELD_SUPPORTED_BAUD[b] == baud)
        {
            break;
        }
    }
    if (b >= NUM_SUPPORTED_BAUD)
    {
        return LTE_SHIELD_ERROR_UNEXPECTED_PARAM;
    }

    // Construct command
    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_BAUD) + 7 + 12);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%lu", LTE_SHIELD_COMMAND_BAUD, baud);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_SET_BAUD_TIMEOUT);

    free(command);

    return err;
}

LTE_Shield_error_t LTE_Shield::setGpioMode(LTE_Shield_gpio_t gpio,
                                           LTE_Shield_gpio_mode_t mode)
{
    LTE_Shield_error_t err;
    char *command;

    // Example command: AT+UGPIOC=16,2
    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_GPIO) + 7);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d,%d", LTE_SHIELD_COMMAND_GPIO, gpio, mode);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);

    return err;
}

LTE_Shield::LTE_Shield_gpio_mode_t LTE_Shield::getGpioMode(LTE_Shield_gpio_t gpio)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char gpioChar[4];
    char *gpioStart;
    int gpioMode;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_GPIO) + 2);
    if (command == NULL)
        return GPIO_MODE_INVALID;
    sprintf(command, "%s?", LTE_SHIELD_COMMAND_GPIO);

    response = lte_calloc_char(96);
    if (response == NULL)
    {
        free(command);
        return GPIO_MODE_INVALID;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    if (err != LTE_SHIELD_ERROR_SUCCESS)
    {
        free(command);
        free(response);
        return GPIO_MODE_INVALID;
    }

    sprintf(gpioChar, "%d", gpio);          // Convert GPIO to char array
    gpioStart = strstr(response, gpioChar); // Find first occurence of GPIO in response
    if (gpioStart == NULL)
        return GPIO_MODE_INVALID; // If not found return invalid
    sscanf(gpioStart, "%*d,%d\r\n", &gpioMode);

    free(command);
    free(response);

    return (LTE_Shield_gpio_mode_t)gpioMode;
}

int LTE_Shield::socketOpen(lte_shield_socket_protocol_t protocol, unsigned int localPort)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    int sockId = -1;
    char *responseStart;

    command = lte_calloc_char(strlen(LTE_SHIELD_CREATE_SOCKET) + 10);
    if (command == NULL)
        return -1;
    sprintf(command, "%s=%d,%d", LTE_SHIELD_CREATE_SOCKET, protocol, localPort);

    response = lte_calloc_char(24);
    if (response == NULL)
    {
        free(command);
        return -1;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    if (err != LTE_SHIELD_ERROR_SUCCESS)
    {
        free(command);
        free(response);
        return -1;
    }

    responseStart = strstr(response, "+USOCR");
    if (responseStart == NULL)
    {
        free(command);
        free(response);
        return -1;
    }

    sscanf(responseStart, "+USOCR: %d", &sockId);

    free(command);
    free(response);

    return sockId;
}

LTE_Shield_error_t LTE_Shield::socketClose(int socket, int timeout)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_CLOSE_SOCKET) + 10);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_CLOSE_SOCKET, socket);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL, timeout);

    free(command);

    return err;
}

LTE_Shield_error_t LTE_Shield::socketConnect(int socket, const char *address,
                                             unsigned int port)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_CONNECT_SOCKET) + strlen(address) + 11);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d,\"%s\",%d", LTE_SHIELD_CONNECT_SOCKET, socket, address, port);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL, LTE_SHIELD_IP_CONNECT_TIMEOUT);

    free(command);

    return err;
}

LTE_Shield_error_t LTE_Shield::socketWrite(int socket, const char *str)
{
    char *command;
    LTE_Shield_error_t err;

    command = lte_calloc_char(strlen(LTE_SHIELD_WRITE_SOCKET) + 8);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d,%d", LTE_SHIELD_WRITE_SOCKET, socket, strlen(str));

    err = sendCommandWithResponse(command, "@", NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    hwPrint(str);

    err = waitForResponse(LTE_SHIELD_RESPONSE_OK, LTE_SHIELD_SOCKET_WRITE_TIMEOUT);

    free(command);
    return err;
}

LTE_Shield_error_t LTE_Shield::socketWrite(int socket, String str)
{
    return socketWrite(socket, str.c_str());
}

LTE_Shield_error_t LTE_Shield::socketRead(int socket, int length, char *readDest)
{
    char *command;
    char *response;
    char *strBegin;
    int readIndex = 0;
    LTE_Shield_error_t err;

    command = lte_calloc_char(strlen(LTE_SHIELD_READ_SOCKET) + 8);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d,%d", LTE_SHIELD_READ_SOCKET, socket, length);

    response = lte_calloc_char(length + strlen(LTE_SHIELD_READ_SOCKET) + 24);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        // Find the first double-quote:
        strBegin = strchr(response, '\"');

        if (strBegin == NULL)
        {
            free(command);
            free(response);
            return LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
        }

        while ((readIndex < length) && (readIndex < strlen(strBegin)))
        {
            readDest[readIndex] = strBegin[1 + readIndex];
            readIndex += 1;
        }
    }

    free(command);
    free(response);

    return err;
}

LTE_Shield_error_t LTE_Shield::socketListen(int socket, unsigned int port)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_LISTEN_SOCKET) + 9);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d,%d", LTE_SHIELD_LISTEN_SOCKET, socket, port);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);
    return err;
}

IPAddress LTE_Shield::lastRemoteIP(void)
{
    return _lastRemoteIP;
}

boolean LTE_Shield::gpsOn(void)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    boolean on = false;

    command = lte_calloc_char(strlen(LTE_SHIELD_GPS_POWER) + 2);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_GPS_POWER);

    response = lte_calloc_char(24);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response,
                                  LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        // Example response: "+UGPS: 0" for off "+UGPS: 1,0,1" for on
        // May be too lazy/simple, but just search for a '1'
        if (strchr(response, '1') != NULL)
            on = true;
    }

    free(command);
    free(response);

    return on;
}

LTE_Shield_error_t LTE_Shield::gpsPower(boolean enable, gnss_system_t gnss_sys)
{
    LTE_Shield_error_t err;
    char *command;
    boolean gpsState;

    // Don't turn GPS on/off if it's already on/off
    gpsState = gpsOn();
    if ((enable && gpsState) || (!enable && !gpsState))
    {
        return LTE_SHIELD_ERROR_SUCCESS;
    }

    command = lte_calloc_char(strlen(LTE_SHIELD_GPS_POWER) + 8);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    if (enable)
    {
        sprintf(command, "%s=1,0,%d", LTE_SHIELD_GPS_POWER, gnss_sys);
    }
    else
    {
        sprintf(command, "%s=0", LTE_SHIELD_GPS_POWER);
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL, 10000);

    free(command);
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnableClock(boolean enable)
{
    // AT+UGZDA=<0,1>
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetClock(struct ClockData *clock)
{
    // AT+UGZDA?
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnableFix(boolean enable)
{
    // AT+UGGGA=<0,1>
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetFix(struct PositionData *pos)
{
    // AT+UGGGA?
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnablePos(boolean enable)
{
    // AT+UGGLL=<0,1>
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetPos(struct PositionData *pos)
{
    // AT+UGGLL?
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnableSat(boolean enable)
{
    // AT+UGGSV=<0,1>
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetSat(uint8_t *sats)
{
    // AT+UGGSV?
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnableRmc(boolean enable)
{
    // AT+UGRMC=<0,1>
    LTE_Shield_error_t err;
    char *command;

    if (!gpsOn())
    {
        err = gpsPower(true);
        if (err != LTE_SHIELD_ERROR_SUCCESS)
        {
            return err;
        }
    }

    command = lte_calloc_char(strlen(LTE_SHIELD_GPS_GPRMC) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_GPS_GPRMC, enable ? 1 : 0);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL, 10000);

    free(command);
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetRmc(struct PositionData *pos, struct SpeedData *spd,
                                         struct ClockData *clk, boolean *valid)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    char *rmcBegin;

    command = lte_calloc_char(strlen(LTE_SHIELD_GPS_GPRMC) + 2);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_GPS_GPRMC);

    response = lte_calloc_char(96);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, response, 10000);
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        // Fast-forward response string to $GPRMC starter
        rmcBegin = strstr(response, "$GPRMC");
        if (rmcBegin == NULL)
        {
            err = LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
        }
        else
        {
            *valid = parseGPRMCString(rmcBegin, pos, clk, spd);
        }
    }

    free(command);
    free(response);
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsEnableSpeed(boolean enable)
{
    // AT+UGVTG=<0,1>
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsGetSpeed(struct SpeedData *speed)
{
    // AT+UGVTG?
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_SUCCESS;
    return err;
}

LTE_Shield_error_t LTE_Shield::gpsRequest(unsigned int timeout, uint32_t accuracy,
                                          boolean detailed)
{
    // AT+ULOC=2,<useCellLocate>,<detailed>,<timeout>,<accuracy>
    LTE_Shield_error_t err;
    char *command;

    // This function will only work if the GPS module is initially turned off.
    if (gpsOn())
    {
        gpsPower(false);
    }

    if (timeout > 999)
        timeout = 999;
    if (accuracy > 999999)
        accuracy = 999999;

    command = lte_calloc_char(strlen(LTE_SHIELD_GPS_REQUEST_LOCATION) + 24);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=2,3,%d,%d,%d", LTE_SHIELD_GPS_REQUEST_LOCATION,
            detailed ? 1 : 0, timeout, accuracy);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK, NULL, 10000);

    free(command);
    return err;
}

/////////////
// Private //
/////////////

LTE_Shield_error_t LTE_Shield::init(unsigned long baud,
                                    LTE_Shield::LTE_Shield_init_type_t initType)
{
    LTE_Shield_error_t err;

    beginSerial(baud); // Begin serial

    if (initType == LTE_SHIELD_INIT_AUTOBAUD)
    {
        if (autobaud(baud) != LTE_SHIELD_ERROR_SUCCESS)
        {
            return init(baud, LTE_SHIELD_INIT_RESET);
        }
    }
    else if (initType == LTE_SHIELD_INIT_RESET)
    {
        powerOn();
        if (at() != LTE_SHIELD_ERROR_SUCCESS)
        {
            return init(baud, LTE_SHIELD_INIT_AUTOBAUD);
        }
    }

    // Use disable echo to test response
    err = enableEcho(false);

    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return init(baud, LTE_SHIELD_INIT_AUTOBAUD);

    _baud = baud;
    setGpioMode(GPIO1, NETWORK_STATUS);
    setGpioMode(GPIO2, GNSS_SUPPLY_ENABLE);
    setSMSMessageFormat(LTE_SHIELD_MESSAGE_FORMAT_TEXT);
    autoTimeZone(true);
    for (int i = 0; i < LTE_SHIELD_NUM_SOCKETS; i++)
    {
        socketClose(i, 100);
    }

    return LTE_SHIELD_ERROR_SUCCESS;
}

void LTE_Shield::powerOn(void)
{
    pinMode(_powerPin, OUTPUT);
    digitalWrite(_powerPin, LOW);
    delay(LTE_SHIELD_POWER_PULSE_PERIOD);
    pinMode(_powerPin, INPUT); // Return to high-impedance, rely on SARA module internal pull-up
}

void LTE_Shield::hwReset(void)
{
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, LOW);
    delay(LTE_RESET_PULSE_PERIOD);
    pinMode(_resetPin, INPUT); // Return to high-impedance, rely on SARA module internal pull-up
}

LTE_Shield_error_t LTE_Shield::functionality(LTE_Shield_functionality_t function)
{
    LTE_Shield_error_t err;
    char *command;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_FUNC) + 4);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_COMMAND_FUNC, function);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);

    return err;
}

LTE_Shield_error_t LTE_Shield::setMno(mobile_network_operator_t mno)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_MNO) + 3);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s=%d", LTE_SHIELD_COMMAND_MNO, (uint8_t)mno);

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  NULL, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);

    free(command);
    free(response);

    return err;
}

LTE_Shield_error_t LTE_Shield::getMno(mobile_network_operator_t *mno)
{
    LTE_Shield_error_t err;
    char *command;
    char *response;
    const char *mno_keys = "0123456"; // Valid MNO responses
    int i;

    command = lte_calloc_char(strlen(LTE_SHIELD_COMMAND_MNO) + 2);
    if (command == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    sprintf(command, "%s?", LTE_SHIELD_COMMAND_MNO);

    response = lte_calloc_char(24);
    if (response == NULL)
    {
        free(command);
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }

    err = sendCommandWithResponse(command, LTE_SHIELD_RESPONSE_OK,
                                  response, LTE_SHIELD_STANDARD_RESPONSE_TIMEOUT);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return err;

    i = strcspn(response, mno_keys); // Find first occurence of MNO key
    if (i == strlen(response))
    {
        *mno = MNO_INVALID;
        return LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
    }
    *mno = (mobile_network_operator_t)(response[i] - 0x30); // Convert to integer

    free(command);
    free(response);

    return err;
}

/*LTE_Shield_error_t LTE_Shield::sendCommandWithResponseAndTimeout(const char * command, 
    char * expectedResponse, uint16_t commandTimeout, boolean at)
{
    unsigned long timeIn = millis();
    char * response;

    sendCommand(command, at);

    // Wait until we've receved the requested number of characters
    while (hwAvailable() < strlen(expectedResponse))
    {
        if (millis() > timeIn + commandTimeout)
        {
            return LTE_SHIELD_ERROR_TIMEOUT;
        }
    }
    response = lte_calloc_char(hwAvailable() + 1);
    if (response == NULL)
    {
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;
    }
    readAvailable(response);
    
    // Check for expected response
    if (strcmp(response, expectedResponse) == 0)
    {
        return LTE_SHIELD_ERROR_SUCCESS;
    }
    return LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
}*/

LTE_Shield_error_t LTE_Shield::waitForResponse(const char *expectedResponse, uint16_t timeout)
{
    unsigned long timeIn;
    boolean found = false;
    int index = 0;

    timeIn = millis();

    while ((!found) && (timeIn + timeout > millis()))
    {
        if (hwAvailable())
        {
            char c = readChar();
            if (c == expectedResponse[index])
            {
                if (++index == strlen(expectedResponse))
                {
                    found = true;
                }
            }
            else
            {
                index = 0;
            }
        }
    }
    return found ? LTE_SHIELD_ERROR_SUCCESS : LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
}

LTE_Shield_error_t LTE_Shield::sendCommandWithResponse(
    const char *command, const char *expectedResponse, char *responseDest,
    unsigned long commandTimeout, boolean at)
{
    unsigned long timeIn;
    boolean found = false;
    int index = 0;
    int destIndex = 0;
    unsigned int charsRead = 0;

    //Serial.print("Command: ");
    //Serial.println(String(command));
    sendCommand(command, at);
    //Serial.print("Response: ");
    timeIn = millis();
    while ((!found) && (timeIn + commandTimeout > millis()))
    {
        if (hwAvailable())
        {
            char c = readChar();
            //Serial.write(c);
            if (responseDest != NULL)
            {
                responseDest[destIndex++] = c;
            }
            charsRead++;
            if (c == expectedResponse[index])
            {
                if (++index == strlen(expectedResponse))
                {
                    found = true;
                }
            }
            else
            {
                index = 0;
            }
        }
    }
    //Serial.println();

    if (found)
    {
        return LTE_SHIELD_ERROR_SUCCESS;
    }
    else if (charsRead == 0)
    {
        return LTE_SHIELD_ERROR_NO_RESPONSE;
    }
    else
    {
        return LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
    }
}

boolean LTE_Shield::sendCommand(const char *command, boolean at)
{
    readAvailable(NULL); // Clear out receive buffer before sending a new command

    if (at)
    {
        hwPrint(LTE_SHIELD_COMMAND_AT);
        hwPrint(command);
        hwPrint("\r");
    }
    else
    {
        hwPrint(command);
    }

    return true;
}

LTE_Shield_error_t LTE_Shield::parseSocketReadIndication(int socket, int length)
{
    LTE_Shield_error_t err;
    char *readDest;

    if ((socket < 0) || (length < 0))
    {
        return LTE_SHIELD_ERROR_UNEXPECTED_RESPONSE;
    }

    readDest = lte_calloc_char(length + 1);
    if (readDest == NULL)
        return LTE_SHIELD_ERROR_OUT_OF_MEMORY;

    err = socketRead(socket, length, readDest);
    if (err != LTE_SHIELD_ERROR_SUCCESS)
        return err;

    if (_socketReadCallback != NULL)
    {
        _socketReadCallback(socket, String(readDest));
    }

    free(readDest);
    return LTE_SHIELD_ERROR_SUCCESS;
}

LTE_Shield_error_t LTE_Shield::parseSocketListenIndication(IPAddress localIP, IPAddress remoteIP)
{
    _lastLocalIP = localIP;
    _lastRemoteIP = remoteIP;
}

LTE_Shield_error_t LTE_Shield::parseSocketCloseIndication(String *closeIndication)
{
    LTE_Shield_error_t err;
    int search;
    int socket;

    search = closeIndication->indexOf("UUSOCL: ") + strlen("UUSOCL: ");

    // Socket will be first integer, should be single-digit number between 0-6:
    socket = closeIndication->substring(search, search + 1).toInt();

    if (_socketCloseCallback != NULL)
    {
        _socketCloseCallback(socket);
    }

    return LTE_SHIELD_ERROR_SUCCESS;
}

size_t LTE_Shield::hwPrint(const char *s)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->print(s);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->print(s);
    }
#endif

    return (size_t)0;
}

size_t LTE_Shield::hwWrite(const char c)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->write(c);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->write(c);
    }
#endif

    return (size_t)0;
}

int LTE_Shield::readAvailable(char *inString)
{
    int len = 0;

    if (_hardSerial != NULL)
    {
        while (_hardSerial->available())
        {
            char c = (char)_hardSerial->read();
            if (inString != NULL)
            {
                inString[len++] = c;
            }
        }
        if (inString != NULL)
        {
            inString[len] = 0;
        }
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    if (_softSerial != NULL)
    {
        while (_softSerial->available())
        {
            char c = (char)_softSerial->read();
            if (inString != NULL)
            {
                inString[len++] = c;
            }
        }
        if (inString != NULL)
        {
            inString[len] = 0;
        }
    }
#endif

    return len;
}

char LTE_Shield::readChar(void)
{
    char ret;

    if (_hardSerial != NULL)
    {
        ret = (char)_hardSerial->read();
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        ret = (char)_softSerial->read();
    }
#endif

    return ret;
}

int LTE_Shield::hwAvailable(void)
{
    if (_hardSerial != NULL)
    {
        return _hardSerial->available();
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        return _softSerial->available();
    }
#endif

    return -1;
}

void LTE_Shield::beginSerial(unsigned long baud)
{
    if (_hardSerial != NULL)
    {
        _hardSerial->begin(baud);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        _softSerial->begin(baud);
    }
#endif
    delay(100);
}

void LTE_Shield::setTimeout(unsigned long timeout)
{
    if (_hardSerial != NULL)
    {
        _hardSerial->setTimeout(timeout);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        _softSerial->setTimeout(timeout);
    }
#endif
}

bool LTE_Shield::find(char *target)
{
    if (_hardSerial != NULL)
    {
        _hardSerial->find(target);
    }
#ifdef LTE_SHIELD_SOFTWARE_SERIAL_ENABLED
    else if (_softSerial != NULL)
    {
        _softSerial->find(target);
    }
#endif
}

LTE_Shield_error_t LTE_Shield::autobaud(unsigned long desiredBaud)
{
    LTE_Shield_error_t err = LTE_SHIELD_ERROR_INVALID;
    int b = 0;

    while ((err != LTE_SHIELD_ERROR_SUCCESS) && (b < NUM_SUPPORTED_BAUD))
    {
        beginSerial(LTE_SHIELD_SUPPORTED_BAUD[b++]);
        setBaud(desiredBaud);
        delay(200);
        beginSerial(desiredBaud);
        err = at();
    }
    if (err == LTE_SHIELD_ERROR_SUCCESS)
    {
        beginSerial(desiredBaud);
    }
    return err;
}

char *LTE_Shield::lte_calloc_char(size_t num)
{
    return (char *)calloc(num, sizeof(char));
}

// GPS Helper Functions:

// Read a source string until a delimiter is hit, store the result in destination
static char *readDataUntil(char *destination, unsigned int destSize,
                           char *source, char delimiter)
{

    char *strEnd;
    size_t len;

    strEnd = strchr(source, delimiter);

    if (strEnd != NULL)
    {
        len = strEnd - source;
        memset(destination, 0, destSize);
        memcpy(destination, source, len);
    }

    return strEnd;
}

#define TEMP_NMEA_DATA_SIZE 16

static boolean parseGPRMCString(char *rmcString, PositionData *pos,
                                ClockData *clk, SpeedData *spd)
{
    char *ptr, *search;
    unsigned long tTemp;
    char tempData[TEMP_NMEA_DATA_SIZE];

    // Fast-forward test to first value:
    ptr = strchr(rmcString, ',');
    ptr++; // Move ptr past first comma

    // If the next character is another comma, there's no time data
    // Find time:
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    // Next comma should be present and not the next position
    if ((search != NULL) && (search != ptr))
    {
        pos->utc = atof(tempData);
        tTemp = pos->utc;
        clk->time.hour = tTemp / 10000;
        tTemp -= ((unsigned long)clk->time.hour * 10000);
        clk->time.minute = tTemp / 100;
        tTemp -= ((unsigned long)clk->time.minute * 100);
        clk->time.second = tTemp;
    }
    else
    {
        pos->utc = 0.0;
        clk->time.hour = 0;
        clk->time.minute = 0;
        clk->time.second = 0;
    }
    ptr = search + 1; // Move pointer to next value

    // Find status character:
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    // Should be a single character
    if ((search != NULL) && (search == ptr + 1))
    {
        pos->status = *ptr; // Assign char at ptr to status
    }
    else
    {
        pos->status = 'X'; // Made up very bad status
    }
    ptr = search + 1;

    // Find latitude:
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        pos->lat = atof(tempData);
    }
    else
    {
        pos->lat = 0.0;
    }
    ptr = search + 1;
    // Find latitude hemishpere
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search == ptr + 1))
    {
        pos->latDir = *ptr; // Assign char at ptr to status
    }
    else
    {
        pos->latDir = 'X';
    }
    ptr = search + 1;

    // Find longitude:
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        pos->lon = atof(tempData);
    }
    else
    {
        pos->lon = 0.0;
    }
    ptr = search + 1;
    // Find latitude hemishpere
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search == ptr + 1))
    {
        pos->lonDir = *ptr; // Assign char at ptr to status
    }
    else
    {
        pos->lonDir = 'X';
    }
    ptr = search + 1;

    // Find speed
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        spd->speed = atof(tempData);
    }
    else
    {
        spd->speed = 0.0;
    }
    ptr = search + 1;
    // Find track
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        spd->track = atof(tempData);
    }
    else
    {
        spd->track = 0.0;
    }
    ptr = search + 1;

    // Find date
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        tTemp = atol(tempData);
        clk->date.day = tTemp / 10000;
        tTemp -= ((unsigned long)clk->date.day * 10000);
        clk->date.month = tTemp / 100;
        tTemp -= ((unsigned long)clk->date.month * 100);
        clk->date.year = tTemp;
    }
    else
    {
        clk->date.day = 0;
        clk->date.month = 0;
        clk->date.year = 0;
    }
    ptr = search + 1;

    // Find magnetic variation in degrees:
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search != ptr))
    {
        spd->magVar = atof(tempData);
    }
    else
    {
        spd->magVar = 0.0;
    }
    ptr = search + 1;
    // Find magnetic variation direction
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, ',');
    if ((search != NULL) && (search == ptr + 1))
    {
        spd->magVarDir = *ptr; // Assign char at ptr to status
    }
    else
    {
        spd->magVarDir = 'X';
    }
    ptr = search + 1;

    // Find position system mode
    search = readDataUntil(tempData, TEMP_NMEA_DATA_SIZE, ptr, '*');
    if ((search != NULL) && (search = ptr + 1))
    {
        pos->mode = *ptr;
    }
    else
    {
        pos->mode = 'X';
    }
    ptr = search + 1;

    if (pos->status == 'A')
    {
        return true;
    }
    return false;
}