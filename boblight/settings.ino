/*
  settings.ino - user settings for Sonoff-Tasmota

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
 * RTC memory
\*********************************************************************************************/

#define RTC_MEM_VALID 0xA55A

uint32_t _rtcHash = 0;

uint32_t getRtcHash()
{
  uint32_t hash = 0;
  uint8_t *bytes = (uint8_t*)&rtcMem;

  for (uint16_t i = 0; i < sizeof(RTCMEM); i++) {
    hash += bytes[i]*(i+1);
  }
  return hash;
}

void RTC_Save()
{
  if (getRtcHash() != _rtcHash) {
    rtcMem.valid = RTC_MEM_VALID;
    ESP.rtcUserMemoryWrite(100, (uint32_t*)&rtcMem, sizeof(RTCMEM));
    _rtcHash = getRtcHash();
#ifdef DEBUG_THEO
    addLog_P(LOG_LEVEL_DEBUG, PSTR("Dump: Save"));
    RTC_Dump();
#endif  // DEBUG_THEO
  }
}

void RTC_Load()
{
  ESP.rtcUserMemoryRead(100, (uint32_t*)&rtcMem, sizeof(RTCMEM));
#ifdef DEBUG_THEO
  addLog_P(LOG_LEVEL_DEBUG, PSTR("Dump: Load"));
  RTC_Dump();
#endif  // DEBUG_THEO
  if (rtcMem.valid != RTC_MEM_VALID) {
    memset(&rtcMem, 0x00, sizeof(RTCMEM));
    rtcMem.valid = RTC_MEM_VALID;
    rtcMem.power = sysCfg.power;
    RTC_Save();
  }
  _rtcHash = getRtcHash();
}

boolean RTC_Valid()
{
  return (RTC_MEM_VALID == rtcMem.valid);
}

#ifdef DEBUG_THEO
void RTC_Dump()
{
  #define CFG_COLS 16

  uint16_t idx;
  uint16_t maxrow;
  uint16_t row;
  uint16_t col;

  uint8_t *buffer = (uint8_t *) &rtcMem;
  maxrow = ((sizeof(RTCMEM)+CFG_COLS)/CFG_COLS);

  for (row = 0; row < maxrow; row++) {
    idx = row * CFG_COLS;
    snprintf_P(log_data, sizeof(log_data), PSTR("%04X:"), idx);
    for (col = 0; col < CFG_COLS; col++) {
      if (!(col%4)) {
        snprintf_P(log_data, sizeof(log_data), PSTR("%s "), log_data);
      }
      snprintf_P(log_data, sizeof(log_data), PSTR("%s %02X"), log_data, buffer[idx + col]);
    }
    snprintf_P(log_data, sizeof(log_data), PSTR("%s |"), log_data);
    for (col = 0; col < CFG_COLS; col++) {
//      if (!(col%4)) {
//        snprintf_P(log_data, sizeof(log_data), PSTR("%s "), log_data);
//      }
      snprintf_P(log_data, sizeof(log_data), PSTR("%s%c"), log_data, ((buffer[idx + col] > 0x20) && (buffer[idx + col] < 0x7F)) ? (char)buffer[idx + col] : ' ');
    }
    snprintf_P(log_data, sizeof(log_data), PSTR("%s|"), log_data);
    addLog(LOG_LEVEL_INFO);
  }
}
#endif  // DEBUG_THEO

/*********************************************************************************************\
 * Config - Flash
\*********************************************************************************************/

extern "C" {
#include "spi_flash.h"
}
#include "eboot_command.h"

extern "C" uint32_t _SPIFFS_end;

#define SPIFFS_END          ((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE

// Version 3.x config
// #define CFG_LOCATION_3      SPIFFS_END - 4

#define CFG_LOCATION        SPIFFS_END  // No need for SPIFFS as it uses EEPROM area
#define CFG_ROTATES         8           // Number of flash sectors used (handles uploads)

uint32_t _cfgHash = 0;
uint32_t _cfgLocation = CFG_LOCATION;

/********************************************************************************************/
/*
 * Based on cores/esp8266/Updater.cpp
 */
