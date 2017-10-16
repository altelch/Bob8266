/*
  xdrv_wemohue.ino - wemo and hue support for Sonoff-Tasmota

  Copyright (C) 2017  Heiko Krupp and Theo Arends

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
 * Belkin WeMo and Philips Hue bridge emulation
\*********************************************************************************************/

#define UDP_BUFFER_SIZE 200         // Max UDP buffer size needed for M-SEARCH message

boolean udpConnected = false;

char packetBuffer[UDP_BUFFER_SIZE]; // buffer to hold incoming UDP packet
IPAddress ipMulticast(239, 255, 255, 250); // Simple Service Discovery Protocol (SSDP)
uint32_t portMulticast = 1900;      // Multicast address and port

/*********************************************************************************************\
 * Hue Bridge UPNP support routines
 * Need to send 3 response packets with varying ST and USN
 *
 * Using Espressif Inc Mac Address of 5C:CF:7F:00:00:00
 * Philips Lighting is 00:17:88:00:00:00
\*********************************************************************************************/

const char HUE_RESPONSE[] PROGMEM =
  "HTTP/1.1 200 OK\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "CACHE-CONTROL: max-age=100\r\n"
  "EXT:\r\n"
  "LOCATION: http://{r1}:80/description.xml\r\n"
  "SERVER: Linux/3.14.0 UPnP/1.0 IpBridge/1.17.0\r\n"
  "hue-bridgeid: {r2}\r\n";
const char HUE_ST1[] PROGMEM =
  "ST: upnp:rootdevice\r\n"
  "USN: uuid:{r3}::upnp:rootdevice\r\n"
  "\r\n";
const char HUE_ST2[] PROGMEM =
  "ST: uuid:{r3}\r\n"
  "USN: uuid:{r3}\r\n"
  "\r\n";
const char HUE_ST3[] PROGMEM =
  "ST: urn:schemas-upnp-org:device:basic:1\r\n"
  "USN: uuid:{r3}\r\n"
  "\r\n";

String hue_bridgeid()
{
  String temp = WiFi.macAddress();
  temp.replace(":", "");
  String bridgeid = temp.substring(0, 6) + "FFFE" + temp.substring(6);
  return bridgeid;  // 5CCF7FFFFE139F3D
}

String hue_serial()
{
  String serial = WiFi.macAddress();
  serial.replace(":", "");
  serial.toLowerCase();
  return serial;  // 5ccf7f139f3d
}

String hue_UUID()
{
  String uuid = F("f6543a06-da50-11ba-8d8f-");
  uuid += hue_serial();
  return uuid;  // f6543a06-da50-11ba-8d8f-5ccf7f139f3d
}

void hue_respondToMSearch()
{
  char message[MSG_SIZE];

  if (portUDP.beginPacket(portUDP.remoteIP(), portUDP.remotePort())) {
    String response1 = FPSTR(HUE_RESPONSE);
    response1.replace("{r1}", WiFi.localIP().toString());
    response1.replace("{r2}", hue_bridgeid());

    String response = response1;
    response += FPSTR(HUE_ST1);
    response.replace("{r3}", hue_UUID());
    portUDP.write(response.c_str());
    portUDP.endPacket();

    response = response1;
    response += FPSTR(HUE_ST2);
    response.replace("{r3}", hue_UUID());
    portUDP.write(response.c_str());
    portUDP.endPacket();

    response = response1;
    response += FPSTR(HUE_ST3);
    response.replace("{r3}", hue_UUID());
    portUDP.write(response.c_str());
    portUDP.endPacket();

    snprintf_P(message, sizeof(message), PSTR(D_3_RESPONSE_PACKETS_SENT));
  } else {
    snprintf_P(message, sizeof(message), PSTR(D_FAILED_TO_SEND_RESPONSE));
  }
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_UPNP D_HUE " %s " D_TO " %s:%d"),
    message, portUDP.remoteIP().toString().c_str(), portUDP.remotePort());
  addLog(LOG_LEVEL_DEBUG);
}

/*********************************************************************************************\
 * Philips Hue bridge UDP multicast support
\*********************************************************************************************/

boolean UDP_Disconnect()
{
  if (udpConnected) {
    WiFiUDP::stopAll();
    addLog_P(LOG_LEVEL_DEBUG, PSTR(D_LOG_UPNP D_MULTICAST_DISABLED));
    udpConnected = false;
  }
  return udpConnected;
}

boolean UDP_Connect()
{
  if (!udpConnected) {
    if (portUDP.beginMulticast(WiFi.localIP(), ipMulticast, portMulticast)) {
      addLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_UPNP D_MULTICAST_REJOINED));
      udpConnected = true;
    } else {
      addLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_UPNP D_MULTICAST_JOIN_FAILED));
      udpConnected = false;
    }
  }
  return udpConnected;
}

