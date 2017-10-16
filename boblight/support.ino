/*
  support.ino - support for Sonoff-Tasmota

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

IPAddress syslog_host_addr;  // Syslog host IP address
unsigned long syslog_host_refresh = 0;

/*********************************************************************************************\
 * Watchdog extension (https://github.com/esp8266/Arduino/issues/1532)
\*********************************************************************************************/

Ticker tickerOSWatch;

#define OSWATCH_RESET_TIME 30

static unsigned long osw_last_loop;
byte osw_flag = 0;

#ifndef USE_WS2812_DMA  // Collides with Neopixelbus but solves exception
void osw_osWatch() ICACHE_RAM_ATTR;
#endif  // USE_WS2812_DMA

void osw_osWatch()
{
  unsigned long t = millis();
  unsigned long last_run = abs(t - osw_last_loop);

#ifdef DEBUG_THEO
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_OSWATCH " FreeRam %d, rssi %d, last_run %d"), ESP.getFreeHeap(), WIFI_getRSSIasQuality(WiFi.RSSI()), last_run);
  addLog(LOG_LEVEL_DEBUG);
#endif  // DEBUG_THEO
  if (last_run >= (OSWATCH_RESET_TIME * 1000)) {
    addLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_OSWATCH " " D_BLOCKED_LOOP ". " D_RESTARTING));
    rtcMem.osw_flag = 1;
    RTC_Save();
    ESP.reset();  // hard reset
  }
}

void osw_init()
{
  osw_flag = rtcMem.osw_flag;
  rtcMem.osw_flag = 0;
  osw_last_loop = millis();
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), osw_osWatch);
}

void osw_loop()
{
  osw_last_loop = millis();
//  while(1) delay(1000);  // this will trigger the os watch
}

String getResetReason()
{
  char buff[32];
  if (osw_flag) {
    strncpy_P(buff, PSTR(D_BLOCKED_LOOP), sizeof(buff));
    return String(buff);
  } else {
    return ESP.getResetReason();
  }
}

#ifdef DEBUG_THEO
void exception_tst(byte type)
{
/*
Exception (28):
epc1=0x4000bf64 epc2=0x00000000 epc3=0x00000000 excvaddr=0x00000007 depc=0x00000000

ctx: cont
sp: 3fff1f30 end: 3fff2840 offset: 01a0

>>>stack>>>
3fff20d0:  202c3573 756f7247 2c302070 646e4920
3fff20e0:  40236a6e 7954202c 45206570 00454358
3fff20f0:  00000010 00000007 00000000 3fff2180
3fff2100:  3fff2190 40107bfc 3fff3e4c 3fff22c0
3fff2110:  40261934 000000f0 3fff22c0 401004d8
3fff2120:  40238fcf 00000050 3fff2100 4021fc10
3fff2130:  3fff32bc 4021680c 3ffeade1 4021ff7d
3fff2140:  3fff2190 3fff2180 0000000c 7fffffff
3fff2150:  00000019 00000000 00000000 3fff21c0
3fff2160:  3fff23f3 3ffe8e08 00000000 4021ffb4
3fff2170:  3fff2190 3fff2180 0000000c 40201118
3fff2180:  3fff21c0 0000003c 3ffef840 00000007
3fff2190:  00000000 00000000 00000000 40201128
3fff21a0:  3fff23f3 000000f1 3fff23ec 4020fafb
3fff21b0:  3fff23f3 3fff21c0 3fff21d0 3fff23f6
3fff21c0:  00000000 3fff23fb 4022321b 00000000

Exception 28: LoadProhibited: A load referenced a page mapped with an attribute that does not permit loads
Decoding 14 results
0x40236a6e: ets_vsnprintf at ?? line ?
0x40107bfc: vsnprintf at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/libc_replacements.c line 387
0x40261934: bignum_exptmod at ?? line ?
0x401004d8: malloc at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266\umm_malloc/umm_malloc.c line 1664
0x40238fcf: wifi_station_get_connect_status at ?? line ?
0x4021fc10: operator new[](unsigned int) at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/abi.cpp line 57
0x4021680c: ESP8266WiFiSTAClass::status() at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WiFi\src/ESP8266WiFiSTA.cpp line 569
0x4021ff7d: vsnprintf_P(char*, unsigned int, char const*, __va_list_tag) at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/pgmspace.cpp line 146
0x4021ffb4: snprintf_P(char*, unsigned int, char const*, ...) at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/pgmspace.cpp line 146
0x40201118: atol at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/core_esp8266_noniso.c line 45
0x40201128: atoi at C:\Data2\Arduino\arduino-1.8.1-esp-2.3.0\portable\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266/core_esp8266_noniso.c line 45
0x4020fafb: mqttDataCb(char*, unsigned char*, unsigned int) at R:\Arduino\Work-ESP8266\Theo\sonoff\sonoff-4\sonoff/sonoff.ino line 679 (discriminator 1)
0x4022321b: pp_attach at ?? line ?

00:00:08 MQTT: tele/sonoff/INFO3 = {"Started":"Fatal exception:28 flag:2 (EXCEPTION) epc1:0x4000bf64 epc2:0x00000000 epc3:0x00000000 excvaddr:0x00000007 depc:0x00000000"}
*/
  if (1 == type) {
    char svalue[10];
    snprintf_P(svalue, sizeof(svalue), PSTR("%s"), 7);  // Exception 28 as number in string (7 in excvaddr)
  }
/*
14:50:52 osWatch: FreeRam 25896, rssi 68, last_run 0
14:51:02 osWatch: FreeRam 25896, rssi 58, last_run 0
14:51:03 CMND: exception 2
14:51:12 osWatch: FreeRam 25360, rssi 60, last_run 8771
14:51:22 osWatch: FreeRam 25360, rssi 62, last_run 18771
14:51:32 osWatch: FreeRam 25360, rssi 62, last_run 28771
14:51:42 osWatch: FreeRam 25360, rssi 62, last_run 38771
14:51:42 osWatch: Warning, loop blocked. Restart now
*/
  if (2 == type) {
    while(1) delay(1000);  // this will trigger the os watch
  }
}
#endif  // DEBUG_THEO