void setFlashModeDout()
{
  uint8_t *_buffer;
  uint32_t address;

  eboot_command ebcmd;
  eboot_command_read(&ebcmd);
  address = ebcmd.args[0];
  _buffer = new uint8_t[FLASH_SECTOR_SIZE];
  if (SPI_FLASH_RESULT_OK == spi_flash_read(address, (uint32_t*)_buffer, FLASH_SECTOR_SIZE)) {
    if (_buffer[2] != 3) {  // DOUT
      _buffer[2] = 3;
      noInterrupts();
      if (SPI_FLASH_RESULT_OK == spi_flash_erase_sector(address / FLASH_SECTOR_SIZE)) {
        spi_flash_write(address, (uint32_t*)_buffer, FLASH_SECTOR_SIZE);
      }
      interrupts();
    }
  }
  delete[] _buffer;
}

uint32_t getHash()
{
  uint32_t hash = 0;
  uint8_t *bytes = (uint8_t*)&sysCfg;

  for (uint16_t i = 0; i < sizeof(SYSCFG); i++) {
    hash += bytes[i]*(i+1);
  }
  return hash;
}

/*********************************************************************************************\
 * Config Save - Save parameters to Flash ONLY if any parameter has changed
\*********************************************************************************************/

uint32_t CFG_Address()
{
  return _cfgLocation * SPI_FLASH_SEC_SIZE;
}

void CFG_Save(byte rotate)
{
/* Save configuration in eeprom or one of 7 slots below
 *
 * rotate 0 = Save in next flash slot
 * rotate 1 = Save only in eeprom flash slot until SetOption12 0 or restart
 * rotate 2 = Save in eeprom flash slot, erase next flash slots and continue depending on stop_flash_rotate
 * stop_flash_rotate 0 = Allow flash slot rotation (SetOption12 0)
 * stop_flash_rotate 1 = Allow only eeprom flash slot use (SetOption12 1)
 */
  if ((getHash() != _cfgHash) || rotate) {
    if (1 == rotate) {   // Use eeprom flash slot only and disable flash rotate from now on (upgrade)
      stop_flash_rotate = 1;
    }
    if (2 == rotate) {   // Use eeprom flash slot and erase next flash slots if stop_flash_rotate is off (default)
      _cfgLocation = CFG_LOCATION +1;
    }
    if (stop_flash_rotate) {
      _cfgLocation = CFG_LOCATION;
    } else {
      _cfgLocation--;
      if (_cfgLocation <= (CFG_LOCATION - CFG_ROTATES)) {
        _cfgLocation = CFG_LOCATION;
      }
    }
    sysCfg.saveFlag++;
    noInterrupts();
    spi_flash_erase_sector(_cfgLocation);
    spi_flash_write(_cfgLocation * SPI_FLASH_SEC_SIZE, (uint32*)&sysCfg, sizeof(SYSCFG));
    interrupts();
    if (!stop_flash_rotate && rotate) {
      for (byte i = 1; i < CFG_ROTATES; i++) {
        noInterrupts();
        spi_flash_erase_sector(_cfgLocation -i);  // Delete previous configurations by resetting to 0xFF
        interrupts();
        delay(1);
      }
    }
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_CONFIG D_SAVED_TO_FLASH_AT " %X, " D_COUNT " %d, " D_BYTES " %d"),
       _cfgLocation, sysCfg.saveFlag, sizeof(SYSCFG));
    addLog(LOG_LEVEL_DEBUG);
    _cfgHash = getHash();
  }
  RTC_Save();
}

void CFG_Load()
{
/* Load configuration from eeprom or one of 7 slots below if first load does not stop_flash_rotate
 */
  struct SYSCFGH {
    unsigned long cfg_holder;
    unsigned long saveFlag;
  } _sysCfgH;

  _cfgLocation = CFG_LOCATION +1;
  for (byte i = 0; i < CFG_ROTATES; i++) {
    _cfgLocation--;
    noInterrupts();
    spi_flash_read(_cfgLocation * SPI_FLASH_SEC_SIZE, (uint32*)&sysCfg, sizeof(SYSCFG));
    spi_flash_read((_cfgLocation -1) * SPI_FLASH_SEC_SIZE, (uint32*)&_sysCfgH, sizeof(SYSCFGH));
    interrupts();

  snprintf_P(log_data, sizeof(log_data), PSTR("Cnfg: Check at %X with count %d and holder %X"), _cfgLocation -1, _sysCfgH.saveFlag, _sysCfgH.cfg_holder);
  addLog(LOG_LEVEL_DEBUG);

    if ((sysCfg.flag.stop_flash_rotate) || (sysCfg.cfg_holder != _sysCfgH.cfg_holder) || (sysCfg.saveFlag > _sysCfgH.saveFlag)) {
      break;
    }
    delay(1);
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_CONFIG D_LOADED_FROM_FLASH_AT " %X, " D_COUNT " %d"),
    _cfgLocation, sysCfg.saveFlag);
  addLog(LOG_LEVEL_DEBUG);

  _cfgHash = getHash();

  RTC_Load();
}

