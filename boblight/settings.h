/*
  settings.h - setting variables for Sonoff-Tasmota

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

#define PARAM8_SIZE  23                    // Number of param bytes

typedef union {                            // Restricted by MISRA-C Rule 18.4 but so usefull...
  uint32_t data;                           // Allow bit manipulation using SetOption
  struct {
    uint32_t savestate : 1;                // bit 0
    uint32_t button_restrict : 1;          // bit 1
    uint32_t value_units : 1;              // bit 2
    uint32_t mqtt_enabled : 1;             // bit 3
    uint32_t mqtt_response : 1;            // bit 4
    uint32_t mqtt_power_retain : 1;
    uint32_t mqtt_button_retain : 1;
    uint32_t mqtt_switch_retain : 1;
    uint32_t temperature_conversion : 1;   // bit 8
    uint32_t mqtt_sensor_retain : 1;
    uint32_t mqtt_offline : 1;             // bit 10
    uint32_t button_swap : 1;              // bit 11 (v5.1.6)
    uint32_t stop_flash_rotate : 1;        // bit 12 (v5.2.0)
    uint32_t button_single : 1;            // bit 13 (v5.4.0)
    uint32_t interlock : 1;                // bit 14 (v5.6.0)
    uint32_t pwm_control : 1;              // bit 15 (v5.8.1)
    uint32_t spare16 : 1;
    uint32_t spare17 : 1;
    uint32_t spare18 : 1;
    uint32_t wattage_resolution : 1;
    uint32_t voltage_resolution : 1;
    uint32_t emulation : 2;
    uint32_t energy_resolution : 3;
    uint32_t pressure_resolution : 2;
    uint32_t humidity_resolution : 2;
    uint32_t temperature_resolution : 2;
  };
} sysBitfield;

struct SYSCFG {
  unsigned long cfg_holder;
  unsigned long saveFlag;
  unsigned long version;
  unsigned long bootcount;
  sysBitfield   flag;                     
  int16_t       savedata;
  int8_t        timezone;
  char          otaUrl[101];
  byte          seriallog_level;
  uint8_t       sta_config;
  byte          sta_active;
  char          sta_ssid[2][33];
  char          sta_pwd[2][65];
  char          hostname[33];
  uint8_t       webserver;
  byte          weblog_level;
  uint8_t       power;
  uint8_t       ledstate;
  uint8_t       poweronstate;
  uint16_t      ws_pixels;
  uint8_t       ws_red;
  uint8_t       ws_green;
  uint8_t       ws_blue;
  uint8_t       ws_ledtable;
  uint8_t       ws_dimmer;
  uint8_t       ws_fade;
  uint8_t       ws_speed;
  uint8_t       ws_scheme;
  uint8_t       ws_width;
  uint16_t      ws_wakeup;
  char          friendlyname[4][33];
  uint8_t       sleep;
  uint8_t       module;
  mytmplt       my_module;
  uint16_t      led_pixels;
  uint8_t       led_color[5];
  uint8_t       led_table;
  uint8_t       led_dimmer;
  uint8_t       led_fade;
  uint8_t       led_speed;
  uint8_t       led_scheme;
  uint8_t       led_width;
  uint16_t      led_wakeup;
  char          web_password[33];
  char          ntp_server[3][33];
  uint32_t      ip_address[4];
} sysCfg;

struct RTCMEM {
  uint16_t      valid;
  byte          osw_flag;
  uint8_t       power;
} rtcMem;
