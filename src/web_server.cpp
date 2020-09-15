/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_WEB)
#undef ENABLE_DEBUG
#endif

#include <string>

#include "emonesp.h"
#include "web_server.h"
#include "web_server_static.h"
#include "app_config.h"
#include "wifi.h"
#include "mqtt.h"
#include "input.h"
#include "emoncms.h"
#include "ota.h"
#include "debug.h"
#include <NTPClient.h>
#include "espal.h"

AsyncWebServer server(80);          // Create class for Web server
AsyncWebSocket ws("/ws");
AsyncWebSocket wsDebug("/debug/console");
AsyncWebSocket wsEmonTx("/emontx/console");
StaticFileWebHandler staticFile;

StreamSpyReader emonTxBuffer;
StreamSpyReader debugBuffer;

bool enableCors = true;

// Event timeouts
unsigned long wifiRestartTime = 0;
unsigned long mqttRestartTime = 0;
unsigned long systemRestartTime = 0;
unsigned long systemRebootTime = 0;
unsigned long apOffTime = 0;

// Content Types
const char _CONTENT_TYPE_HTML[] PROGMEM = "text/html";
const char _CONTENT_TYPE_TEXT[] PROGMEM = "text/text";
const char _CONTENT_TYPE_CSS[] PROGMEM = "text/css";
const char _CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char _CONTENT_TYPE_JS[] PROGMEM = "application/javascript";
const char _CONTENT_TYPE_JPEG[] PROGMEM = "image/jpeg";
const char _CONTENT_TYPE_PNG[] PROGMEM = "image/png";
const char _CONTENT_TYPE_SVG[] PROGMEM = "image/svg+xml";

// Get running firmware version from build tag environment variable
#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
String currentfirmware = ESCAPEQUOTE(BUILD_TAG);

