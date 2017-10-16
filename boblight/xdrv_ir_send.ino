/*
  xdrv_ir_send.ino - infra red support for Sonoff-Tasmota

  Copyright (C) 2017  Heiko Krupp, Lazar Obradovic and Theo Arends

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

#ifdef USE_IR_REMOTE
/*********************************************************************************************\
 * IR Remote send and receive using IRremoteESP8266 library
\*********************************************************************************************/

#ifndef USE_IR_HVAC
#include <IRremoteESP8266.h>
#else
#include <IRMitsubishiAC.h>

// HVAC TOSHIBA_
#define HVAC_TOSHIBA_HDR_MARK 4400
#define HVAC_TOSHIBA_HDR_SPACE 4300
#define HVAC_TOSHIBA_BIT_MARK 543
#define HVAC_TOSHIBA_ONE_SPACE 1623
#define HVAC_MISTUBISHI_ZERO_SPACE 472
#define HVAC_TOSHIBA_RPT_MARK 440
#define HVAC_TOSHIBA_RPT_SPACE 7048 // Above original iremote limit
#define HVAC_TOSHIBA_DATALEN 9

IRMitsubishiAC *mitsubir = NULL;

const char FANSPEED[] = "A12345S";
const char HVACMODE[] = "HDCA";
#endif

/*********************************************************************************************\
 * IR Send
\*********************************************************************************************/

IRsend *irsend = NULL;

void ir_send_init(void)
{
  irsend = new IRsend(pin[GPIO_IRSEND]); // an IR led is at GPIO_IRSEND
  irsend->begin();

#ifdef USE_IR_HVAC
  mitsubir = new IRMitsubishiAC(pin[GPIO_IRSEND]);
#endif //USE_IR_HVAC
}

#ifdef USE_IR_RECEIVE
/*********************************************************************************************\
 * IR Receive
\*********************************************************************************************/

#define IR_TIME_AVOID_DUPLICATE 500 // Milliseconds

IRrecv *irrecv = NULL;
unsigned long ir_lasttime = 0;

void ir_recv_init(void)
{
  irrecv = new IRrecv(pin[GPIO_IRRECV]); // an IR led is at GPIO_IRRECV
  irrecv->enableIRIn();                  // Start the receiver

  //  addLog_P(LOG_LEVEL_DEBUG, PSTR("IrReceive initialized"));
}

void ir_recv_check()
{
  char sirtype[100];
  char *protocol;
  int8_t iridx = 0;
  uint8_t diridx = 0;

  decode_results results;

  if (irrecv->decode(&results)) {

    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_IRR "RawLen %d, Bits %d, Value %08X, Decode %d"),
               results.rawlen, results.bits, results.value, results.decode_type);
    addLog(LOG_LEVEL_DEBUG);

    unsigned long now = millis();
    if ((now - ir_lasttime > IR_TIME_AVOID_DUPLICATE) && (UNKNOWN != results.decode_type) && (results.bits > 0)) {
      ir_lasttime = now;

      iridx = results.decode_type;
      if ((iridx < 0) || (iridx > 14)) {
        iridx = 0;
      }
      diridx = iridx;
      // Based on IRremoteESP8266.h enum decode_type_t
      snprintf_P(sirtype, sizeof(sirtype), PSTR("UNKNOWN RC5 RC6 NEC SONY PANASONIC JVC SAMSUNG WHYNTER AIWA_RC_T501 LG SANYO MITSUBISHI DISH SHARP"));
      protocol = strtok(sirtype, " ");
      while (iridx) {
        iridx--;
        protocol = strtok(NULL, " ");
      }

      snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("{\"" D_IRRECEIVED "\":{\"" D_IR_PROTOCOL "\":\"%s\", \"" D_IR_BITS "\":%d, \"" D_IR_DATA "\":\"%X\"}}"),
                 protocol, results.bits, results.value);
      mqtt_publish_topic_P(6, PSTR(D_IRRECEIVED));
#ifdef USE_DOMOTICZ
      unsigned long value = results.value | (diridx << 28); // [Protocol:4, Data:28]
      domoticz_sensor(DZ_COUNT, value);                    // Send data as Domoticz Counter value
#endif                                                     // USE_DOMOTICZ
    }

    irrecv->resume();
  }
}
#endif // USE_IR_RECEIVE