/*********************************************************************************************\
 * General
\*********************************************************************************************/

char* _dtostrf(double number, unsigned char prec, char *s, bool i18n)
{
  bool negative = false;

  if (isnan(number)) {
    strcpy_P(s, PSTR("nan"));
    return s;
  }
  if (isinf(number)) {
    strcpy_P(s, PSTR("inf"));
    return s;
  }
  char decimal = '.';
  if (i18n) {
    decimal = D_DECIMAL_SEPARATOR[0];
  }

  char* out = s;

  // Handle negative numbers
  if (number < 0.0) {
    negative = true;
    number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  // I optimized out most of the divisions
  double rounding = 2.0;
  for (uint8_t i = 0; i < prec; ++i) {
    rounding *= 10.0;
  }
  rounding = 1.0 / rounding;
  number += rounding;

  // Figure out how big our number really is
  double tenpow = 1.0;
  int digitcount = 1;
  while (number >= 10.0 * tenpow) {
    tenpow *= 10.0;
    digitcount++;
  }
  number /= tenpow;

  // Handle negative sign
  if (negative) {
    *out++ = '-';
  }

  // Print the digits, and if necessary, the decimal point
  digitcount += prec;
  int8_t digit = 0;
  while (digitcount-- > 0) {
    digit = (int8_t)number;
    if (digit > 9) {
      digit = 9; // insurance
    }
    *out++ = (char)('0' | digit);
    if ((digitcount == prec) && (prec > 0)) {
      *out++ = decimal;
    }
    number -= digit;
    number *= 10.0;
  }

  // make sure the string is terminated
  *out = 0;
  return s;
}

char* dtostrfd(double number, unsigned char prec, char *s)  // Always decimal dot
{
  return _dtostrf(number, prec, s, 0);
}

char* dtostrfi(double number, unsigned char prec, char *s) // Use localized decimal dot
{
  return _dtostrf(number, prec, s, 1);
}

boolean parseIP(uint32_t* addr, const char* str)
{
  uint8_t *part = (uint8_t*)addr;
  byte i;

  *addr = 0;
  for (i = 0; i < 4; i++) {
    part[i] = strtoul(str, NULL, 10);        // Convert byte
    str = strchr(str, '.');
    if (str == NULL || *str == '\0') {
      break;  // No more separators, exit
    }
    str++;                                   // Point to next character after separator
  }
  return (3 == i);
}

// Function to parse & check if version_str is newer than our currently installed version.
bool newerVersion(char* version_str)
{
  uint32_t version = 0;
  uint8_t i = 0;
  char *str_ptr;
  char* version_dup = strdup(version_str);  // Duplicate the version_str as strtok_r will modify it.

  if (!version_dup) {
    return false;  // Bail if we can't duplicate. Assume bad.
  }
  // Loop through the version string, splitting on '.' seperators.
  for (char *str = strtok_r(version_dup, ".", &str_ptr); str && i < sizeof(VERSION); str = strtok_r(NULL, ".", &str_ptr), i++) {
    int field = atoi(str);
    // The fields in a version string can only range from 0-255.
    if ((field < 0) || (field > 255)) {
      free(version_dup);
      return false;
    }
    // Shuffle the accumulated bytes across, and add the new byte.
    version = (version << 8) + field;
    // Check alpha delimiter after 1.2.3 only
    if ((2 == i) && isalpha(str[strlen(str)-1])) {
      field = str[strlen(str)-1] & 0x1f;
      version = (version << 8) + field;
      i++;
    }
  }
  free(version_dup);  // We no longer need this.
  // A version string should have 2-4 fields. e.g. 1.2, 1.2.3, or 1.2.3a (= 1.2.3.1).
  // If not, then don't consider it a valid version string.
  if ((i < 2) || (i > sizeof(VERSION))) {
    return false;
  }
  // Keep shifting the parsed version until we hit the maximum number of tokens.
  // VERSION stores the major number of the version in the most significant byte of the uint32_t.
  while (i < sizeof(VERSION)) {
    version <<= 8;
    i++;
  }
  // Now we should have a fully constructed version number in uint32_t form.
  return (version > VERSION);
}

char* getPowerDevice(char* dest, uint8_t idx, size_t size, uint8_t option)
{
  char sidx[8];

  strncpy_P(dest, S_RSLT_POWER, size);
  if ((Maxdevice + option) > 1) {
    snprintf_P(sidx, sizeof(sidx), PSTR("%d"), idx);
    strncat(dest, sidx, size);
  }
  return dest;
}

char* getPowerDevice(char* dest, uint8_t idx, size_t size)
{
  return getPowerDevice(dest, idx, size, 0);
}

/*********************************************************************************************\
 * Wifi
\*********************************************************************************************/

#define WIFI_CONFIG_SEC   180  // seconds before restart
#define WIFI_MANAGER_SEC  180  // seconds before restart
#define WIFI_CHECK_SEC    20   // seconds
#define WIFI_RETRY_SEC    30   // seconds

uint8_t _wificounter;
uint8_t _wifiretry;
uint8_t _wifistatus;
uint8_t _wpsresult;
uint8_t _wificonfigflag = 0;
uint8_t _wifiConfigCounter = 0;

int WIFI_getRSSIasQuality(int RSSI)
{
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

boolean WIFI_configCounter()
{
  if (_wifiConfigCounter) {
    _wifiConfigCounter = WIFI_MANAGER_SEC;
  }
  return (_wifiConfigCounter);
}

extern "C" {
#include "user_interface.h"
}

void WIFI_wps_status_cb(wps_cb_status status);

void WIFI_wps_status_cb(wps_cb_status status)
{
/* from user_interface.h:
  enum wps_cb_status {
    WPS_CB_ST_SUCCESS = 0,
    WPS_CB_ST_FAILED,
    WPS_CB_ST_TIMEOUT,
    WPS_CB_ST_WEP,      // WPS failed because that WEP is not supported
    WPS_CB_ST_SCAN_ERR, // can not find the target WPS AP
  };
*/
  _wpsresult = status;
  if (WPS_CB_ST_SUCCESS == _wpsresult) {
    wifi_wps_disable();
  } else {
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_WPS_FAILED_WITH_STATUS " %d"), _wpsresult);
    addLog(LOG_LEVEL_DEBUG);
    _wifiConfigCounter = 2;
  }
}

