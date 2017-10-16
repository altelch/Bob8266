/*
  xdrv_snfled.ino - PWM, WS2812 and sonoff led support for Sonoff-Tasmota

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
 * PWM, WS2812, Sonoff B1, AiLight, Sonoff Led and BN-SZ01
 *
 * sfl_flg  Module     Color  ColorTemp
 *  1       PWM1       W      no          (Sonoff BN-SZ)
 *  2       PWM2       CW     yes         (Sonoff Led)
 *  3       PWM3       RGB    no          (H801 and MagicHome)
 *  4       PWM4       RGBW   no          (H801 and MagicHome)
 *  5       PWM5       RGBCW  yes         (H801)
 *  9       reserved          no
 * 10       reserved          yes
 * 11       +WS2812    RGB    no
 * 12       AiLight    RGBW   no
 * 13       Sonoff B1  RGBCW  yes
 *
 * led_scheme  WS2812  Others  Effect
 * 0           yes     yes     Color On/Off
 * 1           yes     yes     Wakeup light
 * 2           yes     no      Clock
 * 3           yes     no      Incandescent
 * 4           yes     no      RGB
 * 5           yes     no      Christmas
 * 6           yes     no      Hanukkah
 * 7           yes     no      Kwanzaa
 * 8           yes     no      Rainbow
 * 9           yes     no      Fire
 *
\*********************************************************************************************/

uint8_t ledTable[] = {
  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,
  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,
  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22,
 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32,
 33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45,
 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78,
 80, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99,
101,102,104,105,107,108,110,111,113,114,116,117,119,121,122,124,
125,127,129,130,132,134,135,137,139,141,142,144,146,148,150,151,
153,155,157,159,161,163,165,166,168,170,172,174,176,178,180,182,
184,186,189,191,193,195,197,199,201,204,206,208,210,212,215,217,
219,221,224,226,228,231,233,235,238,240,243,245,248,250,253,255 };

uint8_t sl_dcolor[5];
uint8_t sl_tcolor[5];
uint8_t sl_lcolor[5];

uint8_t sl_power = 0;
uint8_t sl_any = 1;
uint8_t sl_wakeupActive = 0;
uint8_t sl_wakeupDimmer = 0;
uint16_t sl_wakeupCntr = 0;

unsigned long stripTimerCntr = 0;  // Bars and Gradient

/*********************************************************************************************\
 * Sonoff B1 and AiLight inspired by OpenLight https://github.com/icamgo/noduino-sdk
\*********************************************************************************************/

extern "C" {
  void os_delay_us(unsigned int);
}

/********************************************************************************************/

void led_init(void)
{
  ws2812_init();
  if (1 == sysCfg.led_scheme) {
    sysCfg.led_scheme = 0;
  }

  sl_power = 0;
  sl_any = 1;
  sl_wakeupActive = 0;
}

void sl_setDim(uint8_t myDimmer)
{
  float temp;

  float newDim = 100 / (float)myDimmer;
  for (byte i = 0; i < 3; i++) {
    temp = (float)sysCfg.led_color[i] / newDim;
    sl_dcolor[i] = (uint8_t)temp;
  }
}

void sl_setColor()
{
  uint8_t highest = 0;
  float temp;

  for (byte i = 0; i < 3; i++) {
    if (highest < sl_dcolor[i]) {
      highest = sl_dcolor[i];
    }
  }
  float mDim = (float)highest / 2.55;
  sysCfg.led_dimmer = (uint8_t)mDim;
  float newDim = 100 / mDim;
  for (byte i = 0; i < 3; i++) {
    temp = (float)sl_dcolor[i] * newDim;
    sysCfg.led_color[i] = (uint8_t)temp;
  }
}

char* sl_getColor(char* scolor)
{
  sl_setDim(sysCfg.led_dimmer);
  scolor[0] = '\0';
  for (byte i = 0; i < 3; i++) {
    snprintf_P(scolor, 11, PSTR("%s%02X"), scolor, sl_dcolor[i]);
  }
  return scolor;
}

