 /*
  boblight.ino - Boblight firmware for iTead Sonoff, Wemos and NodeMCU hardware

  Copyright (C) 2017  Heiko Krupp
  
  Based on Sonoff-Tasmota by Theo Arends (C) 2017

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

#define VERSION                0x01000000  // 1.0.0a

enum week_t  {Last, First, Second, Third, Fourth};
enum dow_t   {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
enum month_t {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};
enum hemis_t {North, South};
enum log_t   {LOG_LEVEL_NONE, LOG_LEVEL_ERROR, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG_MORE, LOG_LEVEL_ALL};  // SerialLog, Syslog, Weblog
enum wifi_t  {WIFI_RESTART, WIFI_SMARTCONFIG, WIFI_MANAGER, WIFI_WPSCONFIG, WIFI_RETRY, WIFI_WAIT, MAX_WIFI_OPTION};  // WifiConfig
enum led_t   {LED_OFF, LED_POWER, MAX_LED_OPTION};  // LedState
enum emul_t  {EMUL_NONE, EMUL_HUE, EMUL_MAX};  // Emulation

#include "user_config.h"
#include "user_config_override.h"
#include "i18n.h"
#include "boblight_template.h"

/*********************************************************************************************\
 * No user configurable items below
\*********************************************************************************************/

#define MODULE                 WEMOS        // [Module] Select default model

#define WS2812_LEDS            30           // [Pixels] Number of LEDs

#define DEFAULT_NAME           "Bob8266"    // Default Device Name
#define WIFI_HOSTNAME          "%s-%04d"    // Expands to <NAME>-<last 4 decimal chars of MAC address>
#define CONFIG_FILE_SIGN       0xA5         // Configuration file signature
#define CONFIG_FILE_XOR        0x5A         // Configuration file xor (0 = No Xor)

#define APP_POWER              0            // Default saved power state Off
#define WS2812_MAX_LEDS        512          // Max number of LEDs

#define STATES                 20           // State loops per second
#define OTA_ATTEMPTS           10           // Number of times to try fetching the new firmware

#define INPUT_BUFFER_SIZE      50           // Max number of characters in (serial) command buffer
#define CMDSZ                  20           // Max number of characters in command
#define MAX_LOG_LINES          20           // Max number of lines in weblog
#define MSG_SIZE               512          // Log message size

#define APP_BAUDRATE           115200       // Default serial baudrate
#define MAX_STATUS             11           // Max number of status lines

enum butt_t  {PRESSED, NOT_PRESSED};

#include "support.h"                        // Global support

#include <Ticker.h>                         // RTC, OSWatch
#include <ESP8266WiFi.h>                    // Ota, WifiManager
#include <ESP8266HTTPClient.h>              // Ota
#include <ESP8266httpUpdate.h>              // Ota
#include <StreamString.h>                   // Webserver, Updater
#include <ArduinoJson.h>                    // WemoHue, IRremote, Domoticz
#include <ESP8266WebServer.h>               // WifiManager, Webserver
#include <DNSServer.h>                      // WifiManager
#include "settings.h"

struct TIME_T {
  uint8_t       Second;
  uint8_t       Minute;
  uint8_t       Hour;
  uint8_t       Wday;      // day of week, sunday is day 1
  uint8_t       Day;
  uint8_t       Month;
  char          MonthName[4];
  uint16_t      DayOfYear;
  uint16_t      Year;
  unsigned long Valid;
} rtcTime;

struct TimeChangeRule
{
  uint8_t       hemis;     // 0-Northern, 1=Southern Hemisphere (=Opposite DST/STD)
  uint8_t       week;      // 1=First, 2=Second, 3=Third, 4=Fourth, or 0=Last week of the month
  uint8_t       dow;       // day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t       month;     // 1=Jan, 2=Feb, ... 12=Dec
  uint8_t       hour;      // 0-23
  int           offset;    // offset from UTC in minutes
};

TimeChangeRule myDST = { TIME_DST };  // Daylight Saving Time
TimeChangeRule mySTD = { TIME_STD };  // Standard Time