boolean WIFI_WPSConfigDone(void)
{
  return (!_wpsresult);
}

boolean WIFI_beginWPSConfig(void)
{
  _wpsresult = 99;
  if (!wifi_wps_disable()) {
    return false;
  }
  if (!wifi_wps_enable(WPS_TYPE_PBC)) {
    return false;  // so far only WPS_TYPE_PBC is supported (SDK 2.0.0)
  }
  if (!wifi_set_wps_cb((wps_st_cb_t) &WIFI_wps_status_cb)) {
    return false;
  }
  if (!wifi_wps_start()) {
    return false;
  }
  return true;
}

void WIFI_config(uint8_t type)
{
  if (!_wificonfigflag) {
    if (type >= WIFI_RETRY) {  // WIFI_RETRY and WIFI_WAIT
      return;
    }
    UDP_Disconnect();
    WiFi.disconnect();        // Solve possible Wifi hangs
    _wificonfigflag = type;
    _wifiConfigCounter = WIFI_CONFIG_SEC;   // Allow up to WIFI_CONFIG_SECS seconds for phone to provide ssid/pswd
    _wificounter = _wifiConfigCounter +5;
    blinks = 1999;
    if (WIFI_RESTART == _wificonfigflag) {
      restartflag = 2;
    }
    else if (WIFI_SMARTCONFIG == _wificonfigflag) {
      addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_1_SMARTCONFIG D_ACTIVE_FOR_1_MINUTE));
      WiFi.beginSmartConfig();
    }
    else if (WIFI_WPSCONFIG == _wificonfigflag) {
      if (WIFI_beginWPSConfig()) {
        addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_3_WPSCONFIG D_ACTIVE_FOR_1_MINUTE));
      } else {
        addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_3_WPSCONFIG D_FAILED_TO_START));
        _wifiConfigCounter = 3;
      }
    }
    else if (WIFI_MANAGER == _wificonfigflag) {
      addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_WCFG_2_WIFIMANAGER D_ACTIVE_FOR_1_MINUTE));
      beginWifiManager();
    }
  }
}