void pollUDP()
{
  if (udpConnected) {
    if (portUDP.parsePacket()) {
      int len = portUDP.read(packetBuffer, UDP_BUFFER_SIZE -1);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      String request = packetBuffer;

//      addLog_P(LOG_LEVEL_DEBUG_MORE, PSTR("UDP: Packet received"));
//      addLog_P(LOG_LEVEL_DEBUG_MORE, packetBuffer);

      if (request.indexOf("M-SEARCH") >= 0) {
        request.toLowerCase();
        request.replace(" ", "");

//        addLog_P(LOG_LEVEL_DEBUG_MORE, PSTR("UDP: M-SEARCH Packet received"));
//        addLog_P(LOG_LEVEL_DEBUG_MORE, request.c_str());

        if (((request.indexOf(F("st:urn:schemas-upnp-org:device:basic:1")) > 0) ||
             (request.indexOf(F("st:upnp:rootdevice")) > 0) ||
             (request.indexOf(F("st:ssdpsearch:all")) > 0) ||
             (request.indexOf(F("st:ssdp:all")) > 0))) {
          hue_respondToMSearch();
        }
      }
    }
  }
}

/*********************************************************************************************\
 * Hue web server additions
\*********************************************************************************************/

const char HUE_DESCRIPTION_XML[] PROGMEM =
  "<?xml version=\"1.0\"?>"
  "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion>"
    "<major>1</major>"
    "<minor>0</minor>"
  "</specVersion>"
  "<URLBase>http://{x1}:80/</URLBase>"
  "<device>"
    "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
    "<friendlyName>Amazon-Echo-HA-Bridge ({x1})</friendlyName>"
    "<manufacturer>Royal Philips Electronics</manufacturer>"
    "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
    "<modelName>Philips hue bridge 2012</modelName>"
    "<modelNumber>929000226503</modelNumber>"
    "<serialNumber>{x3}</serialNumber>"
    "<UDN>uuid:{x2}</UDN>"
  "</device>"
  "</root>\r\n"
  "\r\n";
const char HUE_LIGHT_STATUS_JSON[] PROGMEM =
  "\"on\":{state},"
  "\"bri\":{b},"
  "\"hue\":{h},"
  "\"sat\":{s},"
  "\"xy\":[0.5, 0.5],"
  "\"ct\":500,"
  "\"alert\":\"none\","
  "\"effect\":\"none\","
  "\"colormode\":\"hs\","
  "\"reachable\":true";
const char HUE_LIGHTS_STATUS_JSON[] PROGMEM =
  "\"type\":\"Extended color light\","
  "\"name\":\"{j1}\","
  "\"modelid\":\"LCT007\","
  "\"uniqueid\":\"{j2}\","
  "\"swversion\":\"5.50.1.19085\""
  "}";
const char HUE_GROUP0_STATUS_JSON[] PROGMEM =
  "{\"name\":\"Group 0\","
   "\"lights\":[{l1}],"
   "\"type\":\"LightGroup\","
   "\"action\":{";
//     "\"scene\":\"none\",";
const char HUE_CONFIG_RESPONSE_JSON[] PROGMEM =
  "{\"name\":\"Philips hue\","
   "\"mac\":\"{mac}\","
   "\"dhcp\":true,"
   "\"ipaddress\":\"{ip}\","
   "\"netmask\":\"{mask}\","
   "\"gateway\":\"{gw}\","
   "\"proxyaddress\":\"none\","
   "\"proxyport\":0,"
   "\"bridgeid\":\"{bid}\","
   "\"UTC\":\"{dt}\","
   "\"whitelist\":{\"{id}\":{"
     "\"last use date\":\"{dt}\","
     "\"create date\":\"{dt}\","
     "\"name\":\"Remote\"}},"
   "\"swversion\":\"01039019\","
   "\"apiversion\":\"1.17.0\","
   "\"swupdate\":{\"updatestate\":0,\"url\":\"\",\"text\":\"\",\"notify\": false},"
   "\"linkbutton\":false,"
   "\"portalservices\":false"
  "}";
const char HUE_LIGHT_RESPONSE_JSON[] PROGMEM =
  "{\"success\":{\"/lights/{id}/state/{cmd}\":{res}}}";
const char HUE_ERROR_JSON[] PROGMEM =
  "[{\"error\":{\"type\":901,\"address\":\"/\",\"description\":\"Internal Error\"}}]";

/********************************************************************************************/

String hue_deviceId(uint8_t id)
{
  String deviceid = WiFi.macAddress() + F(":00:11-") + String(id);
  deviceid.toLowerCase();
  return deviceid;  // 5c:cf:7f:13:9f:3d:00:11-1
}