void sl_prepPower()
{
  char scolor[11];
  char scommand[16];

  if (sysCfg.led_dimmer && !(sl_power)) {
    do_cmnd_power(Maxdevice, 0);
  }
  else if (!sysCfg.led_dimmer && sl_power) {
    do_cmnd_power(Maxdevice, 1);
  }

  getPowerDevice(scommand, Maxdevice, sizeof(scommand));
}

void sl_setPower(uint8_t mpower)
{
  sl_power = ((mpower & (0x01 << (Maxdevice -1))) != 0);
  if (sl_wakeupActive) {
    sl_wakeupActive--;
  }
  if (sl_power) {
    sl_any = 1;
  }
  sl_animate();
}

void sl_animate()
{
// {"Wakeup":"Done"}
  uint8_t fadeValue;
  uint8_t cur_col[5];

  stripTimerCntr++;
  if (!sl_power) {  // Power Off
    sleep = sysCfg.sleep;
    stripTimerCntr = 0;
    for (byte i = 0; i < 3; i++) {
      sl_tcolor[i] = 0;
    }
  }
  else {
    sleep = 0;
    switch (sysCfg.led_scheme) {
      case 0:  // Power On
        sl_setDim(sysCfg.led_dimmer);  // Power On
        if (0 == sysCfg.led_fade) {
          for (byte i = 0; i < 3; i++) {
            sl_tcolor[i] = sl_dcolor[i];
          }
        } else {
          for (byte i = 0; i < 3; i++) {
            if (sl_tcolor[i] != sl_dcolor[i]) {
              if (sl_tcolor[i] < sl_dcolor[i]) {
                sl_tcolor[i] += ((sl_dcolor[i] - sl_tcolor[i]) >> sysCfg.led_speed) +1;
              }
              if (sl_tcolor[i] > sl_dcolor[i]) {
                sl_tcolor[i] -= ((sl_tcolor[i] - sl_dcolor[i]) >> sysCfg.led_speed) +1;
              }
            }
          }
        }
        break;
      case 1:  // Power On using wake up duration
        if (2 == sl_wakeupActive) {
          sl_wakeupActive = 1;
          for (byte i = 0; i < 3; i++) {
            sl_tcolor[i] = 0;
          }
          sl_wakeupCntr = 0;
          sl_wakeupDimmer = 0;
        }
        sl_wakeupCntr++;
        if (sl_wakeupCntr > ((sysCfg.led_wakeup * STATES) / sysCfg.led_dimmer)) {
          sl_wakeupCntr = 0;
          sl_wakeupDimmer++;
          if (sl_wakeupDimmer <= sysCfg.led_dimmer) {
            sl_setDim(sl_wakeupDimmer);
            for (byte i = 0; i < 3; i++) {
              sl_tcolor[i] = sl_dcolor[i];
            }
          } else {
            sl_wakeupActive = 0;
            sysCfg.led_scheme = 0;
          }
        }
        break;
      default:
        ws2812_showScheme(sysCfg.led_scheme -2);
    }
  }

  if ((sysCfg.led_scheme < 2) || !sl_power) {
    for (byte i = 0; i < 3; i++) {
      if (sl_lcolor[i] != sl_tcolor[i]) {
        sl_any = 1;
      }
    }
    if (sl_any) {
      sl_any = 0;
      for (byte i = 0; i < 3; i++) {
        sl_lcolor[i] = sl_tcolor[i];
        cur_col[i] = (sysCfg.led_table) ? ledTable[sl_lcolor[i]] : sl_lcolor[i];
      }
      ws2812_setColor(0, cur_col[0], cur_col[1], cur_col[2]);
    }
  }
}

/*********************************************************************************************\
 * Hue support
\*********************************************************************************************/

float sl_Hue = 0.0;
float sl_Sat = 0.0;
float sl_Bri = 0.0;