void dumpRequest(AsyncWebServerRequest *request) {
#ifdef ENABLE_DEBUG
  if(request->method() == HTTP_GET) {
    DBUGF("GET");
  } else if(request->method() == HTTP_POST) {
    DBUGF("POST");
  } else if(request->method() == HTTP_DELETE) {
    DBUGF("DELETE");
  } else if(request->method() == HTTP_PUT) {
    DBUGF("PUT");
  } else if(request->method() == HTTP_PATCH) {
    DBUGF("PATCH");
  } else if(request->method() == HTTP_HEAD) {
    DBUGF("HEAD");
  } else if(request->method() == HTTP_OPTIONS) {
    DBUGF("OPTIONS");
  } else {
    DBUGF("UNKNOWN");
  }
  DBUGF(" http://%s%s", request->host().c_str(), request->url().c_str());

  if(request->contentLength()){
    DBUGF("_CONTENT_TYPE: %s", request->contentType().c_str());
    DBUGF("_CONTENT_LENGTH: %u", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for(i=0; i<headers; i++) {
    AsyncWebHeader* h = request->getHeader(i);
    DBUGF("_HEADER[%s]: %s", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for(i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile()){
      DBUGF("_FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size());
    } else if(p->isPost()){
      DBUGF("_POST[%s]: %s", p->name().c_str(), p->value().c_str());
    } else {
      DBUGF("_GET[%s]: %s", p->name().c_str(), p->value().c_str());
    }
  }
#endif
}

// -------------------------------------------------------------------
// Helper function to perform the standard operations on a request
// -------------------------------------------------------------------
bool requestPreProcess(AsyncWebServerRequest *request, AsyncResponseStream *&response, const __FlashStringHelper *contentType = CONTENT_TYPE_JSON)
{
  dumpRequest(request);

  if(wifi_mode_is_sta() && www_username!="" &&
     false == request->authenticate(www_username.c_str(), www_password.c_str())) {
    request->requestAuthentication(node_name.c_str());
    return false;
  }

  response = request->beginResponseStream(String(contentType));
  if(enableCors) {
    response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
  }

  response->addHeader(F("Cache-Control"), F("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0"));

  return true;
}

// -------------------------------------------------------------------
// Helper function to detect positive string
// -------------------------------------------------------------------
bool isPositive(const String &str) {
  return str == "1" || str == "true";
}

bool isPositive(AsyncWebServerRequest *request, const char *param) {
  bool paramFound = request->hasArg(param);
  String arg = request->arg(param);
  return paramFound && (0 == arg.length() || isPositive(arg));
}

// -------------------------------------------------------------------
// Wifi scan /scan not currently used
// url: /scan
//
// First request will return 0 results unless you start scan from somewhere else (loop/setup)
// Do not request more often than 3-5 seconds
// -------------------------------------------------------------------
void handleScan(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_JSON)) {
    return;
  }

#ifndef ENABLE_ASYNC_WIFI_SCAN
  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2) {
    WiFi.scanNetworks(true, false);
  } else if(n) {
    for (int i = 0; i < n; ++i) {
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  response->print(json);
  request->send(response);
#else // ENABLE_ASYNC_WIFI_SCAN
  // Async WiFi scan need the Git version of the ESP8266 core
  if(WIFI_SCAN_RUNNING == WiFi.scanComplete()) {
    response->setCode(500);
    response->setContentType(CONTENT_TYPE_TEXT);
    response->print("Busy");
    request->send(response);
    return;
  }

  DBUGF("Starting WiFi scan");
  WiFi.scanNetworksAsync([request, response](int networksFound) {
    DBUGF("%d networks found", networksFound);
    String json = "[";
    for (int i = 0; i < networksFound; ++i) {
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
      json += "}";
    }
    WiFi.scanDelete();
    json += "]";
    response->print(json);
    request->send(response);
  }, false);
#endif
}

// -------------------------------------------------------------------
// Handle turning Access point off
// url: /apoff
// -------------------------------------------------------------------
void handleAPOff(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  response->setCode(200);
  response->print("Turning AP Off");
  request->send(response);

  DBUGLN("Turning AP Off");
  apOffTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Save selected network to EEPROM and attempt connection
// url: /savenetwork
// -------------------------------------------------------------------
void handleSaveNetwork(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  String qsid = request->arg("ssid");
  String qpass = request->arg("pass");

  if (qsid != 0) {
    config_save_wifi(qsid, qpass);

    response->setCode(200);
    response->print("saved");
    wifiRestartTime = millis() + 2000;
  } else {
    response->setCode(400);
    response->print("No SSID");
  }

  request->send(response);
}

// -------------------------------------------------------------------
// Save Emoncms
// url: /saveemoncms
// -------------------------------------------------------------------
void handleSaveEmoncms(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  config_save_emoncms(isPositive(request->arg("enable")),
                      request->arg("server"),
                      request->arg("path"),
                      request->arg("node"),
                      request->arg("apikey"),
                      request->arg("fingerprint"));

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %s %s %s %s",
           emoncms_server.c_str(),
           emoncms_path.c_str(),
           emoncms_node.c_str(),
           emoncms_apikey.c_str(),
           emoncms_fingerprint.c_str());
  DBUGLN(tmpStr);

  response->setCode(200);
  response->print(tmpStr);
  request->send(response);
}

// -------------------------------------------------------------------
// Save MQTT Config
// url: /savemqtt
// -------------------------------------------------------------------
void handleSaveMqtt(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  int port = 1883;
  AsyncWebParameter *portParm = request->getParam("port");
  if(nullptr != portParm) {
    port = portParm->value().toInt();
  }

  config_save_mqtt(isPositive(request->arg("enable")),
                   request->arg("server"),
                   port,
                   request->arg("topic"),
                   request->arg("prefix"),
                   request->arg("user"),
                   request->arg("pass"));

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %d %s %s %s %s", mqtt_server.c_str(), port,
          mqtt_topic.c_str(), mqtt_feed_prefix.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
  DBUGLN(tmpStr);

  response->setCode(200);
  response->print(tmpStr);
  request->send(response);

  // If connected disconnect MQTT to trigger re-connect with new details
  mqtt_restart();
}

// -------------------------------------------------------------------
// Save Calibration Config
// url: /savecal
// -------------------------------------------------------------------
void handleSaveCal(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

#ifdef SOLAR_METER
  config_save_cal(request->arg("voltage"),
                  request->arg("ct1"),
                  request->arg("ct2"),
                  request->arg("freq"),
                  request->arg("gain"),
                  request->arg("svoltage"),
                  request->arg("sct1"),
                  request->arg("sct2"));

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %s %s %s %s %s %s %s", voltage_cal.c_str(),
           ct1_cal.c_str(), ct2_cal.c_str(), freq_cal.c_str(), gain_cal.c_str(),
           svoltage_cal.c_str(), sct1_cal.c_str(), sct2_cal.c_str());
  DBUGLN(tmpStr);
#else
  config_save_cal(request->arg("voltage"),
                  request->arg("ct1"),
                  request->arg("ct2"),
                  request->arg("freq"),
                  request->arg("gain"));

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %s %s %s %s", voltage_cal.c_str(),
           ct1_cal.c_str(), ct2_cal.c_str(), freq_cal.c_str(), gain_cal.c_str());
  DBUGLN(tmpStr);
#endif

  response->setCode(200);
  response->print(tmpStr);
  request->send(response);

  // restart the system to load values into energy meter
  systemRestartTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Save the web site user/pass
// url: /saveadmin
// -------------------------------------------------------------------
void handleSaveAdmin(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  String quser = request->arg("user");
  String qpass = request->arg("pass");

  config_save_admin(quser, qpass);

  response->setCode(200);
  response->print("saved");
  request->send(response);
}

// -------------------------------------------------------------------
// Save timer
// url: /savetimer
// -------------------------------------------------------------------
void handleSaveTimer(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  String tmp = request->arg("timer_start1");
  int qtimer_start1 = tmp.toInt();
  tmp = request->arg("timer_stop1");
  int qtimer_stop1 = tmp.toInt();
  tmp = request->arg("timer_start2");
  int qtimer_start2 = tmp.toInt();
  tmp = request->arg("timer_stop2");
  int qtimer_stop2 = tmp.toInt();
  tmp = request->arg("voltage_output");
  int qvoltage_output = tmp.toInt();
  tmp = request->arg("time_offset");
  int qtime_offset = tmp.toInt();

  config_save_timer(qtimer_start1, qtimer_stop1, qtimer_start2, qtimer_stop2, qvoltage_output, qtime_offset);

  mqtt_publish("out/timer",String(qtimer_start1)+" "+String(qtimer_stop1)+" "+String(qtimer_start2)+" "+String(qtimer_stop2)+" "+String(qvoltage_output));

  response->setCode(200);
  response->print("saved");
  request->send(response);
}

void handleSetVout(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }
  String tmp = request->arg("val");
  int vout = tmp.toInt();

  tmp = request->arg("save");
  int qsave = tmp.toInt();

  int save = 0;
  if (qsave==1) save = 1;

  config_save_voltage_output(vout,save);
  mqtt_publish("out/vout",String(vout));

  response->setCode(200);
  if (save) response->print("saved");
  else response->print("ok");
  request->send(response);
}

void
handleSetFlowT(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }
  String tmp = request->arg("val");
  float flow = tmp.toFloat();
  int vout = (int) (flow - 7.14)/0.0371;

  tmp = request->arg("save");
  int qsave = tmp.toInt();

  int save = 0;
  if (qsave==1) save = 1;

  config_save_voltage_output(vout,save);
  if (mqtt_server!=0) mqtt_publish("out/vout",String(vout));

  response->setCode(200);
  if (save) response->print("saved");
  else response->print("ok");
  request->send(response);
}