void WIFI_begin(uint8_t flag)
{
  const char PhyMode[] = " BGN";

  UDP_Disconnect();
  if (!strncmp_P(ESP.getSdkVersion(),PSTR("1.5.3"),5)) {
    addLog_P(LOG_LEVEL_DEBUG, S_LOG_WIFI, PSTR(D_PATCH_ISSUE_2186));
    WiFi.mode(WIFI_OFF);    // See https://github.com/esp8266/Arduino/issues/2186
  }
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);      // Disable AP mode
  if (sysCfg.sleep) {
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);  // Allow light sleep during idle times
  }
  if (!WiFi.getAutoConnect()) {
    WiFi.setAutoConnect(true);
  }
  switch (flag) {
  case 0:  // AP1
  case 1:  // AP2
    sysCfg.sta_active = flag;
    break;
  case 2:  // Toggle
    sysCfg.sta_active ^= 1;
  }        // 3: Current AP
  if (0 == strlen(sysCfg.sta_ssid[1])) {
    sysCfg.sta_active = 0;
  }
  if (sysCfg.ip_address[0]) {
    WiFi.config(sysCfg.ip_address[0], sysCfg.ip_address[1], sysCfg.ip_address[2], sysCfg.ip_address[3]);  // Set static IP
  }
  WiFi.hostname(Hostname);
  WiFi.begin(sysCfg.sta_ssid[sysCfg.sta_active], sysCfg.sta_pwd[sysCfg.sta_active]);
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_CONNECTING_TO_AP "%d %s " D_IN_MODE " 11%c " D_AS " %s..."),
    sysCfg.sta_active +1, sysCfg.sta_ssid[sysCfg.sta_active], PhyMode[WiFi.getPhyMode() & 0x3], Hostname);
  addLog(LOG_LEVEL_INFO);
}

