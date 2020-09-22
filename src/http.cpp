/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "emonesp.h"
#include "http.h"

#define HTTP_TIMEOUT 4

#ifdef ESP32
WiFiClientSecure client_ssl;        // Create class for ESP32 HTTPS TCP connections get_http()
WiFiClient client;                  // Create class for ESP32 HTTP TCP connections get_http()
#elif defined(ESP8266)
WiFiClientSecure client;            // Create class for ESP8266 HTTP TCP connections get_https()
HTTPClient http;                    // Create class for ESP8266 HTTP TCP connections get_http()
#endif


static char request[MAX_DATA_LEN+100];

#ifdef ESP32
// -------------------------------------------------------------------
// HTTP or HTTPS GET Request
// url: N/A
// -------------------------------------------------------------------
String get_http(const char * host, String url, int port, const char * fingerprint) {
  WiFiClientSecure * http;

  if (fingerprint) {
    http = &client_ssl;
  } else {
    http = (WiFiClientSecure *) &client;
  }

  // Use WiFiClient class to create TCP connections
  if (!http->connect(host, port, HTTP_TIMEOUT*1000)) {
    DEBUG.printf("%s:%d\n", host, port);      //debug
    return ("Connection error");
  }
  http->setTimeout(HTTP_TIMEOUT);

  if (!fingerprint || http->verify(fingerprint, host)) {
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url, host);
    http->print(request);
    // Handle wait for reply and timeout
    unsigned long timeout = millis();
    while (http->available() == 0) {
      if (millis() - timeout > (HTTP_TIMEOUT*1000)) {
        http->stop();
        return ("Client Timeout");
      }
      #ifdef ENABLE_WDT
      feedLoopWDT();
      #endif
    }
    // Handle message receive
    while (http->available()) {
      String line = http->readStringUntil('\r');
      DEBUG.println(line);      //debug
      if (line.startsWith("HTTP/1.1 200 OK")) {
        return ("ok");
      }
    }
  } else {
    return ("HTTPS fingerprint no match");
  }
  return ("error " + String(host));
}

#elif defined(ESP8266)
// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------

String get_https(const char* fingerprint, const char* host, String url, int httpsPort){
  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpsPort)) {
    DEBUG.print(host + httpsPort); //debug
    return("Connection error");
  }
  if (client.verify(fingerprint, host)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host +
                 "\r\n" + "Connection: close\r\n\r\n");
     // Handle wait for reply and timeout
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        client.stop();
        return("Client Timeout");
      }
      #ifdef ENABLE_WDT
      feedLoopWDT();
      #endif
    }
    // Handle message receive
    while(client.available()){
      String line = client.readStringUntil('\r');
      DEBUG.println(line); //debug
      if (line.startsWith("HTTP/1.1 200 OK")) {
        return("ok");
      }
    }
  } else {
    return("HTTPS fingerprint no match");
  }
  return("error " + String(host));
}

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
String get_http(const char *host, String url){
  http.begin(String("http://") + host + String(url));
  int httpCode = http.GET();
  if((httpCode > 0) && (httpCode == HTTP_CODE_OK)){
    String payload = http.getString();
    DEBUG.println(payload);
    http.end();
    return(payload);
  } else {
    http.end();
    return("server error: "+String(httpCode));
  }
} // end http_get
#endif