// -------------------------------------------------------------------
// Last values on atmega serial
// url: /lastvalues
// -------------------------------------------------------------------
void handleLastValues(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  response->setCode(200);
  response->print(last_datastr);
  request->send(response);
}

// -------------------------------------------------------------------
// Returns status json
// url: /status
// -------------------------------------------------------------------
void handleStatus(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response)) {
    return;
  }

  const size_t capacity = JSON_OBJECT_SIZE(40) + 1024;
  DynamicJsonDocument doc(capacity);

  if (wifi_mode_is_sta_only()) {
    doc["mode"] = "STA";
  } else if (wifi_mode_is_ap_only()) {
    doc["mode"] = "AP";
  } else if (wifi_mode_is_ap() && wifi_mode_is_sta()) {
    doc["mode"] = "STA+AP";
  }

//  s += "\"networks\":["+st+"],";
//  s += "\"rssi\":["+rssi+"],";

  doc["wifi_client_connected"] = (int)wifi_client_connected();
  doc["net_connected"] = (int)wifi_client_connected();
  doc["srssi"] = WiFi.RSSI();
  doc["ipaddress"] = ipaddress;

  doc["emoncms_connected"] = (int)emoncms_connected;
  doc["packets_sent"] = packets_sent;
  doc["packets_success"] = packets_success;

  doc["mqtt_connected"] = (int)mqtt_connected();

  doc["free_heap"] = ESPAL.getFreeHeap();
  doc["time"] = getTime();
  doc["ctrl_mode"] = ctrl_mode;
  doc["ctrl_state"] = ctrl_state;

  response->setCode(200);
  serializeJson(doc, *response);
  request->send(response);
}