void sl_rgb2hsb()
{
  sl_setDim(sysCfg.led_dimmer);

  // convert colors to float between (0.0 - 1.0)
  float r = sl_dcolor[0] / 255.0f;
  float g = sl_dcolor[1] / 255.0f;
  float b = sl_dcolor[2] / 255.0f;

  float max = (r > g && r > b) ? r : (g > b) ? g : b;
  float min = (r < g && r < b) ? r : (g < b) ? g : b;

  float d = max - min;

  sl_Hue = 0.0;
  sl_Bri = max;
  sl_Sat = (0.0f == sl_Bri) ? 0 : (d / sl_Bri);

  if (d != 0.0f)
  {
    if (r == max) {
      sl_Hue = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (g == max) {
      sl_Hue = (b - r) / d + 2.0f;
    } else {
      sl_Hue = (r - g) / d + 4.0f;
    }
    sl_Hue /= 6.0f;
  }
}

void sl_hsb2rgb()
{
  float r;
  float g;
  float b;

  float h = sl_Hue;
  float s = sl_Sat;
  float v = sl_Bri;

  if (0.0f == sl_Sat) {
    r = g = b = v; // achromatic or black
  } else {
    if (h < 0.0f) {
      h += 1.0f;
    }
    else if (h >= 1.0f) {
      h -= 1.0f;
    }
    h *= 6.0f;
    int i = (int)h;
    float f = h - i;
    float q = v * (1.0f - s * f);
    float p = v * (1.0f - s);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i) {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      default:
        r = v;
        g = p;
        b = q;
        break;
      }
  }

  sl_dcolor[0] = (uint8_t)(r * 255.0f);
  sl_dcolor[1] = (uint8_t)(g * 255.0f);
  sl_dcolor[2] = (uint8_t)(b * 255.0f);
}

/********************************************************************************************/

void sl_replaceHSB(String *response)
{
  sl_rgb2hsb();
  response->replace("{h}", String((uint16_t)(65535.0f * sl_Hue)));
  response->replace("{s}", String((uint8_t)(254.0f * sl_Sat)));
  response->replace("{b}", String((uint8_t)(254.0f * sl_Bri)));
}

void sl_getHSB(float *hue, float *sat, float *bri)
{
  sl_rgb2hsb(); 
  *hue = sl_Hue;
  *sat = sl_Sat;
  *bri = sl_Bri;
}

void sl_setHSB(float hue, float sat, float bri, uint16_t ct)
{
  uint8_t tmp = (uint8_t)(bri * 100);
  sysCfg.led_dimmer = tmp;
  sl_prepPower();
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

boolean sl_command(char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload)
{
  boolean serviced = true;
  boolean coldim = false;
  char scolor[11];
  char *p;


  if (!strcasecmp_P(type, PSTR(D_CMND_LED)) && (index > 0) && (index <= sysCfg.led_pixels)) {
    if (dataBuf[0] == '#') {
      dataBuf++;
      data_len--;
    }
    uint8_t sl_ledcolor[3];
    if (6 == data_len) {
      for (byte i = 0; i < 3; i++) {
        strlcpy(scolor, dataBuf + (i *2), 3);
        sl_ledcolor[i] = (uint8_t)strtol(scolor, &p, 16);
      }
      ws2812_setColor(index, sl_ledcolor[0], sl_ledcolor[1], sl_ledcolor[2]);
    }
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_PIXELS))) {
    if ((payload > 0) && (payload <= WS2812_MAX_LEDS)) {
      sysCfg.led_pixels = payload;
      ws2812_clear();
      sl_any = 1;
    }
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_WIDTH))) {
    if ((payload >= 0) && (payload <= 4)) {
      sysCfg.led_width = payload;
    }
  }
  else if (!strcasecmp_P(type, PSTR(D_CMND_SCHEME))) {
    if ((payload >= 0) && (payload <= 9)) {
      sysCfg.led_scheme = payload;
      if (1 == sysCfg.led_scheme) {
        sl_wakeupActive = 3;
      }
      do_cmnd_power(Maxdevice, 1);
      stripTimerCntr = 0;
    }
  }
  if (coldim) {
    sl_prepPower();
  }
  return serviced;
}