void WIFI_check_ip()
{
  if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    _wificounter = WIFI_CHECK_SEC;
    _wifiretry = WIFI_RETRY_SEC;
    addLog_P((_wifistatus != WL_CONNECTED) ? LOG_LEVEL_INFO : LOG_LEVEL_DEBUG_MORE, S_LOG_WIFI, PSTR(D_CONNECTED));
    if (_wifistatus != WL_CONNECTED) {
      sysCfg.ip_address[1] = (uint32_t)WiFi.gatewayIP();
      sysCfg.ip_address[2] = (uint32_t)WiFi.subnetMask();
      sysCfg.ip_address[3] = (uint32_t)WiFi.dnsIP();
    }
    _wifistatus = WL_CONNECTED;
  } else {
    _wifistatus = WiFi.status();
    switch (_wifistatus) {
      case WL_CONNECTED:
        addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_NO_IP_ADDRESS));
        _wifistatus = 0;
        _wifiretry = WIFI_RETRY_SEC;
        break;
      case WL_NO_SSID_AVAIL:
        addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_AP_NOT_REACHED));
        if (WIFI_WAIT == sysCfg.sta_config) {
          _wifiretry = WIFI_RETRY_SEC;
        } else {
          if (_wifiretry > (WIFI_RETRY_SEC / 2)) {
            _wifiretry = WIFI_RETRY_SEC / 2;
          }
          else if (_wifiretry) {
            _wifiretry = 0;
          }
        }
        break;
      case WL_CONNECT_FAILED:
        addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_WRONG_PASSWORD));
        if (_wifiretry > (WIFI_RETRY_SEC / 2)) {
          _wifiretry = WIFI_RETRY_SEC / 2;
        }
        else if (_wifiretry) {
          _wifiretry = 0;
        }
        break;
      default:  // WL_IDLE_STATUS and WL_DISCONNECTED
        if (!_wifiretry || ((WIFI_RETRY_SEC / 2) == _wifiretry)) {
          addLog_P(LOG_LEVEL_INFO, S_LOG_WIFI, PSTR(D_CONNECT_FAILED_AP_TIMEOUT));
        } else {
          addLog_P(LOG_LEVEL_DEBUG, S_LOG_WIFI, PSTR(D_ATTEMPTING_CONNECTION));
        }
    }
    if (_wifiretry) {
      if (WIFI_RETRY_SEC == _wifiretry) {
        WIFI_begin(3);  // Select default SSID
      }
      if ((sysCfg.sta_config != WIFI_WAIT) && ((WIFI_RETRY_SEC / 2) == _wifiretry)) {
        WIFI_begin(2);  // Select alternate SSID
      }
      _wificounter = 1;
      _wifiretry--;
    } else {
      WIFI_config(sysCfg.sta_config);
      _wificounter = 1;
      _wifiretry = WIFI_RETRY_SEC;
    }
  }
}

void WIFI_Check(uint8_t param)
{
  _wificounter--;
  switch (param) {
  case WIFI_SMARTCONFIG:
  case WIFI_MANAGER:
  case WIFI_WPSCONFIG:
    WIFI_config(param);
    break;
  default:
    if (_wifiConfigCounter) {
      _wifiConfigCounter--;
      _wificounter = _wifiConfigCounter +5;
      if (_wifiConfigCounter) {
        if ((WIFI_SMARTCONFIG == _wificonfigflag) && WiFi.smartConfigDone()) {
          _wifiConfigCounter = 0;
        }
        if ((WIFI_WPSCONFIG == _wificonfigflag) && WIFI_WPSConfigDone()) {
          _wifiConfigCounter = 0;
        }
        if (!_wifiConfigCounter) {
          if (strlen(WiFi.SSID().c_str())) {
            strlcpy(sysCfg.sta_ssid[0], WiFi.SSID().c_str(), sizeof(sysCfg.sta_ssid[0]));
          }
          if (strlen(WiFi.psk().c_str())) {
            strlcpy(sysCfg.sta_pwd[0], WiFi.psk().c_str(), sizeof(sysCfg.sta_pwd[0]));
          }
          sysCfg.sta_active = 0;
          snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_WIFI D_WCFG_1_SMARTCONFIG D_CMND_SSID "1 %s, " D_CMND_PASSWORD "1 %s"), sysCfg.sta_ssid[0], sysCfg.sta_pwd[0]);
          addLog(LOG_LEVEL_INFO);
        }
      }
      if (!_wifiConfigCounter) {
        if (WIFI_SMARTCONFIG == _wificonfigflag) {
          WiFi.stopSmartConfig();
        }
        restartflag = 2;
      }
    } else {
      if (_wificounter <= 0) {
        addLog_P(LOG_LEVEL_DEBUG_MORE, S_LOG_WIFI, PSTR(D_CHECKING_CONNECTION));
        _wificounter = WIFI_CHECK_SEC;
        WIFI_check_ip();
      }
      if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0) && !_wificonfigflag) {
        if (sysCfg.webserver) {
          startWebserver(sysCfg.webserver, WiFi.localIP());
        } else {
          stopWebserver();
        }
        UDP_Connect();
      }
    }
  }
}