#ifdef USE_IR_HVAC
/*********************************************************************************************\
 * IR Heating, Ventilation and Air Conditioning using IRMitsubishiAC library
\*********************************************************************************************/

boolean ir_hvac_toshiba(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp)
{
  unsigned int rawdata[2 + 2 * 8 * HVAC_TOSHIBA_DATALEN + 2];
  byte data[HVAC_TOSHIBA_DATALEN] = {0xF2, 0x0D, 0x03, 0xFC, 0x01, 0x00, 0x00, 0x00, 0x00};

  char *p;
  char *token;
  uint8_t mode;

  if (HVAC_Mode == NULL) {
    p = (char *)HVACMODE; // default HVAC_HOT
  }
  else {
    p = strchr(HVACMODE, toupper(HVAC_Mode[0]));
  }
  if (!p) {
    return true;
  }
  data[6] = (p - HVACMODE) ^ 0x03; // HOT = 0x03, DRY = 0x02, COOL = 0x01, AUTO = 0x00

  if (!HVAC_Power) {
    data[6] = (byte)0x07; // Turn OFF HVAC
  }

  if (HVAC_FanMode == NULL) {
    p = (char *)FANSPEED; // default FAN_SPEED_AUTO
  }
  else {
    p = strchr(FANSPEED, toupper(HVAC_FanMode[0]));
  }
  if (!p) {
    return true;
  }
  mode = p - FANSPEED + 1;
  if ((1 == mode) || (7 == mode)) {
    mode = 0;
  }
  mode = mode << 5; // AUTO = 0x00, SPEED = 0x40, 0x60, 0x80, 0xA0, 0xC0, SILENT = 0x00
  data[6] = data[6] | mode;

  byte Temp;
  if (HVAC_Temp > 30) {
    Temp = 30;
  }
  else if (HVAC_Temp < 17) {
    Temp = 17;
  }
  else {
    Temp = HVAC_Temp;
  }
  data[5] = (byte)Temp - 17 << 4;

  data[HVAC_TOSHIBA_DATALEN - 1] = 0;
  for (int x = 0; x < HVAC_TOSHIBA_DATALEN - 1; x++) {
    data[HVAC_TOSHIBA_DATALEN - 1] = (byte)data[x] ^ data[HVAC_TOSHIBA_DATALEN - 1]; // CRC is a simple bits addition
  }

  int i = 0;
  byte mask = 1;

  //header
  rawdata[i++] = HVAC_TOSHIBA_HDR_MARK;
  rawdata[i++] = HVAC_TOSHIBA_HDR_SPACE;

  //data
  for (int b = 0; b < HVAC_TOSHIBA_DATALEN; b++) {
    for (mask = B10000000; mask > 0; mask >>= 1) { //iterate through bit mask
      if (data[b] & mask) { // Bit ONE
        rawdata[i++] = HVAC_TOSHIBA_BIT_MARK;
        rawdata[i++] = HVAC_TOSHIBA_ONE_SPACE;
      }
      else { // Bit ZERO
        rawdata[i++] = HVAC_TOSHIBA_BIT_MARK;
        rawdata[i++] = HVAC_MISTUBISHI_ZERO_SPACE;
      }
    }
  }

  //trailer
  rawdata[i++] = HVAC_TOSHIBA_RPT_MARK;
  rawdata[i++] = HVAC_TOSHIBA_RPT_SPACE;

  noInterrupts();
  irsend->sendRaw(rawdata, i, 38);
  irsend->sendRaw(rawdata, i, 38);
  interrupts();

  return false;
}