void CFG_Erase()
{
  SpiFlashOpResult result;

  uint32_t _sectorStart = (ESP.getSketchSize() / SPI_FLASH_SEC_SIZE) + 1;
  uint32_t _sectorEnd = ESP.getFlashChipRealSize() / SPI_FLASH_SEC_SIZE;
  boolean _serialoutput = (LOG_LEVEL_DEBUG_MORE <= seriallog_level);

  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION D_ERASE " %d " D_UNIT_SECTORS), _sectorEnd - _sectorStart);
  addLog(LOG_LEVEL_DEBUG);

  for (uint32_t _sector = _sectorStart; _sector < _sectorEnd; _sector++) {
    noInterrupts();
    result = spi_flash_erase_sector(_sector);
    interrupts();
    if (_serialoutput) {
      Serial.print(F(D_LOG_APPLICATION D_ERASED_SECTOR " "));
      Serial.print(_sector);
      if (SPI_FLASH_RESULT_OK == result) {
        Serial.println(F(" " D_OK));
      } else {
        Serial.println(F(" " D_ERROR));
      }
      delay(10);
    }
  }
}

void CFG_Dump(char* parms)
{
  #define CFG_COLS 16

  uint16_t idx;
  uint16_t maxrow;
  uint16_t row;
  uint16_t col;
  char *p;

  uint8_t *buffer = (uint8_t *) &sysCfg;
  maxrow = ((sizeof(SYSCFG)+CFG_COLS)/CFG_COLS);

  uint16_t srow = strtol(parms, &p, 16) / CFG_COLS;
  uint16_t mrow = strtol(p, &p, 10);

//  snprintf_P(log_data, sizeof(log_data), PSTR("Cnfg: Parms %s, Start row %d, rows %d"), parms, srow, mrow);
//  addLog(LOG_LEVEL_DEBUG);

  if (0 == mrow) {  // Default only 8 lines
    mrow = 8;
  }
  if (srow > maxrow) {
    srow = maxrow - mrow;
  }
  if (mrow < (maxrow - srow)) {
    maxrow = srow + mrow;
  }

  for (row = srow; row < maxrow; row++) {
    idx = row * CFG_COLS;
    snprintf_P(log_data, sizeof(log_data), PSTR("%04X:"), idx);
    for (col = 0; col < CFG_COLS; col++) {
      if (!(col%4)) {
        snprintf_P(log_data, sizeof(log_data), PSTR("%s "), log_data);
      }
      snprintf_P(log_data, sizeof(log_data), PSTR("%s %02X"), log_data, buffer[idx + col]);
    }
    snprintf_P(log_data, sizeof(log_data), PSTR("%s |"), log_data);
    for (col = 0; col < CFG_COLS; col++) {
//      if (!(col%4)) {
//        snprintf_P(log_data, sizeof(log_data), PSTR("%s "), log_data);
//      }
      snprintf_P(log_data, sizeof(log_data), PSTR("%s%c"), log_data, ((buffer[idx + col] > 0x20) && (buffer[idx + col] < 0x7F)) ? (char)buffer[idx + col] : ' ');
    }
    snprintf_P(log_data, sizeof(log_data), PSTR("%s|"), log_data);
    addLog(LOG_LEVEL_INFO);
    delay(1);
  }
}

/********************************************************************************************/

void CFG_Default()
{
  addLog_P(LOG_LEVEL_NONE, PSTR(D_LOG_CONFIG D_USE_DEFAULTS));
  CFG_DefaultSet1();
  CFG_DefaultSet2();
  CFG_Save(2);
}

void CFG_DefaultSet1()
{
  memset(&sysCfg, 0x00, sizeof(SYSCFG));

  sysCfg.cfg_holder = CFG_HOLDER;
  sysCfg.version = VERSION;
}