int WIFI_State()
{
  int state;

  if ((WL_CONNECTED == WiFi.status()) && (static_cast<uint32_t>(WiFi.localIP()) != 0)) {
    state = WIFI_RESTART;
  }
  if (_wificonfigflag) {
    state = _wificonfigflag;
  }
  return state;
}

void WIFI_Connect()
{
  WiFi.persistent(false);   // Solve possible wifi init errors
  _wifistatus = 0;
  _wifiretry = WIFI_RETRY_SEC;
  _wificounter = 1;
}

/*********************************************************************************************\
 * Real Time Clock
 *
 * Sources: Time by Michael Margolis and Paul Stoffregen (https://github.com/PaulStoffregen/Time)
 *          Timezone by Jack Christensen (https://github.com/JChristensen/Timezone)
\*********************************************************************************************/

extern "C" {
#include "sntp.h"
}

#define SECS_PER_MIN  ((uint32_t)(60UL))
#define SECS_PER_HOUR ((uint32_t)(3600UL))
#define SECS_PER_DAY  ((uint32_t)(SECS_PER_HOUR * 24UL))
#define LEAP_YEAR(Y)  (((1970+Y)>0) && !((1970+Y)%4) && (((1970+Y)%100) || !((1970+Y)%400)))

Ticker tickerRTC;

static const uint8_t monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // API starts months from 1, this array starts from 0

uint32_t utctime = 0;
uint32_t loctime = 0;
uint32_t dsttime = 0;
uint32_t stdtime = 0;
uint32_t ntptime = 0;
uint32_t midnight = 1451602800;
uint8_t  midnightnow = 0;

String getBuildDateTime()
{
  // "2017-03-07T11:08:02" - ISO8601:2004
  char bdt[21];
  char *str;
  char *p;
  char *smonth;
  char mdate[] = __DATE__;  // "Mar  7 2017"
  int month;
  int day;
  int year;

  byte i = 0;
  for (str = strtok_r(mdate, " ", &p); str && i < 3; str = strtok_r(NULL, " ", &p)) {
    switch (i++) {
    case 0:  // Month
      smonth = str;
      break;
    case 1:  // Day
      day = atoi(str);
      break;
    case 2:  // Year
      year = atoi(str);
    }
  }
  month = (strstr(monthNames, smonth) -monthNames) /3 +1;
  snprintf_P(bdt, sizeof(bdt), PSTR("%d" D_YEAR_MONTH_SEPARATOR "%02d" D_MONTH_DAY_SEPARATOR "%02d" D_DATE_TIME_SEPARATOR "%s"), year, month, day, __TIME__);
  return String(bdt);
}

String getDateTime()
{
  // "2017-03-07T11:08:02" - ISO8601:2004
  char dt[21];

  snprintf_P(dt, sizeof(dt), PSTR("%04d" D_YEAR_MONTH_SEPARATOR "%02d" D_MONTH_DAY_SEPARATOR "%02d" D_DATE_TIME_SEPARATOR "%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"),
    rtcTime.Year, rtcTime.Month, rtcTime.Day, rtcTime.Hour, rtcTime.Minute, rtcTime.Second);
  return String(dt);
}

