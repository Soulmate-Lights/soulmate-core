// Copyright 2019 Soulmate Lighting, LLC
/* cpplint-ignore readability/casting */

#ifdef ESP32
#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_

#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include "./settings.h"

Preferences preferences;

AsyncWebServer server(80);
AsyncWebServer socketServer(81);
AsyncWebSocket ws("/");

namespace SoulmateWifi {

  bool connected = false;
  bool isConnected = false;
  bool restartRequired = false;

  // Bounjour / mDNS presence announcement.
  void startMDNS() {
    MDNS.end();

    delay(100);

    Serial.println(F("Starting MDNS. Before:"));
    Serial.println(String(ESP.getFreeHeap()));

    String name = String("soulmate-" + WiFi.macAddress() + String(random(255)));
    name.replace(":", "");
    char copy[50];
    name.toCharArray(copy, 50);

    if (MDNS.begin(copy)) {
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println(F("Error starting MDNS"));
    }

    Serial.println(F("Starting MDNS. After:"));
    Serial.println(String(ESP.getFreeHeap()));
  }

  void stopMDNS() {
    MDNS.end();
  }

  // WiFi configuration
  void connectTo(const char *ssid, const char *pass) {
    Serial.println(F("Connecting. Before:"));
    Serial.println(String(ESP.getFreeHeap()));
    preferences.begin("Wifi", false);
    preferences.putString("ssid", String(ssid));
    preferences.putString("pass", String(pass));
    preferences.end();

    // Soulmate.StopBluetooth();
    WiFi.begin(ssid, pass);

    Serial.println(F("Connecting. After:"));
    Serial.println(String(ESP.getFreeHeap()));
  }

  void disconnect() {
    Serial.println(F("Disconnect WIFI now"));
    preferences.begin("Wifi", false);
    preferences.remove("ssid");
    preferences.remove("pass");
    preferences.end();

    // server.reset();
    // socketServer.reset();
    WiFi.disconnect();

    // delay(1000);
    // ESP.restart();
  }

  // Ping a message to all WebSockets
  void updateWifiClients() {
    String s = Soulmate.status();
    ws.textAll(s);
  }

  // WebSockets event received¡
  void onEvent(
      AsyncWebSocket *server,
      AsyncWebSocketClient *client,
      AwsEventType type,
      void *arg,
      uint8_t *data,
      size_t len) {
    if (type != WS_EVT_DATA) return;

    bool finished = false;
    AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);