// -------------------------------------------------------------------
// Returns OpenEVSE Config json
// url: /config
// -------------------------------------------------------------------
void handleConfigGet(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response)) {
    return;
  }

  const size_t capacity = JSON_OBJECT_SIZE(40) + 1024;
  DynamicJsonDocument doc(capacity);

  // EmonESP Config
  doc["espflash"] = ESPAL.getFlashChipSize();
  doc["version"] = currentfirmware;
  doc["node_description"] = node_description;
  doc["node_type"] = node_type;

  config_serialize(doc, true, false, true);

  response->setCode(200);
  serializeJson(doc, *response);
  request->send(response);
}

void handleConfigPost(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response)) {
    return;
  }

  if(request->_tempObject)
  {
    String *body = (String *)request->_tempObject;

    if(config_deserialize(*body)) {
      config_commit();
      response->setCode(200);
      response->print("{\"msg\":\"done\"}");
    } else {
      response->setCode(400);
      response->print("{\"msg\":\"Could not parse JSON\"}");
    }

    delete body;
    request->_tempObject = NULL;
  } else {
    response->setCode(400);
    response->print("{\"msg\":\"No Body\"}");
  }

  request->send(response);
}

// -------------------------------------------------------------------
// Reset config and reboot
// url: /reset
// -------------------------------------------------------------------
void handleRst(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  config_reset();
  #ifdef ESP32
  WiFi.disconnect(false, true);
  #else
  ESPAL.eraseConfig();
  #endif

  response->setCode(200);
  response->print("1");
  request->send(response);

  systemRebootTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Restart (Reboot)
// url: /restart
// -------------------------------------------------------------------
void handleRestart(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
}

  response->setCode(200);
  response->print("1");
  request->send(response);

  systemRestartTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Handle test input API
// url /input
// e.g http://192.168.0.75/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// -------------------------------------------------------------------
void handleInput(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  strcpy(input_string, request->arg("string").c_str());

  response->setCode(200);
  response->print(input_string);
  request->send(response);

  DBUGLN(input_string);
}

// -------------------------------------------------------------------
// Check for updates and display current version
// url: /firmware
// -------------------------------------------------------------------
void handleUpdateCheck(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response)) {
    return;
  }

  DBUGLN("Running: " + currentfirmware);
  // Get latest firmware version number
  // BUG/HACK/TODO: This will block, should be done in the loop call
  String latestfirmware = ota_get_latest_version();
  DBUGLN("Latest: " + latestfirmware);
  // Update web interface with firmware version(s)
  String s = "{";
  s += "\"current\":\""+currentfirmware+"\",";
  s += "\"latest\":\""+latestfirmware+"\"";
  s += "}";

  response->setCode(200);
  response->print(s);
  request->send(response);
}