String getUTCDateTime()
{
  // "2017-03-07T11:08:02" - ISO8601:2004
  char dt[21];

  TIME_T tmpTime;
  breakTime(utctime, tmpTime);
  tmpTime.Year += 1970;

  snprintf_P(dt, sizeof(dt), PSTR("%04d" D_YEAR_MONTH_SEPARATOR "%02d" D_MONTH_DAY_SEPARATOR "%02d" D_DATE_TIME_SEPARATOR "%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"),
    tmpTime.Year, tmpTime.Month, tmpTime.Day, tmpTime.Hour, tmpTime.Minute, tmpTime.Second);
  return String(dt);
}

void breakTime(uint32_t timeInput, TIME_T &tm)
{
// break the given timeInput into time components
// this is a more compact version of the C library localtime function
// note that year is offset from 1970 !!!

  uint8_t year;
  uint8_t month;
  uint8_t monthLength;
  uint32_t time;
  unsigned long days;

  time = timeInput;
  tm.Second = time % 60;
  time /= 60;                // now it is minutes
  tm.Minute = time % 60;
  time /= 60;                // now it is hours
  tm.Hour = time % 24;
  time /= 24;                // now it is days
  tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.Year = year;            // year is offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time -= days;              // now it is days in this year, starting at 0
  tm.DayOfYear = time;

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (1 == month) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  strlcpy(tm.MonthName, monthNames + (month *3), 4);
  tm.Month = month + 1;      // jan is month 1
  tm.Day = time + 1;         // day of month
  tm.Valid = (timeInput > 1451602800);  // 2016-01-01
}

uint32_t makeTime(TIME_T &tm)
{
// assemble time elements into time_t
// note year argument is offset from 1970

  int i;
  uint32_t seconds;

  // seconds from 1970 till 1 jan 00:00:00 of the given year
  seconds = tm.Year * (SECS_PER_DAY * 365);
  for (i = 0; i < tm.Year; i++) {
    if (LEAP_YEAR(i)) {
      seconds +=  SECS_PER_DAY;   // add extra days for leap years
    }
  }

  // add days for this year, months start from 1
  for (i = 1; i < tm.Month; i++) {
    if ((2 == i) && LEAP_YEAR(tm.Year)) {
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * monthDays[i-1];  // monthDay array starts from 0
    }
  }
  seconds+= (tm.Day - 1) * SECS_PER_DAY;
  seconds+= tm.Hour * SECS_PER_HOUR;
  seconds+= tm.Minute * SECS_PER_MIN;
  seconds+= tm.Second;
  return seconds;
}

uint32_t toTime_t(TimeChangeRule r, int yr)
{
    TIME_T tm;
    uint32_t t;
    uint8_t m;
    uint8_t w;            // temp copies of r.month and r.week

    m = r.month;
    w = r.week;
    if (0 == w) {         // Last week = 0
      if (++m > 12) {     // for "Last", go to the next month
        m = 1;
        yr++;
      }
      w = 1;              // and treat as first week of next month, subtract 7 days later
    }

    tm.Hour = r.hour;
    tm.Minute = 0;
    tm.Second = 0;
    tm.Day = 1;
    tm.Month = m;
    tm.Year = yr - 1970;
    t = makeTime(tm);        // First day of the month, or first day of next month for "Last" rules
    breakTime(t, tm);
    t += (7 * (w - 1) + (r.dow - tm.Wday + 7) % 7) * SECS_PER_DAY;
    if (0 == r.week) {
      t -= 7 * SECS_PER_DAY;    //back up a week if this is a "Last" rule
    }
    return t;
}

String rtc_time(int type)
{
  char stime[25];   // Skip newline

  uint32_t time = utctime;
  if (1 == type) time = loctime;
  if (2 == type) time = dsttime;
  if (3 == type) time = stdtime;
  snprintf_P(stime, sizeof(stime), sntp_get_real_time(time));
  return String(stime);
}

uint32_t rtc_loctime()
{
  return loctime;
}

uint32_t rtc_midnight()
{
  return midnight;
}

boolean rtc_midnight_now()
{
  boolean mnflg = midnightnow;
  if (mnflg) {
    midnightnow = 0;
  }
  return mnflg;
}