    // Final frame
    if ((info->index + len) == info->len) {
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(reinterpret_cast<char *>(data));

      if (root == JsonObject::invalid()) {
        Serial.println(F("Invalid JSON object received:"));
        Serial.println(String(reinterpret_cast<char *>(data)));
      } else {
        Soulmate.consumeJson(root);
        updateWifiClients();
      }
    }
  }

  void connectToSavedWifi() {
    // if (WiFi.status() == WL_CONNECTED) return;
    Serial.println(F("connectToSavedWifi"));

    preferences.begin("Wifi", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    if (!ssid.equals("")) {
      isConnected = false;
      // We may or may not need this for ESP32 wifi stability.
      WiFi.disconnect(true);
      delay(500);
      WiFi.mode(WIFI_STA);
      delay(500);

      WiFi.begin(ssid.c_str(), pass.c_str());

      uint32_t endTime = millis() + 2000;
      while (WiFi.status() != WL_CONNECTED && millis() < endTime) {
        delay(500);
        Serial.println(F("."));
      }
    }
    preferences.end();
  }

  void reconnect() {
    disconnect();
    connectToSavedWifi();
  }

  void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
      case SYSTEM_EVENT_WIFI_READY:
        Serial.println(F("[Wifi] WiFi interface ready"));
        break;
      case SYSTEM_EVENT_SCAN_DONE:
        Serial.println(F("[Wifi] Completed scan for access points"));
        break;
      case SYSTEM_EVENT_STA_START:
        Serial.println(F("[Wifi] WiFi client started"));
        break;
      case SYSTEM_EVENT_STA_STOP:
        Serial.println(F("[Wifi] WiFi clients stopped"));
        break;
      case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println(F("[Wifi] Connected to access point"));
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println(F("[Wifi] Disconnected from WiFi access point"));
        if (isConnected) {
          isConnected = false;
          connectToSavedWifi();
        } else {
          Serial.println(F("Spurious disconnect event"));
        }
        break;
      case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        Serial.println(F("[Wifi] Authentication mode of access point has changed"));
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        if (!isConnected) {
          Serial.println(F("Got IP. Before:"));
          Serial.println(String(ESP.getFreeHeap()));

          isConnected = true;
          Serial.print("Obtained IP address: ");
          Serial.println(WiFi.localIP());
          Serial.println(F("Starting MDNS"));
          startMDNS();
          Serial.println(F("Got IP. After:"));
          Serial.println(String(ESP.getFreeHeap()));

          Serial.println(F("WS on Event. Before:"));
          Serial.println(String(ESP.getFreeHeap()));
          ws.onEvent(onEvent);
          Serial.println(F("WS on Event. After:"));
          Serial.println(String(ESP.getFreeHeap()));
          socketServer.addHandler(&ws);

          Serial.println(F("Server 1: Before:"));
          Serial.println(String(ESP.getFreeHeap()));
          socketServer.begin();
          Serial.println(F("Server 1: After:"));
          Serial.println(String(ESP.getFreeHeap()));
          Serial.println(F("Server 2: Before:"));
          Serial.println(String(ESP.getFreeHeap()));
          server.begin();
          Serial.println(F("Server 2: After:"));
          Serial.println(String(ESP.getFreeHeap()));
        } else {
          Serial.println(F("Spurious got IP event."));
          Serial.println(F("Reconnecting to saved wifi..."));
          connectToSavedWifi();
        }
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        Serial.println(F("[Wifi] Lost IP address and IP address is reset to 0"));
        break;
      case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        Serial.println(F("[Wifi] WiFi Protected Setup (WPS): succeeded in enrollee mode"));
        break;
      case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        Serial.println(F("[Wifi] WiFi Protected Setup (WPS): failed in enrollee mode"));
        break;
      case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        Serial.println(F("[Wifi] WiFi Protected Setup (WPS): timeout in enrollee mode"));
        break;
      case SYSTEM_EVENT_STA_WPS_ER_PIN:
        Serial.println(F("[Wifi] WiFi Protected Setup (WPS): pin code in enrollee mode"));
        break;
      case SYSTEM_EVENT_AP_START:
        Serial.println(F("[Wifi] WiFi access point started"));
        break;
      case SYSTEM_EVENT_AP_STOP:
        Serial.println(F("[Wifi] WiFi access point  stopped"));
        break;
      case SYSTEM_EVENT_AP_STACONNECTED:
        Serial.println(F("[Wifi] Client connected"));
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        Serial.println(F("[Wifi] Client disconnected"));
        break;
      case SYSTEM_EVENT_AP_STAIPASSIGNED:
        Serial.println(F("[Wifi] Assigned IP address to client"));
        break;
      case SYSTEM_EVENT_AP_PROBEREQRECVED:
        Serial.println(F("[Wifi] Received probe request"));
        break;
      case SYSTEM_EVENT_GOT_IP6:
        Serial.println(F("[Wifi] IPv6 is preferred"));
        break;
      case SYSTEM_EVENT_ETH_START:
        Serial.println(F("[Wifi] Ethernet started"));
        break;
      case SYSTEM_EVENT_ETH_STOP:
        Serial.println(F("[Wifi] Ethernet stopped"));
        break;
      case SYSTEM_EVENT_ETH_CONNECTED:
        Serial.println(F("[Wifi] Ethernet connected"));
        break;
      case SYSTEM_EVENT_ETH_DISCONNECTED:
        Serial.println(F("[Wifi] Ethernet disconnected"));
        break;
      case SYSTEM_EVENT_ETH_GOT_IP:
        Serial.println(F("[Wifi] Obtained IP address"));
        break;
    }
  }

  void setup(void) {
    WiFi.onEvent(WiFiEvent);

    Serial.println(F("Setting up WIFI. Before:"));
    Serial.println(String(ESP.getFreeHeap()));

    connectToSavedWifi();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("text/plain"), Soulmate.status());
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("text/plain"), Soulmate.status());
    });

    // respond to GET requests on URL /heap
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("text/plain"), String(ESP.getFreeHeap()));
    });

    server.on("/ota", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.println(F("Closing request"));
      AsyncWebServerResponse *response = request->beginResponse(200, F("text/plain"), "OK");
      response->addHeader("Connection", "close");
      request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      EVERY_N_MILLISECONDS(100) {
        if (request->hasHeader(F("Content-Length"))) {
          AsyncWebHeader* h = request->getHeader("Content-Length");
          float size = String(h->value().c_str()).toFloat();
          float percentage = (float)index / size;
          Soulmate.lightPercentage(percentage);
        }
      }

      if (!index) {

        stopMDNS();
        Soulmate.StopBluetooth();
        Soulmate.stop();
        SPIFFS.end();
        ws.enable(false);
        ws.closeAll();

        if (!Update.begin()) {
          Update.printError(Serial);
          restartRequired = true;
        }
      }

      if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
          restartRequired = true;
        }
      }

      if (final) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
        // For some multi-thread reason,
        // it's better to restart in the main loop thread.
        restartRequired = true;
      } });

    Serial.println(F("Setting up WIFI. After:"));
    Serial.println(String(ESP.getFreeHeap()));
  }

  void loop() {
    if (restartRequired) {
      delay(500);
      ESP.restart();
    }
  }
}  // namespace SoulmateWifi

void SoulmateLibrary::WifiLoop() {
  SoulmateWifi::loop();
}

void SoulmateLibrary::WifiSetup() {
  SoulmateWifi::setup();
}

void SoulmateLibrary::updateWifiClients() {
  SoulmateWifi::updateWifiClients();
}

void SoulmateLibrary::reconnect() {
  SoulmateWifi::reconnect();
}

void SoulmateLibrary::connectTo(const char *ssid, const char *pass) {
  preferences.begin("Wifi", false);
  preferences.putString("ssid", String(ssid));
  preferences.putString("pass", String(pass));
  preferences.end();

  SoulmateSettings::setStartInWifiMode(true);
  ESP.restart();
}

bool SoulmateLibrary::wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void SoulmateLibrary::disconnectWiFi() {
  SoulmateWifi::disconnect();
}

String SoulmateLibrary::ip() {
  return WiFi.localIP().toString();
}

#endif
#endif  // BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_