String hue_userId()
{
  char userid[7];

  snprintf_P(userid, sizeof(userid), PSTR("%03x"), ESP.getChipId());
  return String(userid);
}

void handleUPnPsetupHue()
{
  addLog_P(LOG_LEVEL_DEBUG, S_LOG_HTTP, PSTR(D_HUE_BRIDGE_SETUP));
  String description_xml = FPSTR(HUE_DESCRIPTION_XML);
  description_xml.replace("{x1}", WiFi.localIP().toString());
  description_xml.replace("{x2}", hue_UUID());
  description_xml.replace("{x3}", hue_serial());
  webServer->send(200, FPSTR(HDR_CTYPE_XML), description_xml);
}

void hue_todo(String *path)
{
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_API_NOT_IMPLEMENTED " (%s)"), path->c_str());
  addLog(LOG_LEVEL_DEBUG_MORE);

  webServer->send(200, FPSTR(HDR_CTYPE_JSON), "{}");
}

void hue_config_response(String *response)
{
  *response += FPSTR(HUE_CONFIG_RESPONSE_JSON);
  response->replace("{mac}", WiFi.macAddress());
  response->replace("{ip}", WiFi.localIP().toString());
  response->replace("{mask}", WiFi.subnetMask().toString());
  response->replace("{gw}", WiFi.gatewayIP().toString());
  response->replace("{bid}", hue_bridgeid());
  response->replace("{dt}", getUTCDateTime());
  response->replace("{id}", hue_userId());
}