int Baudrate = APP_BAUDRATE;          // Serial interface baud rate
byte SerialInByte;                    // Received byte
int SerialInByteCounter = 0;          // Index in receive buffer
int16_t savedatacounter;              // Counter and flag for config save to Flash
unsigned long timerxs = 0;            // State loop timer
int state = 0;                        // State per second flag
int otaflag = 0;                      // OTA state flag
int otaok = 0;                        // OTA result
byte otaretry = OTA_ATTEMPTS;         // OTA retry counter
int restartflag = 0;                  // Sonoff restart flag
int wificheckflag = WIFI_RESTART;     // Wifi state flag
int uptime = 0;                       // Current uptime in hours
boolean uptime_flg = true;            // Signal latest uptime
byte logidx = 0;                      // Index in Web log buffer
byte logajaxflg = 0;                  // Reset web console log
byte Maxdevice = 0;                   // Max number of devices supported
int status_update_timer = 0;          // Refresh initial status

WiFiClient espClient;                 // Wifi Client
WiFiUDP portUDP;                      // Alexa
WiFiServer bob(BOB_PORT);
WiFiClient bobClient;

uint8_t power;                        // Current copy of sysCfg.power
byte seriallog_level;                 // Current copy of sysCfg.seriallog_level
uint8_t sleep;                        // Current copy of sysCfg.sleep
uint8_t stop_flash_rotate = 0;        // Allow flash configuration rotation

int blinks = 201;                     // Number of LED blinks
uint8_t blinkstate = 0;               // LED state

uint8_t lastbutton[4] = { NOT_PRESSED, NOT_PRESSED, NOT_PRESSED, NOT_PRESSED };     // Last button states
uint8_t multiwindow[4] = { 0 };       // Max time between button presses to record press count
uint8_t multipress[4] = { 0 };        // Number of button presses within multiwindow
uint8_t blockgpio0 = 4;               // Block GPIO0 for 4 seconds after poweron to workaround Wemos D1 RTS circuit

mytmplt my_module;                    // Active copy of GPIOs
uint8_t pin[GPIO_MAX];                // Possible pin configurations
uint8_t rel_inverted[4] = { 0 };      // Relay inverted flag (1 = (0 = On, 1 = Off))
uint8_t led_inverted[4] = { 0 };      // LED inverted flag (1 = (0 = On, 1 = Off))

char Version[16];                     // Version string from VERSION define
char Hostname[33];                    // Composed Wifi hostname
char serialInBuf[INPUT_BUFFER_SIZE + 2];  // Receive buffer
char log_data[MSG_SIZE];              // Logging
String Log[MAX_LOG_LINES];            // Web log buffer

/********************************************************************************************/

void setRelay(uint8_t rpower)
{
  uint8_t state;

  if (4 == sysCfg.poweronstate) {  // All on and stay on
    power = (1 << Maxdevice) -1;
    rpower = power;
  }
  
//  sl_setPower(rpower);
    
  for (byte i = 0; i < Maxdevice; i++) {
    state = rpower &1;
    if (pin[GPIO_REL1 +i] < 99) {
      digitalWrite(pin[GPIO_REL1 +i], rel_inverted[i] ? !state : state);
    }
    rpower >>= 1;
  }
}

void setLed(uint8_t state)
{
  if (state) {
    state = 1;
  }
  digitalWrite(pin[GPIO_LED1], (led_inverted[0]) ? !state : state);
}

/********************************************************************************************/

void do_cmnd_power(byte device, byte state)
{
// device  = Relay number 1 and up
// state 0 = Relay Off
// state 1 = Relay On
// state 2 = Toggle relay
// state 3 = Show power state

  if ((device < 1) || (device > Maxdevice)) {
    device = 1;
  }
  byte mask = 0x01 << (device -1);

  switch (state) {
    case 0: { // Off
      power &= (0xFF ^ mask);
      break; }
    case 1: // On
      power |= mask;
      break;
    case 2: // Toggle
      power ^= mask;
  }
  setRelay(power);
}

void do_cmnd(char *cmnd)
{
  Serial.println(cmnd);  
}
/********************************************************************************************/