void rtc_second()
{
  byte ntpsync;
  uint32_t stdoffset;
  uint32_t dstoffset;
  TIME_T tmpTime;

  ntpsync = 0;
  if (rtcTime.Year < 2016) {
    if (WL_CONNECTED == WiFi.status()) {
      ntpsync = 1;  // Initial NTP sync
    }
  } else {
    if ((1 == rtcTime.Minute) && (1 == rtcTime.Second)) {
      ntpsync = 1;  // Hourly NTP sync at xx:01:01
    }
  }
  if (ntpsync) {
    ntptime = sntp_get_current_timestamp();
    if (ntptime) {
      utctime = ntptime;
      breakTime(utctime, tmpTime);
      rtcTime.Year = tmpTime.Year + 1970;
      dsttime = toTime_t(myDST, rtcTime.Year);
      stdtime = toTime_t(mySTD, rtcTime.Year);
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "(" D_UTC_TIME ") %s"), rtc_time(0).c_str());
      addLog(LOG_LEVEL_DEBUG);
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "(" D_DST_TIME ") %s"), rtc_time(2).c_str());
      addLog(LOG_LEVEL_DEBUG);
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "(" D_STD_TIME ") %s"), rtc_time(3).c_str());
      addLog(LOG_LEVEL_DEBUG);
    }
  }
  utctime++;
  loctime = utctime;
  if (loctime > 1451602800) {  // 2016-01-01
    if (99 == sysCfg.timezone) {
      if (myDST.hemis) {
        dstoffset = mySTD.offset * SECS_PER_MIN;  // Southern hemisphere
        stdoffset = myDST.offset * SECS_PER_MIN;
      } else {
        dstoffset = myDST.offset * SECS_PER_MIN;  // Northern hemisphere
        stdoffset = mySTD.offset * SECS_PER_MIN;
      }
      if ((utctime >= (dsttime - stdoffset)) && (utctime < (stdtime - dstoffset))) {
        loctime += dstoffset;  // Daylight Saving Time
      } else {
        loctime += stdoffset;  // Standard Time
      }
    } else {
      loctime += sysCfg.timezone * SECS_PER_HOUR;
    }
  }
  breakTime(loctime, rtcTime);
  if (!rtcTime.Hour && !rtcTime.Minute && !rtcTime.Second && rtcTime.Valid) {
    midnight = loctime;
    midnightnow = 1;
  }
  rtcTime.Year += 1970;
}

void rtc_init()
{
  sntp_setservername(0, sysCfg.ntp_server[0]);
  sntp_setservername(1, sysCfg.ntp_server[1]);
  sntp_setservername(2, sysCfg.ntp_server[2]);
  sntp_stop();
  sntp_set_timezone(0);      // UTC time
  sntp_init();
  utctime = 0;
  breakTime(utctime, rtcTime);
  tickerRTC.attach(1, rtc_second);
}

/*********************************************************************************************\
 * Logging
\*********************************************************************************************/

void addLog(byte loglevel)
{
  char mxtime[9];  // 13:45:21

  snprintf_P(mxtime, sizeof(mxtime), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), rtcTime.Hour, rtcTime.Minute, rtcTime.Second);

  if (loglevel <= seriallog_level) {
    Serial.printf("%s %s\n", mxtime, log_data);
  }
  if (sysCfg.webserver && (loglevel <= sysCfg.weblog_level)) {
    Log[logidx] = String(mxtime) + " " + String(log_data);
    logidx++;
    if (logidx > MAX_LOG_LINES -1) {
      logidx = 0;
    }
  }
}

void addLog_P(byte loglevel, const char *formatP)
{
  snprintf_P(log_data, sizeof(log_data), formatP);
  addLog(loglevel);
}

void addLog_P(byte loglevel, const char *formatP, const char *formatP2)
{
  char message[100];

  snprintf_P(log_data, sizeof(log_data), formatP);
  snprintf_P(message, sizeof(message), formatP2);
  strncat(log_data, message, sizeof(log_data));
  addLog(loglevel);
}

/*********************************************************************************************\
 *
\*********************************************************************************************/