// -------------------------------------------------------------------
// Update firmware
// url: /update
// -------------------------------------------------------------------
void handleUpdate(AsyncWebServerRequest *request) {
  // BUG/HACK/TODO: This will block, should be done in the loop call

  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }


  DBUGLN("UPDATING...");
  delay(500);

  t_httpUpdate_return ret = ota_http_update();

  int retCode = 400;
  String str = "Error";
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      str = "Update failed error (";
      str += ESPhttpUpdate.getLastError();
      str += "): ";
      str += ESPhttpUpdate.getLastErrorString();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      str = "No update, running latest firmware";
      break;
    case HTTP_UPDATE_OK:
      retCode = 200;
      str = "Update done!";
      break;
  }
  response->setCode(retCode);
  response->print(str);
  request->send(response);

  DBUGLN(str);
}

// -------------------------------------------------------------------
// Update firmware
// url: /update
// -------------------------------------------------------------------
void
handleUpdateGet(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_HTML)) {
    return;
  }

  response->setCode(200);
  response->print(
    F("<html><form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='firmware'> "
        "<input type='submit' value='Update'>"
      "</form></html>"));
  request->send(response);
}

void
handleUpdatePost(AsyncWebServerRequest *request) {
  bool shouldReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, CONTENT_TYPE_TEXT, shouldReboot ? "OK" : "FAIL");
  response->addHeader("Connection", "close");
  request->send(response);

  if(shouldReboot) {
    systemRestartTime = millis() + 1000;
  }
}

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void
handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if(!index)
  {
    dumpRequest(request);

    DBUGF("Update Start: %s", filename.c_str());

    DBUGVAR(data[0]);
    //int command = data[0] == 0xE9 ? U_FLASH : U_SPIFFS;
    int command = U_FLASH;
    size_t updateSize = U_FLASH == command ?
      (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000 :
      ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);

    DBUGVAR(command);
    DBUGVAR(updateSize);

    Update.runAsync(true);
    if(!Update.begin(updateSize, command)) {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
  }
  if(!Update.hasError())
  {
    if(Update.write(data, len) != len) {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
  }
  if(final)
  {
    if(Update.end(true)) {
      DBUGF("Update Success: %uB\n", index+len);
    } else {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
  }
}

void handleDescribe(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200, CONTENT_TYPE_TEXT, "smartplug");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void handleTime(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200, CONTENT_TYPE_TEXT,getTime());
  request->send(response);
}

void handleCtrlMode(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }
  String qmode = request->arg("mode");
  if (qmode=="On") ctrl_mode = "On";
  if (qmode=="Off") ctrl_mode = "Off";
  if (qmode=="Timer") ctrl_mode = "Timer";

  if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));

  response->setCode(200);
  response->print(qmode);
  request->send(response);
}

void handleDebug(AsyncWebServerRequest *request, StreamSpy &spy)
{
  AsyncResponseStream *response;
  if(false == requestPreProcess(request, response, CONTENT_TYPE_TEXT)) {
    return;
  }

  response->setCode(200);
  spy.printBuffer(*response);
  request->send(response);

}

void handleNotFound(AsyncWebServerRequest *request)
{
  DBUG("NOT_FOUND: ");
  dumpRequest(request);

  if(wifi_mode_is_ap_only()) {
    // Redirect to the home page in AP mode (for the captive portal)
    AsyncResponseStream *response = request->beginResponseStream(String(CONTENT_TYPE_HTML));

    String url = F("http://");
    url += ipaddress;

    String s = F("<html><body><a href=\"");
    s += url;
    s += F("\">EmonESP</a></body></html>");

    response->setCode(301);
    response->addHeader(F("Location"), url);
    response->print(s);
    request->send(response);
  } else {
    request->send(404);
  }
}

void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  if(!index) {
    DBUGF("BodyStart: %u", total);
    request->_tempObject = new String();
  }
  String *body = (String *)request->_tempObject;
  DBUGF("%.*s", len, (const char*)data);
  body->concat((const char*)data, len);
  if(index + len == total) {
    DBUGF("BodyEnd: %u", total);
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    DBUGF("ws[%s][%u] connect", server->url(), client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT) {
    DBUGF("ws[%s][%u] disconnect: %u", server->url(), client->id());
  } else if(type == WS_EVT_ERROR) {
    DBUGF("ws[%s][%u] error(%u): %s", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG) {
    DBUGF("ws[%s][%u] pong[%u]: %s", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      DBUGF("ws[%s][%u] %s-message[%u]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", len);
    } else {
      // TODO: handle messages that are comprised of multiple frames or the frame is split into multiple packets
    }
  }
}

void onEmonTxEvent(AsyncWebSocket * server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_DATA) {
    EMONTX_PORT.write(data, len);
  }
}