void hue_config(String *path)
{
  String response = "";
  hue_config_response(&response);
  webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void hue_light_status(byte device, String *response)
{
  *response += FPSTR(HUE_LIGHT_STATUS_JSON);
  response->replace("{state}", (power & (0x01 << (device-1))) ? "true" : "false");

  sl_replaceHSB(response);
}

void hue_global_cfg(String *path)
{
  String response;

  path->remove(0,1);                                 // cut leading / to get <id>
  response = F("{\"lights\":{\"");
  for (uint8_t i = 1; i <= Maxdevice; i++) {
    response += i;
    response += F("\":{\"state\":{");
    hue_light_status(i, &response);
    response += "},";
    response += FPSTR(HUE_LIGHTS_STATUS_JSON);
    response.replace("{j1}", sysCfg.friendlyname[i-1]);
    response.replace("{j2}", hue_deviceId(i));
    if (i < Maxdevice) {
      response += ",\"";
    }
  }
  response += F("},\"groups\":{},\"schedules\":{},\"config\":");
  hue_config_response(&response);
  response += "}";
  webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void hue_auth(String *path)
{
  char response[38];

  snprintf_P(response, sizeof(response), PSTR("[{\"success\":{\"username\":\"%s\"}}]"), hue_userId().c_str());
  webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void hue_lights(String *path)
{
/*
 * http://sonoff/api/username/lights/1/state?1={"on":true,"hue":56100,"sat":254,"bri":254,"alert":"none","transitiontime":40}
 */
  String response;
  uint8_t device = 1;
  uint16_t tmp = 0;
  int16_t pos = 0;
  float bri = 0;
  float hue = 0;
  float sat = 0;
  uint16_t ct = 0;
  bool resp = false;
  bool on = false;
  bool change = false;
  char id[4];

  path->remove(0,path->indexOf("/lights"));          // Remove until /lights
  if (path->endsWith("/lights")) {                   // Got /lights
    response = "{\"";
    for (uint8_t i = 1; i <= Maxdevice; i++) {
      response += i;
      response += F("\":{\"state\":{");
      hue_light_status(i, &response);
      response += "},";
      response += FPSTR(HUE_LIGHTS_STATUS_JSON);
      response.replace("{j1}", sysCfg.friendlyname[i-1]);
      response.replace("{j2}", hue_deviceId(i));
      if (i < Maxdevice) {
        response += ",\"";
      }
    }
    response += "}";
    webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else if (path->endsWith("/state")) {               // Got ID/state
    path->remove(0,8);                               // Remove /lights/
    path->remove(path->indexOf("/state"));           // Remove /state
    device = atoi(path->c_str());
    if ((device < 1) || (device > Maxdevice)) {
      device = 1;
    }
    if (1 == webServer->args()) {
      response = "[";

      StaticJsonBuffer<400> jsonBuffer;
      JsonObject &hue_json = jsonBuffer.parseObject(webServer->arg(0));
      if (hue_json.containsKey("on")) {

        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id}", String(device));
        response.replace("{cmd}", "on");

        on = hue_json["on"];
        switch(on)
        {
          case false : do_cmnd_power(device, 0);
                       response.replace("{res}", "false");
                       break;
          case true  : do_cmnd_power(device, 1);
                       response.replace("{res}", "true");
                       break;
          default    : response.replace("{res}", (power & (0x01 << (device-1))) ? "true" : "false");
                       break;
        }
        resp = true;
      }
      sl_getHSB(&hue,&sat,&bri);

      if (hue_json.containsKey("bri")) {
        tmp = hue_json["bri"];
        bri = (float)tmp / 254.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id}", String(device));
        response.replace("{cmd}", "bri");
        response.replace("{res}", String(tmp));
        resp = true;
        change = true;
      }
      if (hue_json.containsKey("hue")) {
        tmp = hue_json["hue"];
        hue = (float)tmp / 65535.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id}", String(device));
        response.replace("{cmd}", "hue");
        response.replace("{res}", String(tmp));
        resp = true;
        change = true;
      }
      if (hue_json.containsKey("sat")) {
        tmp = hue_json["sat"];
        sat = (float)tmp / 254.0f;
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id}", String(device));
        response.replace("{cmd}", "sat");
        response.replace("{res}", String(tmp));
        change = true;
      }
      if (hue_json.containsKey("ct")) {              // Color temperature 153 (Cold) to 500 (Warm)
        ct = hue_json["ct"];
        if (resp) {
          response += ",";
        }
        response += FPSTR(HUE_LIGHT_RESPONSE_JSON);
        response.replace("{id}", String(device));
        response.replace("{cmd}", "ct");
        response.replace("{res}", String(ct));
        change = true;
      }
      if (change) {
        sl_setHSB(hue, sat, bri, ct);
        change = false;
      }
      response += "]";
      if (2 == response.length()) {
        response = FPSTR(HUE_ERROR_JSON);
      }
    }
    else {
      response = FPSTR(HUE_ERROR_JSON);
    }

    webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else if(path->indexOf("/lights/") >= 0) {          // Got /lights/ID
    path->remove(0,8);                               // Remove /lights/
    device = atoi(path->c_str());
    if ((device < 1) || (device > Maxdevice)) {
      device = 1;
    }
    response += F("{\"state\":{");
    hue_light_status(device, &response);
    response += "},";
    response += FPSTR(HUE_LIGHTS_STATUS_JSON);
    response.replace("{j1}", sysCfg.friendlyname[device-1]);
    response.replace("{j2}", hue_deviceId(device));
    webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
  }
  else {
    webServer->send(406, FPSTR(HDR_CTYPE_JSON), "{}");
  }
}

void hue_groups(String *path)
{
/*
 * http://sonoff/api/username/groups?1={"name":"Woonkamer","lights":[],"type":"Room","class":"Living room"})
 */
  String response = "{}";

  if (path->endsWith("/0")) {
    response = FPSTR(HUE_GROUP0_STATUS_JSON);
    String lights = F("\"1\"");
    for (uint8_t i = 2; i <= Maxdevice; i++) {
      lights += ",\"" + String(i) + "\"";
    }
    response.replace("{l1}", lights);
    hue_light_status(1, &response);
    response += F("}}");
  }

  webServer->send(200, FPSTR(HDR_CTYPE_JSON), response);
}

void handle_hue_api(String *path)
{
  /* HUE API uses /api/<userid>/<command> syntax. The userid is created by the echo device and
   * on original HUE the pressed button allows for creation of this user. We simply ignore the
   * user part and allow every caller as with Web or WeMo.
   *
   * (c) Heiko Krupp, 2017
   */

  uint8_t args = 0;

  path->remove(0, 4);                                // remove /api
  uint16_t apilen = path->length();
  snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_API " (%s)"), path->c_str());
  addLog(LOG_LEVEL_DEBUG_MORE);                      // HTP: Hue API (//lights/1/state)
  for (args = 0; args < webServer->args(); args++) {
    String json = webServer->arg(args);
    snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_HTTP D_HUE_POST_ARGS " (%s)"), json.c_str());
    addLog(LOG_LEVEL_DEBUG_MORE);                    // HTP: Hue POST args ({"on":false})
  }

  if (path->endsWith("/invalid/")) {}                // Just ignore
  else if (!apilen) hue_auth(path);                  // New HUE App setup
  else if (path->endsWith("/")) hue_auth(path);      // New HUE App setup
  else if (path->endsWith("/config")) hue_config(path);
  else if (path->indexOf("/lights") >= 0) hue_lights(path);
  else if (path->indexOf("/groups") >= 0) hue_groups(path);
  else if (path->endsWith("/schedules")) hue_todo(path);
  else if (path->endsWith("/sensors")) hue_todo(path);
  else if (path->endsWith("/scenes")) hue_todo(path);
  else if (path->endsWith("/rules")) hue_todo(path);
  else hue_global_cfg(path);
}
