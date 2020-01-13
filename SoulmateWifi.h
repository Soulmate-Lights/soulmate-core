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
#include "./SoulmateHomekit.h"
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
    String name = String("soulmate-" + WiFi.macAddress() + String(random(255)));
    name.replace(":", "");
    char copy[50];
    name.toCharArray(copy, 50);
    if (MDNS.begin(copy)) {
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println(F("[Soulmate-Wifi] Error starting MDNS"));
    }
  }

  void stopMDNS() {
    MDNS.end();
  }

  // WiFi configuration
  void connectTo(const char *ssid, const char *pass) {
    Serial.println(F("[Soulmate-Wifi] Connecting to WiFi"));
    preferences.begin("Wifi", false);
    preferences.putString("ssid", String(ssid));
    preferences.putString("pass", String(pass));
    preferences.end();

    // Soulmate.StopBluetooth();
    WiFi.begin(ssid, pass);
  }

  void disconnect() {
    Serial.println(F("[Soulmate-Wifi] Disconnect WIFI now"));
    preferences.begin("Wifi", false);
    preferences.remove("ssid");
    preferences.remove("pass");
    preferences.end();
    WiFi.disconnect();
  }

  // Ping a message to all WebSockets
  void updateWifiClients() {
    ws.textAll(Soulmate.status());
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

    AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);

    // Final frame
    if ((info->index + len) == info->len) {
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(reinterpret_cast<char *>(data));

      if (root == JsonObject::invalid()) {
        Serial.println(F("[Soulmate-Wifi] Invalid JSON object received:"));
        Serial.println(String(reinterpret_cast<char *>(data)));
      } else {
        Soulmate.consumeJson(root);
        updateWifiClients();
      }
    }
  }

  void connectToSavedWifi() {
    // if (WiFi.status() == WL_CONNECTED) return;
    Serial.println(F("[Soulmate-Wifi] connectToSavedWifi"));

    preferences.begin("Wifi", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    if (!ssid.equals("")) {
      isConnected = false;
      // We may or may not need this for ESP32 wifi stability.
      WiFi.disconnect(true);
      // delay(500);
      WiFi.mode(WIFI_STA);
      // delay(500);

      WiFi.begin(ssid.c_str(), pass.c_str());

      // uint32_t endTime = millis() + 2000;
      // while (WiFi.status() != WL_CONNECTED && millis() < endTime) {
      //   delay(500);
      //   Serial.println(F("[Soulmate-Wifi] ."));
      // }
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
        Serial.println(F("[Soulmate-Wifi] WiFi interface ready"));
        break;
      case SYSTEM_EVENT_SCAN_DONE:
        Serial.println(F("[Soulmate-Wifi] Completed scan for access points"));
        break;
      case SYSTEM_EVENT_STA_START:
        Serial.println(F("[Soulmate-Wifi] WiFi client started"));
        break;
      case SYSTEM_EVENT_STA_STOP:
        Serial.println(F("[Soulmate-Wifi] WiFi clients stopped"));
        break;
      case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println(F("[Soulmate-Wifi] Connected to access point"));
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println(F("[Soulmate-Wifi] Disconnected from WiFi access point"));

        if (isConnected) {
          isConnected = false;
          connectToSavedWifi();
        } else {
          Serial.println(F("[Soulmate-Wifi] Spurious disconnect event"));
        }
        break;
      case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        Serial.println(F("[Soulmate-Wifi] Authentication mode of access point has changed"));
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        if (!isConnected) {
          isConnected = true;
          Serial.print("Obtained IP address: ");
          Serial.println(WiFi.localIP());
          startMDNS();
          ws.onEvent(onEvent);
          socketServer.addHandler(&ws);
          socketServer.begin();
          server.begin();

          wifi_event_group = xEventGroupCreate();
          xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
          {
            hap_init();

            uint8_t mac[6];
            esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
            char accessory_id[32] = {
                0,
            };
            sprintf(accessory_id, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            hap_accessory_callback_t callback;
            callback.hap_object_init = hap_object_init;
            acc = hap_accessory_register(
                (char *)ACCESSORY_NAME,
                accessory_id,
                (char *)"111-11-111",
                (char *)MANUFACTURER_NAME,
                HAP_ACCESSORY_CATEGORY_OTHER,
                811,
                1,
                NULL,
                &callback);
          }
        } else {
          Serial.println(F("[Soulmate-Wifi] Spurious got IP event."));
          Serial.println(F("[Soulmate-Wifi] Reconnecting to saved wifi..."));
          connectToSavedWifi();
        }
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        Serial.println(F("[Soulmate-Wifi] [Wifi] Lost IP address and IP address is reset to 0"));
        isConnected = false;
        connectToSavedWifi();
        break;
      case SYSTEM_EVENT_GOT_IP6:
        Serial.println(F("[Soulmate-Wifi] [Wifi] IPv6 is preferred"));
        break;
      default:
        break;
    }
  }

  void setup(void) {
    WiFi.onEvent(WiFiEvent);

    Serial.println(F("[Soulmate-Wifi] Setting up WIFI. Before:"));
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
        // ws.enable(false);
        // ws.closeAll();

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
  }

  void loop() {
    if (restartRequired) {
      delay(500);
      ESP.restart();
    }
  }
}  // namespace SoulmateWifi

void SoulmateLibrary::WifiLoop() { SoulmateWifi::loop(); }
void SoulmateLibrary::WifiSetup() { SoulmateWifi::setup(); }
void SoulmateLibrary::updateWifiClients() { SoulmateWifi::updateWifiClients(); }
void SoulmateLibrary::reconnect() { SoulmateWifi::reconnect(); }
void SoulmateLibrary::disconnectWiFi() { SoulmateWifi::disconnect(); }

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

String SoulmateLibrary::ip() {
  return WiFi.localIP().toString();
}

#endif
#endif  // BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_