void streamBuffer(StreamSpyReader &buffer, AsyncWebSocket &client)
{
  if(buffer.available() > 0 && client.availableForWriteAll())
  {
    uint8_t *buf;
    size_t len;

    buffer.getBuffer(buf, len);
    client.textAll(buf, len);
    buffer.readBuffer(len);
  }
}

void
web_server_setup()
{
  // Add the Web Socket server
  ws.onEvent(onWsEvent);
  wsEmonTx.onEvent(onEmonTxEvent);

  server.addHandler(&ws);
  server.addHandler(&wsDebug);
  server.addHandler(&wsEmonTx);
  server.addHandler(&staticFile);

  // Handle status updates
  server.on("/status", handleStatus);
  server.on("/config", HTTP_GET, handleConfigGet);
  server.on("/config", HTTP_POST, handleConfigPost, NULL, handleBody);

  // Handle HTTP web interface button presses
  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveemoncms", handleSaveEmoncms);
  server.on("/savemqtt", handleSaveMqtt);
  server.on("/saveadmin", handleSaveAdmin);
  server.on("/savetimer", handleSaveTimer);

  server.on("/reset", handleRst);
  server.on("/restart", handleRestart);

  server.on("/scan", handleScan);
  server.on("/apoff", handleAPOff);
  server.on("/input", handleInput);
  server.on("/lastvalues", handleLastValues);

  server.on("/emoncms/describe", handleDescribe);
  server.on("/time", handleTime);
  server.on("/ctrlmode", handleCtrlMode);
  server.on("/vout", handleSetVout);
  server.on("/flow", handleSetFlowT);

  // Simple Firmware Update Form
  server.on("/upload", HTTP_GET, handleUpdateGet);
  server.on("/upload", HTTP_POST, handleUpdatePost, handleUpdateUpload);

  server.on("/firmware", handleUpdateCheck);
  server.on("/update", handleUpdate);

  // Remote debug consoles
  server.on("/debug", [](AsyncWebServerRequest *request) {
    handleDebug(request, SerialDebug);
  });
  debugBuffer.attach(SerialDebug);

  server.on("/emontx", [](AsyncWebServerRequest *request) {
    handleDebug(request, SerialEmonTx);
  });
  emonTxBuffer.attach(SerialEmonTx);

  server.onNotFound(handleNotFound);
  server.onRequestBody(handleBody);

  server.begin();

  DEBUG.println("Server started");
}

void
web_server_loop() {
  Profile_Start(web_server_loop);

  // Do we need to restart the WiFi?
  if(wifiRestartTime > 0 && millis() > wifiRestartTime) {
    wifiRestartTime = 0;
    wifi_restart();
  }

  // Do we need to restart MQTT?
  if(mqttRestartTime > 0 && millis() > mqttRestartTime) {
    mqttRestartTime = 0;
    mqtt_restart();
  }

  // Do we need to turn off the access point?
  if(apOffTime > 0 && millis() > apOffTime) {
    apOffTime = 0;
    wifi_turn_off_ap();
  }

  // Do we need to restart the system?
  if(systemRestartTime > 0 && millis() > systemRestartTime) {
    systemRestartTime = 0;
    wifi_disconnect();
    ESP.restart();
  }

  // Do we need to reboot the system?
  if(systemRebootTime > 0 && millis() > systemRebootTime) {
    systemRebootTime = 0;
    wifi_disconnect();
    ESP.reset();
  }

  streamBuffer(debugBuffer, wsDebug);
  streamBuffer(emonTxBuffer, wsEmonTx);

  Profile_End(web_server_loop, 5);
}

void web_server_event(JsonDocument &event)
{
  String json;
  serializeJson(event, json);
  ws.textAll(json);
}