void every_second()
{
  if (blockgpio0) {
    blockgpio0--;
  }

  if ((2 == rtcTime.Minute) && uptime_flg) {
    uptime_flg = false;
    uptime++;
  }
  if ((3 == rtcTime.Minute) && !uptime_flg) {
    uptime_flg = true;
  }
}

/*********************************************************************************************\
 * Button handler with single press only or multi-press
\*********************************************************************************************/

void button_handler()
{
  uint8_t button = NOT_PRESSED;
  uint8_t butt_present = 0;
  uint8_t flag = 0;
  char scmnd[20];

  for (byte i = 0; i < Maxdevice; i++) {
    button = NOT_PRESSED;
    butt_present = 0;

    if ((pin[GPIO_KEY1 +i] < 99) && !blockgpio0) {
      butt_present = 1;
      button = digitalRead(pin[GPIO_KEY1 +i]);
    }

    if (butt_present) {
      if (!sysCfg.flag.button_single) {                 // Allow multi-press
        if (multiwindow[i]) {
          multiwindow[i]--;
        } else {
          if (!restartflag && (multipress[i] > 0) && (multipress[i] < MAX_BUTTON_COMMANDS +3)) {
            flag = 0;
            if (multipress[i] < 3) {                    // Single or Double press
              flag = (sysCfg.flag.button_swap +1 == multipress[i]);
              multipress[i] = 1;
            }
            
            if (multipress[i] < 3) {                  // Single or Double press
              if (WIFI_State()) {                     // WPSconfig, Smartconfig or Wifimanager active
                restartflag = 1;
              } else {
                do_cmnd_power(i + multipress[i], 2);  // Execute Toggle command internally
              }
            } else {                                  // 3 - 7 press
              if (!sysCfg.flag.button_restrict) {
                snprintf_P(scmnd, sizeof(scmnd), commands[multipress[i] -3]);
                Serial.println(scmnd);
              }
            }
            multipress[i] = 0;
          }
        }
      }
    }
    lastbutton[i] = button;
  }
}

/*********************************************************************************************\
 * State loop
\*********************************************************************************************/

void stateloop()
{
  uint8_t power_now;

  timerxs = millis() + (1000 / STATES);
  state++;

/*-------------------------------------------------------------------------------------------*\
 * Every second
\*-------------------------------------------------------------------------------------------*/

  if (STATES == state) {
    state = 0;
    every_second();
  }

/*-------------------------------------------------------------------------------------------*\
 * Every 0.1 second
\*-------------------------------------------------------------------------------------------*/

  if (!(state % (STATES/10))) {
/*    if(bob_sync){
      ws2812_BobSync();
      bob_sync=false;
    } */
  }

/*-------------------------------------------------------------------------------------------*\
 * Every 0.05 second
\*-------------------------------------------------------------------------------------------*/

  button_handler();

/*-------------------------------------------------------------------------------------------*\
 * Every 0.2 second
\*-------------------------------------------------------------------------------------------*/

  if (!(state % ((STATES/10)*2))) {
    if (blinks || restartflag || otaflag) {
      if (restartflag || otaflag) {
        blinkstate = 1;   // Stay lit
      } else {
        blinkstate ^= 1;  // Blink
      }
      if ((!(sysCfg.ledstate &0x08)) && ((sysCfg.ledstate &0x06) || (blinks > 200) || (blinkstate))) {
        setLed(blinkstate);
      }
      if (!blinkstate) {
        blinks--;
        if (200 == blinks) {
          blinks = 0;
        }
      }
    }
  }

/*-------------------------------------------------------------------------------------------*\
 * Every second at 0.2 second interval
\*-------------------------------------------------------------------------------------------*/

  switch (state) {
  case (STATES/10)*2:
    if (otaflag) {
      otaflag--;
      if (2 == otaflag) {
        otaretry = OTA_ATTEMPTS;
        ESPhttpUpdate.rebootOnUpdate(false);
        CFG_Save(1);  // Free flash for OTA update
      }
      if (otaflag <= 0) {
        if (sysCfg.webserver) {
          stopWebserver();
        }
        otaflag = 92;
        otaok = 0;
        otaretry--;
        if (otaretry) {
          otaok = (HTTP_UPDATE_FAILED != ESPhttpUpdate.update(sysCfg.otaUrl));
          if (!otaok) {
            otaflag = 2;
          }
        }
      }
      if (90 == otaflag) {     // Allow reconnect
        otaflag = 0;
        if (otaok) {
          setFlashModeDout();  // Force DOUT for both ESP8266 and ESP8285
        }
        restartflag = 2;       // Restart anyway to keep memory clean webserver
      }
    }
    break;
  case (STATES/10)*4:
    if (savedatacounter) {
      savedatacounter--;
      if (savedatacounter <= 0) {
        if (sysCfg.flag.savestate) {
          byte mask = 0xFF;
          if (!((sysCfg.power &mask) == (power &mask))) {
            sysCfg.power = power;
          }
        } else {
          sysCfg.power = 0;
        }
        CFG_Save(0);
        savedatacounter = sysCfg.savedata;
      }
    }
    if (restartflag) {
      if (211 == restartflag) {
        CFG_Default();
        restartflag = 2;
      }
      if (212 == restartflag) {
        CFG_Erase();
        CFG_Default();
        restartflag = 2;
      }
      if (sysCfg.flag.savestate) {
        sysCfg.power = power;
      } else {
        sysCfg.power = 0;
      }
      CFG_Save(0);
      restartflag--;
      if (restartflag <= 0) {
        ESP.restart();
      }
    }
    break;
  case (STATES/10)*6:
    WIFI_Check(wificheckflag);
    wificheckflag = WIFI_RESTART;
    break;
  case (STATES/10)*8:
    if (WL_CONNECTED == WiFi.status()) {
    }
    break;
  }
}