boolean ir_hvac_mitsubishi(const char *HVAC_Mode, const char *HVAC_FanMode, boolean HVAC_Power, int HVAC_Temp)
{
  char *p;
  char *token;
  uint8_t mode;

  mitsubir->stateReset();

  if (HVAC_Mode == NULL) {
    p = (char *)HVACMODE; // default HVAC_HOT
  }
  else {
    p = strchr(HVACMODE, toupper(HVAC_Mode[0]));
  }
  if (!p) {
    return true;
  }
  mode = (p - HVACMODE + 1) << 3; // HOT = 0x08, DRY = 0x10, COOL = 0x18, AUTO = 0x20
  mitsubir->setMode(mode);

  mitsubir->setPower(HVAC_Power);

  if (HVAC_FanMode == NULL) {
    p = (char *)FANSPEED; // default FAN_SPEED_AUTO
  }
  else {
    p = strchr(FANSPEED, toupper(HVAC_FanMode[0]));
  }
  if (!p) {
    return true;
  }
  mode = p - FANSPEED; // AUTO = 0, SPEED = 1 .. 5, SILENT = 6
  mitsubir->setFan(mode);

  mitsubir->setTemp(HVAC_Temp);
  mitsubir->setVane(MITSUBISHI_AC_VANE_AUTO);
  mitsubir->send();

  //  snprintf_P(log_data, sizeof(log_data), PSTR("IRHVAC: Mitsubishi Power %d, Mode %d, FanSpeed %d, Temp %d, VaneMode %d"),
  //    mitsubir->getPower(), mitsubir->getMode(), mitsubir->getFan(), mitsubir->getTemp(), mitsubir->getVane());
  //  addLog(LOG_LEVEL_DEBUG);

  return false;
}
#endif // USE_IR_HVAC

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

/*
 * ArduinoJSON entry used to calculate jsonBuf: JSON_OBJECT_SIZE(3) + 40 = 96
 IRsend:
 { "protocol": "SAMSUNG", "bits": 32, "data": 551502015 }

 IRhvac:
 { "Vendor": "<Toshiba|Mitsubishi>", "Power": <0|1>, "Mode": "<Hot|Cold|Dry|Auto>", "FanSpeed": "<1|2|3|4|5|Auto|Silence>", "Temp": <17..30> }
*/

boolean ir_send_command(char *type, uint16_t index, char *dataBuf, uint16_t data_len, int16_t payload)
{
  boolean serviced = true;
  boolean error = false;
  char dataBufUc[data_len];
  const char *protocol;
  uint8_t bits = 0;
  uint32_t data = 0;

  const char *HVAC_Mode;
  const char *HVAC_FanMode;
  const char *HVAC_Vendor;
  int HVAC_Temp = 21;
  boolean HVAC_Power = true;

  for (uint16_t i = 0; i <= sizeof(dataBufUc); i++) {
    dataBufUc[i] = toupper(dataBuf[i]);
  }
  if (!strcasecmp_P(type, PSTR(D_CMND_IRSEND))) {
    if (data_len) {
      StaticJsonBuffer<128> jsonBuf;
      JsonObject &ir_json = jsonBuf.parseObject(dataBufUc);
      if (!ir_json.success()) {
        Serial.println(PSTR("{\"" D_CMND_IRSEND "\":\"" D_INVALID_JSON "\"}")); // JSON decode failed
      }
      else {
        Serial.println(PSTR("{\"" D_CMND_IRSEND "\":\"" D_DONE "\"}"));
        protocol = ir_json[D_IR_PROTOCOL];
        bits = ir_json[D_IR_BITS];
        data = ir_json[D_IR_DATA];
        if (protocol && bits && data) {
          if (!strcasecmp_P(protocol, PSTR("NEC")))
            irsend->sendNEC(data, bits);
          else if (!strcasecmp_P(protocol, PSTR("SONY")))
            irsend->sendSony(data, bits);
          else if (!strcasecmp_P(protocol, PSTR("RC5")))
            irsend->sendRC5(data, bits);
          else if (!strcasecmp_P(protocol, PSTR("RC6")))
            irsend->sendRC6(data, bits);
          else if (!strcasecmp_P(protocol, PSTR("DISH")))
            irsend->sendDISH(data, bits);
          else if (!strcasecmp_P(protocol, PSTR("JVC")))
            irsend->sendJVC(data, bits, 1);
          else if (!strcasecmp_P(protocol, PSTR("SAMSUNG")))
            irsend->sendSAMSUNG(data, bits);
          else {
            Serial.println(PSTR("{\"" D_CMND_IRSEND "\":\"" D_PROTOCOL_NOT_SUPPORTED "\"}"));
          }
        }
        else {
          error = true;
        }
      }
    }
    else {
      error = true;
    }
    if (error) {
      Serial.println(PSTR("{\"" D_CMND_IRSEND "\":\"" D_NO " " D_IR_PROTOCOL ", " D_IR_BITS " " D_OR " " D_IR_DATA "\"}"));
    }
  }
  else {
    serviced = false; // Unknown command
  }
  return serviced;
}
#endif // USE_IR_REMOTE
