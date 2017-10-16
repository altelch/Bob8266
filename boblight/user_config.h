/*
  user_config.h - user specific configuration for Sonoff-Tasmota

  Copyright (C) 2017  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*********************************************************************************************\
 * ATTENTION: Changes to most PARAMETER defines will only override flash settings if you change
 *            define CFG_HOLDER.
 *            Most parameters can be changed online using commands via MQTT, WebConsole or serial
 *
 * Corresponding MQTT/Serial/Console commands in [brackets]
\*********************************************************************************************/

// -- Localization --------------------------------
//#define MY_LANGUAGE            en-GB           // Enabled by Default

// -- Project -------------------------------------
#define PROJECT                "boblight"        // PROJECT is used as the default topic delimiter and OTA file name
                                                 //   As an IDE restriction it needs to be the same as the main .ino file
#define CFG_HOLDER             0x20171015        // [Reset 1] Change this value to load following default configuration parameters
#define SAVE_DATA              1                 // [SaveData] Save changed parameters to Flash (0 = disable, 1 - 3600 seconds)
#define SAVE_STATE             1                 // [SetOption0] Save changed power state to Flash (0 = disable, 1 = enable)

// -- Wifi ----------------------------------------
#define WIFI_IP_ADDRESS        "0.0.0.0"         // [IpAddress1] Set to 0.0.0.0 for using DHCP or IP address
#define WIFI_GATEWAY           "192.168.2.254"   // {IpAddress2] If not using DHCP set Gateway IP address
#define WIFI_SUBNETMASK        "255.255.255.0"   // [IpAddress3] If not using DHCP set Network mask
#define WIFI_DNS               "192.168.2.27"    // [IpAddress4] If not using DHCP set DNS IP address (might be equal to WIFI_GATEWAY)

#define STA_SSID1              "krupp.net"       // [Ssid1] Wifi SSID
#define STA_PASS1              "password"        // [Password1] Wifi password
#define STA_SSID2              "krupp.net"       // [Ssid2] Optional alternate AP Wifi SSID
#define STA_PASS2              "password"        // [Password2] Optional alternate AP Wifi password
#define WIFI_CONFIG_TOOL       WIFI_MANAGER      // [WifiConfig] Default tool if wifi fails to connect
                                                 //   (WIFI_RESTART, WIFI_SMARTCONFIG, WIFI_MANAGER, WIFI_WPSCONFIG, WIFI_RETRY, WIFI_WAIT)

// -- Log --------------------------------------
#define SERIAL_LOG_LEVEL       LOG_LEVEL_DEBUG   // [SerialLog]
#define WEB_LOG_LEVEL          LOG_LEVEL_INFO    // [WebLog]

// -- Ota -----------------------------------------
#define OTA_URL                "http://krupp.net:80/api/arduino/" PROJECT ".bin"  // [OtaUrl]

// -- HTTP ----------------------------------------
#define USE_WEBSERVER                            // Enable web server and wifi manager (+66k code, +8k mem) - Disable by //
  #define WEB_SERVER           2                 // [WebServer] Web server (0 = Off, 1 = Start as User, 2 = Start as Admin)
  #define WEB_PORT             80                // Web server Port for User and Admin mode
  #define WEB_USERNAME         "admin"           // Web server Admin mode user name
  #define WEB_PASSWORD         ""                // [WebPassword] Web server Admin mode Password for WEB_USERNAME (empty string = Disable)
  #define FRIENDLY_NAME        "Boblight"        // [FriendlyName] Friendlyname up to 32 characters used by webpages and Alexa
  #define EMULATION            EMUL_HUE          // [Emulation] Select Hue Bridge emulation (multi relay/light) (EMUL_NONE, EMUL_HUE)

// -- TCP -----------------------------------------
#define USE_BOBLIGHT                             // Enable boblight emulation
  #define BOB_PORT             19333             // Default boblightd tcp listening port

// -- Time - Up to three NTP servers in your region
#define NTP_SERVER1            "pool.ntp.org"       // [NtpServer1] Select first NTP server by name or IP address (129.250.35.250)
#define NTP_SERVER2            "de.pool.ntp.org"    // [NtpServer2] Select second NTP server by name or IP address (5.39.184.5)
#define NTP_SERVER3            "0.de.pool.ntp.org"  // [NtpServer3] Select third NTP server by name or IP address (93.94.224.67)

// -- Time - Start Daylight Saving Time and timezone offset from UTC in minutes
#define TIME_DST               North, Last, Sun, Mar, 2, +120  // Northern Hemisphere, Last sunday in march at 02:00 +120 minutes

// -- Time - Start Standard Time and timezone offset from UTC in minutes
#define TIME_STD               North, Last, Sun, Oct, 3, +60   // Northern Hemisphere, Last sunday in october 02:00 +60 minutes

// -- Application ---------------------------------
#define APP_TIMEZONE           1                 // [Timezone] +1 hour (Amsterdam) (-12 .. 12 = hours from UTC, 99 = use TIME_DST/TIME_STD)
#define APP_LEDSTATE           LED_POWER         // [LedState] Function of led (LED_OFF, LED_POWER)
#define APP_POWERON_STATE      3                 // [PowerOnState] Power On Relay state (0 = Off, 1 = On, 2 = Toggle Saved state, 3 = Saved state)
#define APP_SLEEP              5                 // [Sleep] Sleep time to lower energy consumption (0 = Off, 1 - 250 mSec)

#define WS2812_LEDS            30                // [Pixels] Number of WS2812 LEDs to start with
#define USE_IR_REMOTE                            // Send IR remote commands using library IRremoteESP8266 and ArduinoJson (+3k code, 0.3k mem)
#define USE_WS2812_CTYPE     1                   // WS2812 Color type (0 - RGB, 1 - GRB)
//  #define USE_WS2812_DMA                         // DMA supports only GPIO03 (= Serial RXD) (+1k mem)
                                                 //   When USE_WS2812_DMA is enabled expect Exceptions on Pow

/*********************************************************************************************\
 * No user configurable items below
\*********************************************************************************************/

#if (ARDUINO < 10610)
  #error "This software is supported with Arduino IDE starting from 1.6.10 and ESP8266 Release 2.3.0"
#endif