void CFG_DefaultSet2()
{
  memset((char*)&sysCfg +16, 0x00, sizeof(SYSCFG) -16);

  sysCfg.flag.savestate = SAVE_STATE;
  sysCfg.savedata = SAVE_DATA;
  sysCfg.timezone = APP_TIMEZONE;
  strlcpy(sysCfg.otaUrl, OTA_URL, sizeof(sysCfg.otaUrl));
  sysCfg.seriallog_level = SERIAL_LOG_LEVEL;
  strlcpy(sysCfg.sta_ssid[0], STA_SSID1, sizeof(sysCfg.sta_ssid[0]));
  strlcpy(sysCfg.sta_pwd[0], STA_PASS1, sizeof(sysCfg.sta_pwd[0]));
  strlcpy(sysCfg.sta_ssid[1], STA_SSID2, sizeof(sysCfg.sta_ssid[1]));
  strlcpy(sysCfg.sta_pwd[1], STA_PASS2, sizeof(sysCfg.sta_pwd[1]));
  strlcpy(sysCfg.hostname, WIFI_HOSTNAME, sizeof(sysCfg.hostname));
  sysCfg.sta_config = WIFI_CONFIG_TOOL;
  sysCfg.webserver = WEB_SERVER;
  sysCfg.weblog_level = WEB_LOG_LEVEL;  
  sysCfg.power = APP_POWER;
  sysCfg.poweronstate = APP_POWERON_STATE;
  sysCfg.ledstate = APP_LEDSTATE;
  sysCfg.sleep = APP_SLEEP;
  sysCfg.ws_pixels = WS2812_LEDS;
  sysCfg.ws_red = 255;
  sysCfg.ws_green = 0;
  sysCfg.ws_blue = 0;
  sysCfg.ws_ledtable = 0;
  sysCfg.ws_dimmer = 8;
  sysCfg.ws_fade = 0;
  sysCfg.ws_speed = 1;
  sysCfg.ws_scheme = 0;
  sysCfg.ws_width = 1;
  sysCfg.ws_wakeup = 0;
  strlcpy(sysCfg.friendlyname[0], FRIENDLY_NAME, sizeof(sysCfg.friendlyname[0]));
  strlcpy(sysCfg.friendlyname[1], FRIENDLY_NAME"2", sizeof(sysCfg.friendlyname[1]));
  strlcpy(sysCfg.friendlyname[2], FRIENDLY_NAME"3", sizeof(sysCfg.friendlyname[2]));
  strlcpy(sysCfg.friendlyname[3], FRIENDLY_NAME"4", sizeof(sysCfg.friendlyname[3]));
  sysCfg.module = MODULE;
  for (byte i = 0; i < MAX_GPIO_PIN; i++){
    sysCfg.my_module.gp.io[i] = 0;
  }
  sysCfg.led_pixels = WS2812_LEDS;
  for (byte i = 0; i < 5; i++) {
    sysCfg.led_color[i] = 255;
  }
  sysCfg.led_table = 0;
  sysCfg.led_dimmer = 10;
  sysCfg.led_fade = 0;
  sysCfg.led_speed = 1;
  sysCfg.led_scheme = 0;
  sysCfg.led_width = 1;
  sysCfg.led_wakeup = 0;
  sysCfg.flag.emulation = EMULATION;
  strlcpy(sysCfg.web_password, WEB_PASSWORD, sizeof(sysCfg.web_password));
  strlcpy(sysCfg.ntp_server[0], NTP_SERVER1, sizeof(sysCfg.ntp_server[0]));
  strlcpy(sysCfg.ntp_server[1], NTP_SERVER2, sizeof(sysCfg.ntp_server[1]));
  strlcpy(sysCfg.ntp_server[2], NTP_SERVER3, sizeof(sysCfg.ntp_server[2]));
  for (byte j =0; j < 3; j++) {
    for (byte i = 0; i < strlen(sysCfg.ntp_server[j]); i++) {
      if (sysCfg.ntp_server[j][i] == ',') {
        sysCfg.ntp_server[j][i] = '.';
      }
    }
  }
  parseIP(&sysCfg.ip_address[0], WIFI_IP_ADDRESS);
  parseIP(&sysCfg.ip_address[1], WIFI_GATEWAY);
  parseIP(&sysCfg.ip_address[2], WIFI_SUBNETMASK);
  parseIP(&sysCfg.ip_address[3], WIFI_DNS);
  sysCfg.led_pixels = WS2812_LEDS;
}

/********************************************************************************************/

void CFG_Delta()
{
}


