/*
  boblight_template.h - template settings for Sonoff-Tasmota

  Copyright (C) 2017  Heiko Krupp

  Based on Sonoff-Tasmota by Theo Arendst (C) 2017

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

// User selectable GPIO functionality
enum upins_t {
  GPIO_NONE,           // Not used
  GPIO_WS2812,         // WS2812 Led string
  GPIO_IRSEND,         // IR remote
  GPIO_KEY1,           // Button usually connected to GPIO0
  GPIO_REL1,           // Relays
  GPIO_REL2,
  GPIO_REL3,
  GPIO_REL4,
  GPIO_REL1_INV,
  GPIO_REL2_INV,
  GPIO_REL3_INV,
  GPIO_REL4_INV,
  GPIO_LED1,
  GPIO_LED2,
  GPIO_LED3,
  GPIO_LED4,
  GPIO_LED1_INV,       // Sonoff Basic
  GPIO_LED2_INV,
  GPIO_LED3_INV,
  GPIO_LED4_INV,
  GPIO_SENSOR_END };

// Text in webpage Module Parameters and commands GPIOS and GPIO
const char sensors[GPIO_SENSOR_END][9] PROGMEM = {
  D_SENSOR_NONE,
  D_SENSOR_WS2812,
  D_SENSOR_IRSEND,
  D_SENSOR_BUTTON "1",
  D_SENSOR_RELAY "1",
  D_SENSOR_LED "1I"
  };

// Programmer selectable GPIO functionality offset by user selectable GPIOs
enum fpins_t {
  GPIO_RXD = GPIO_SENSOR_END,  // Serial interface
  GPIO_TXD,            // Serial interface
  GPIO_USER,           // User configurable
  GPIO_MAX };

/********************************************************************************************/

// Supported hardware modules
enum module_t {
  SONOFF_BASIC,
  WEMOS,
  MAXMODULE };

/********************************************************************************************/

#define MAX_GPIO_PIN       18   // Number of supported GPIO

typedef struct MYIO {
  uint8_t      io[MAX_GPIO_PIN];
} myio;

typedef struct MYTMPLT {
  char         name[15];
  myio         gp;
} mytmplt;

const uint8_t nicelist[MAXMODULE] PROGMEM = {
  SONOFF_BASIC,
  WEMOS
};

// Default module settings
const mytmplt modules[MAXMODULE] PROGMEM = {
  { "Sonoff Basic",    // Sonoff Basic (ESP8266)
     GPIO_KEY1,        // GPIO00 Button
     GPIO_USER,        // GPIO01 Serial RXD and Optional sensor
     0,                // GPIO02
     GPIO_USER,        // GPIO03 Serial TXD and Optional sensor
     GPIO_USER,        // GPIO04 Optional sensor
     0,                // GPIO05
     0,                // GPIO06 (SD_CLK   Flash)
     0,                // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
     0,                // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
     0,                // GPIO09 (SD_DATA2 Flash QIO)
     0,                // GPIO10 (SD_DATA3 Flash QIO)
     0,                // GPIO11 (SD_CMD   Flash)
     GPIO_REL1,        // GPIO12 Red Led and Relay (0 = Off, 1 = On)
     GPIO_LED1_INV,    // GPIO13 Green Led (0 = On, 1 = Off)
     GPIO_USER,        // GPIO14 Optional sensor
     0,                // GPIO15
     0,                // GPIO16
     0                 // ADC0 Analog input
  },
  { "WeMos D1 mini",   // WeMos and NodeMCU hardware (ESP8266)
     GPIO_USER,        // GPIO00 D3 Wemos Button Shield
     GPIO_USER,        // GPIO01 TX Serial RXD
     GPIO_USER,        // GPIO02 D4 Wemos DHT Shield
     GPIO_USER,        // GPIO03 RX Serial TXD and Optional sensor
     GPIO_USER,        // GPIO04 D2 Wemos I2C SDA
     GPIO_USER,        // GPIO05 D1 Wemos I2C SCL / Wemos Relay Shield (0 = Off, 1 = On) / Wemos WS2812B RGB led Shield
     0, 0, 0, 0, 0, 0, // Flash connection
     GPIO_USER,        // GPIO12 D6
     GPIO_USER,        // GPIO13 D7
     GPIO_USER,        // GPIO14 D5
     GPIO_USER,        // GPIO15 D8
     GPIO_USER,        // GPIO16 D0 Wemos Wake
  }
};

