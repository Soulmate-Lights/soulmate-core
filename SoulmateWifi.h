// Copyright 2019 Soulmate Lighting, LLC
/* cpplint-ignore readability/casting */

#ifndef BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_
#define BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_

#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>

#include "./SoulmateHomekit.h"
#include "./SoulmateOTA.h"
#include "./SoulmateSettings.h"
#include "./SoulmateTime.h"

#include <GeneralUtils.h>

Preferences preferences;

AsyncWebServer server(80);
AsyncWebServer socketServer(81);
AsyncWebSocket ws("/");

void delayAndConnect(void *parameter) {
  Serial.println("[Soulmate-Wifi] delayAndConnect starting.");
  Serial.println("[Soulmate-Wifi] Disconnect WiFi...");
  WiFi.disconnect();
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  Serial.println("[Soulmate-Wifi] Read credentials...");
  preferences.begin("Wifi", false);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  preferences.end();

  if (!ssid.equals("")) {
    Serial.println("[Soulmate-Wifi] Set STA mode...");
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Serial.println("[Soulmate-Wifi] WiFi.begin()...");
    Serial.println(ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
  }

  vTaskDelete(NULL);
}

void delayAndConnectWayLater(void *parameter) {
  Serial.println("[Soulmate-Wifi] delayAndConnectWayLater starting.");
  Serial.println("[Soulmate-Wifi] Disconnect WiFi...");
  WiFi.disconnect();
  vTaskDelay(20000 / portTICK_PERIOD_MS);

  Serial.println("[Soulmate-Wifi] Read credentials...");
  preferences.begin("Wifi", false);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  preferences.end();

  if (!ssid.equals("")) {
    Serial.println("[Soulmate-Wifi] Set STA mode...");
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Serial.println("[Soulmate-Wifi] WiFi.begin()...");
    WiFi.begin(ssid.c_str(), pass.c_str());
  }

  vTaskDelete(NULL);
}

namespace SoulmateWifi {

  bool connected = false;
  bool isConnected = false;
  bool restartRequired = false;

  // Bounjour / mDNS presence announcement.
  void startMDNS() {
    // MDNS.end();
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
    // WiFi.begin(ssid, pass);
    xTaskCreate(delayAndConnect, "DelayAndConnect", 10000, NULL, 0, NULL);
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

  uint16_t streamedPixelIndex = 0;
  double lastFrameReceived;

  bool isStreaming() {
    return (millis() - lastFrameReceived) < 500;
  }

  // WebSockets event received¡
  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {

    if (type != WS_EVT_DATA)
      return;

    AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);

    // Streaming pixels
    if (info->opcode == WS_BINARY) {
      lastFrameReceived = millis();
      for (uint16_t i = 0; i < len; i += 4) {
        bool isFirst = data[i] == 1;
        uint8_t red = data[i+1];
        uint8_t green = data[i+2];
        uint8_t blue = data[i+3];

        Soulmate.currentRoutine = -2;
        if (isFirst) {
          streamedPixelIndex = 0;
          Soulmate.fastLedShow();
        }

        if (streamedPixelIndex < N_LEDS) {
          Soulmate.leds[streamedPixelIndex] = CRGB(red, green, blue);
        }

        streamedPixelIndex++;
      }

      return;
    }

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
    preferences.end();

    if (!ssid.equals("")) {
      isConnected = false;
      // We may or may not need this for ESP32 wifi stability.
      xTaskCreate(delayAndConnect, "DelayAndConnect", 10000, NULL, 1, NULL);
    }
  }

  void reconnect() {
    Serial.println(F("[Soulmate-Wifi] Reconnecting..."));
    isConnected = false;
    connectToSavedWifi();
  }

  int spuriousCount = 0;

  void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("WiFiEvent");
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
      Serial.println(F("[Soulmate-Wifi] Disconnected from WiFi access point"));
      if (isConnected) {
        teardownHomekit();
        isConnected = false;
        Serial.println("Was connected. Reconnect");
        xTaskCreate(delayAndConnect, "DelayAndConnect", 10000, NULL, 0, NULL);
      } else {
        if (info.disconnected.reason == WIFI_REASON_NO_AP_FOUND) {
          spuriousCount++;
          Serial.println("The access point wasn't found. We can probably stop after a while.");
          if (spuriousCount > 5) {
            Serial.println("That's it, disconnect Wifi");
            spuriousCount = 0;
            WiFi.disconnect();
            xTaskCreate(delayAndConnectWayLater, "DelayAndConnectWayLater", 10000, NULL, 0, NULL);
          }
        } else {
          Serial.println(GeneralUtils::wifiErrorToString(info.disconnected.reason));
          Serial.println(F("[Soulmate-Wifi] Spurious disconnect event"));
        }
      }
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

        long receivedSeconds = fetchTime();
        if (receivedSeconds > 0) {
          unsigned long currentSeconds = millis() / 1000;
          unsigned long startedSeconds = receivedSeconds - currentSeconds;
          Circadian::startTrackingTime(startedSeconds);
        }

        connectHomekit();
      } else {
        Serial.println(F("[Soulmate-Wifi] Spurious got IP event."));
      }
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println(F("[Soulmate-Wifi] [Wifi] Lost IP address and IP address "
                       "is reset to 0"));
      reconnect();
      break;
    default:
      break;
    }
  }

  void setup(void) {
    WiFi.onEvent(WiFiEvent);

    connectToSavedWifi();
    setupHomekit();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("text/plain"), Soulmate.status());
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("text/plain"), Soulmate.status());
    });

    server.on(
        "/ota", HTTP_POST,
        [](AsyncWebServerRequest *request) {
          AsyncWebServerResponse *response =
              request->beginResponse(200, F("text/plain"), "OK");
          response->addHeader("Connection", "close");
          request->send(response);
        },
        [](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {
          EVERY_N_MILLISECONDS(100) {
            if (request->hasHeader(F("Content-Length"))) {
              AsyncWebHeader *h = request->getHeader("Content-Length");
              float size = String(h->value().c_str()).toFloat();
              float percentage = (float)index / size;
              Soulmate.lightPercentage(percentage);
            }
          }

          if (!index) {
            Soulmate.stop();
            SPIFFS.end();

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
              Serial.printf("Update Success: %uB\n", index + len);
            } else {
              Update.printError(Serial);
            }

            // For some multi-thread reason,
            // it's better to restart in the main loop thread.
            restartRequired = true;
          }
        });
  }

  void loop() {
    EVERY_N_SECONDS(1) {
      if (
        Soulmate.currentRoutine == -2 && !isStreaming()
      ) {
        Soulmate.currentRoutine = 0;
      }
    }

    EVERY_N_SECONDS(1) {
      ws.cleanupClients();
    }

    if (restartRequired) {
      delay(500);
      ESP.restart();
    }
  }
} // namespace SoulmateWifi

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
void SoulmateLibrary::disconnectWiFi() {
  SoulmateWifi::disconnect();
}
bool SoulmateLibrary::isStreaming() {
  return SoulmateWifi::isStreaming();
}

void SoulmateLibrary::connectTo(const char *ssid, const char *pass) {
  preferences.begin("Wifi", false);
  preferences.putString("ssid", String(ssid));
  preferences.putString("pass", String(pass));
  preferences.end();

  Serial.println("[Soulmate-Wifi] Saved credentials, start connect task");
  xTaskCreate(delayAndConnect, "DelayAndConnect", 10000, NULL, 0, NULL);
}

bool SoulmateLibrary::wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String SoulmateLibrary::ip() {
  return WiFi.localIP().toString();
}

#endif // BUILDER_LIBRARIES_SOULMATE_SOULMATEWIFI_H_