/********************************************************************************************/

void serial()
{
  while (Serial.available()) {
    yield();
    SerialInByte = Serial.read();
  }
}

/********************************************************************************************/

void GPIO_init()
{
  uint8_t mpin;
  mytmplt def_module;

  if (!sysCfg.module || (sysCfg.module >= MAXMODULE)) {
    sysCfg.module = MODULE;
  }

  memcpy_P(&def_module, &modules[sysCfg.module], sizeof(def_module));
  strlcpy(my_module.name, def_module.name, sizeof(my_module.name));
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    if (sysCfg.my_module.gp.io[i] > GPIO_NONE) {
      my_module.gp.io[i] = sysCfg.my_module.gp.io[i];
    }
    if ((def_module.gp.io[i] > GPIO_NONE) && (def_module.gp.io[i] < GPIO_USER)) {
      my_module.gp.io[i] = def_module.gp.io[i];
    }
  }

  for (byte i = 0; i < GPIO_MAX; i++) {
    pin[i] = 99;
  }
  for (byte i = 0; i < MAX_GPIO_PIN; i++) {
    mpin = my_module.gp.io[i];

    if (mpin) {
      if ((mpin >= GPIO_REL1_INV) && (mpin <= GPIO_REL4_INV)) {
        rel_inverted[mpin - GPIO_REL1_INV] = 1;
        mpin -= (GPIO_REL1_INV - GPIO_REL1);
      }
      else if ((mpin >= GPIO_LED1_INV) && (mpin <= GPIO_LED4_INV)) {
        led_inverted[mpin - GPIO_LED1_INV] = 1;
        mpin -= (GPIO_LED1_INV - GPIO_LED1);
      }
    }
    if (mpin) {
      pin[mpin] = i;
    }
  }

  if (2 == pin[GPIO_TXD]) {
    Serial.set_tx(2);
  }

  Maxdevice = 0;
  
  for (byte i = 0; i < 4; i++) {
    if (pin[GPIO_REL1 +i] < 99) {
      pinMode(pin[GPIO_REL1 +i], OUTPUT);
      Maxdevice++;
    }
  }
  
  for (byte i = 0; i < 4; i++) {
    if (pin[GPIO_KEY1 +i] < 99) {
      pinMode(pin[GPIO_KEY1 +i], (16 == pin[GPIO_KEY1 +i]) ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
    if (pin[GPIO_LED1 +i] < 99) {
      pinMode(pin[GPIO_LED1 +i], OUTPUT);
      digitalWrite(pin[GPIO_LED1 +i], led_inverted[i]);
    }
  }

  if (pin[GPIO_WS2812] < 99) {  // RGB led
    Maxdevice++;
  }
  
  led_init();

  setLed(sysCfg.ledstate &8);

  if (pin[GPIO_IRSEND] < 99) {
    ir_send_init();
  }
}

extern "C" {
extern struct rst_info resetInfo;
}

void setup()
{
  byte idx;

  Serial.begin(Baudrate);
  delay(10);
  Serial.println();
  seriallog_level = LOG_LEVEL_INFO;  // Allow specific serial messages until config loaded

  snprintf_P(Version, sizeof(Version), PSTR("%d.%d.%d"), VERSION >> 24 & 0xff, VERSION >> 16 & 0xff, VERSION >> 8 & 0xff);
  if (VERSION & 0x1f) {
    idx = strlen(Version);
    Version[idx] = 96 + (VERSION & 0x1f);
    Version[idx +1] = 0;
  }

  CFG_Load();
  CFG_Delta();

  osw_init();

  seriallog_level = sysCfg.seriallog_level;
  stop_flash_rotate = sysCfg.flag.stop_flash_rotate;
  savedatacounter = sysCfg.savedata;
  sleep = sysCfg.sleep;

  sysCfg.bootcount++;
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_BOOT_COUNT " %d"), sysCfg.bootcount);
  addLog(LOG_LEVEL_DEBUG);

  GPIO_init();

  if (Serial.baudRate() != Baudrate) {
    if (seriallog_level) {
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_SET_BAUDRATE_TO " %d"), Baudrate);
      addLog(LOG_LEVEL_INFO);
    }
    delay(100);
    Serial.flush();
    Serial.begin(Baudrate);
    delay(10);
    Serial.println();
  }

  if (strstr(sysCfg.hostname, "%")) {
    strlcpy(sysCfg.hostname, WIFI_HOSTNAME, sizeof(sysCfg.hostname));
    snprintf_P(Hostname, sizeof(Hostname)-1, sysCfg.hostname, DEFAULT_NAME, ESP.getChipId() & 0x1FFF);
  } else {
    snprintf_P(Hostname, sizeof(Hostname)-1, sysCfg.hostname);
  }

  WIFI_Connect();

  if (4 == sysCfg.poweronstate) {  // Allways on
    setRelay(power);
  } else {
    if ((resetInfo.reason == REASON_DEFAULT_RST) || (resetInfo.reason == REASON_EXT_SYS_RST)) {
      switch (sysCfg.poweronstate) {
      case 0:  // All off
        power = 0;
        setRelay(power);
        break;
      case 1:  // All on
        power = (1 << Maxdevice) -1;
        setRelay(power);
        break;
      case 2:  // All saved state toggle
        power = sysCfg.power & ((1 << Maxdevice) -1) ^ 0xFF;
        if (sysCfg.flag.savestate) {
          setRelay(power);
        }
        break;
      case 3:  // All saved state
        power = sysCfg.power & ((1 << Maxdevice) -1);
        if (sysCfg.flag.savestate) {
          setRelay(power);
        }
        break;
      }
    } else {
      power = sysCfg.power & ((1 << Maxdevice) -1);
      if (sysCfg.flag.savestate) {
        setRelay(power);
      }
    }
  }

  for (byte i = 0; i < Maxdevice; i++) {
    if ((pin[GPIO_REL1 +i] < 99) && (digitalRead(pin[GPIO_REL1 +i]) ^ rel_inverted[i])) {
      bitSet(power, i);
    }
  }

  rtc_init();

  bob.begin();
  bob.setNoDelay(true);
}

void loop()
{
  osw_loop();

  pollBob();

  pollDnsWeb();

  pollUDP();

  if (millis() >= timerxs) {
    stateloop();
  }

  if (Serial.available()){
    serial();
  }

  // yield == delay(0), delay contains yield, auto yield in loop
  delay(sleep);  // https://github.com/esp8266/Arduino/issues/2021
}
