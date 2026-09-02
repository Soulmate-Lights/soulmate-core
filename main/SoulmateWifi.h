// Copyright 2019 Soulmate Lighting, LLC
/* cpplint-ignore readability/casting */

#ifndef SOULMATE_SOULMATEWIFI_H_
#define SOULMATE_SOULMATEWIFI_H_

#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "SoulmateHomekit.h"
#include "SoulmateSettings.h"
#include "SoulmateTime.h"

#include <GeneralUtils.h>

// How long an association attempt gets before the supervisor declares it dead
// and schedules another. The ESP32 supplicant normally gives up and reports a
// reason code well inside this; the timeout is the backstop for the case where
// no event arrives at all.
#ifndef SOULMATE_WIFI_ASSOC_TIMEOUT_MS
  #define SOULMATE_WIFI_ASSOC_TIMEOUT_MS 12000
#endif

// Backoff between attempts. The first retry after a working connection drops is
// immediate; after that this doubles, capped.
#ifndef SOULMATE_WIFI_BACKOFF_MIN_MS
  #define SOULMATE_WIFI_BACKOFF_MIN_MS 5000
#endif
#ifndef SOULMATE_WIFI_BACKOFF_MAX_MS
  #define SOULMATE_WIFI_BACKOFF_MAX_MS 60000
#endif

// Radio power save.
//
// arduino-esp32 leaves STA mode in WIFI_PS_MIN_MODEM, which powers the receiver
// down between DTIM beacons. The WiFi task is pinned to core 1
// (CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1), sharing it with the NimBLE host,
// loopTask and the network event task — so it can be late to a beacon window,
// and missed beacons are exactly what produces WIFI_REASON_BEACON_TIMEOUT.
// This is a mains-powered lamp; there is nothing worth saving.
//
// The trade-off is BLE. With coexistence arbitration on, holding the WiFi
// receiver up continuously leaves the BT controller less radio time. If BLE
// control becomes unreliable, WIFI_PS_MIN_MODEM is the other end of this dial.
//
// WIFI_PS_NONE is only reachable when the BT controller is off. IDF asserts and
// calls abort() in esp_wifi_set_ps() if power save is disabled while Bluetooth
// is enabled ("Should enable WiFi modem sleep when both WiFi and Bluetooth are
// enabled"), and sdkconfig ships CONFIG_BT_ENABLED=y with BluetoothSetup()
// running from setup() unless SKIP_BLUETOOTH is defined. Since this is set from
// SYSTEM_EVENT_STA_START, the abort lands on the first association attempt and
// the board boot-loops rather than misbehaving in some recoverable way.
#ifndef SOULMATE_WIFI_POWER_SAVE
  #ifdef SKIP_BLUETOOTH
    #define SOULMATE_WIFI_POWER_SAVE WIFI_PS_NONE
  #else
    #define SOULMATE_WIFI_POWER_SAVE WIFI_PS_MIN_MODEM
  #endif
#endif

// Max TX power. Kept at the previous value so this isn't a silent behaviour
// change, but made a knob: on a panel drawing amps, a 19.5 dBm transmit burst
// (~300 mA) sits on a rail that is already sagging, and dialling this back is
// worth trying before blaming the AP. WIFI_POWER_15dBm is the usual next step.
#ifndef SOULMATE_WIFI_TX_POWER
  #define SOULMATE_WIFI_TX_POWER WIFI_POWER_19_5dBm
#endif

Preferences preferences;

AsyncWebServer server(80);
AsyncWebServer socketServer(81);
AsyncWebSocket ws("/");

namespace SoulmateWifi {

  bool isConnected = false;
  bool restartRequired = false;

  // Connection state machine.
  //
  // There used to be no state here beyond `isConnected`, and five call sites
  // that each did `xTaskCreate(delayAndConnect, ...)` with nothing coordinating
  // them. Two of those tasks running at once interleave their
  // WiFi.disconnect() / WiFi.begin() pairs, which is a reliable way to wedge
  // the supplicant. One owner, one state variable.
  enum State {
    IDLE,       // not connected, nothing in flight
    CONNECTING, // WiFi.begin() issued, waiting for an IP
    UP          // associated, DHCP done
  };

  State state = IDLE;
  uint32_t stateEnteredAt = 0;
  uint32_t attemptCount = 0;

  // Cached so the once-per-second supervisor tick doesn't read NVS.
  bool haveCredentials = false;

  // Our own connect task calls WiFi.disconnect() before WiFi.begin(), which
  // raises a DISCONNECTED event with reason ASSOC_LEAVE. Ignoring disconnects
  // for the first moments of a CONNECTING state distinguishes that from a real
  // association failure without needing a flag shared across three tasks.
  static const uint32_t kSelfDisconnectWindowMs = 2500;

  void enterState(State next) {
    state = next;
    stateEnteredAt = millis();
  }

  // Bonjour / mDNS presence announcement.
  //
  // The name used to carry a random suffix — "soulmate-" + MAC + random(255) —
  // so it changed on every reconnect. Anything that had cached the hostname
  // could no longer reach the lamp, which from the app's side is
  // indistinguishable from the WiFi having dropped. (random() was also never
  // seeded, so the "random" suffix was the same sequence every boot anyway.)
  // The MAC alone is already unique.
  void startMDNS() {
    MDNS.end();

    String name = "soulmate-" + WiFi.macAddress();
    name.replace(":", "");
    name.toLowerCase();

    char copy[50];
    name.toCharArray(copy, 50);
    if (MDNS.begin(copy)) {
      Serial.println("[Soulmate-Wifi] mDNS: " + name + ".local");
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println(F("[Soulmate-Wifi] Error starting MDNS"));
    }
  }

  void stopMDNS() {
    MDNS.end();
  }

  bool credentialsPresent() {
    preferences.begin("Wifi", false);
    String ssid = preferences.getString("ssid", "");
    preferences.end();
    return !ssid.equals("");
  }

  // The single connect attempt. Runs as a short-lived task because WiFi.begin()
  // wants a delay after the preceding disconnect, and nothing that calls this
  // — a WiFi event, a websocket handler, a BLE write — can afford to block.
  void connectTask(void *parameter) {
    preferences.begin("Wifi", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");
    preferences.end();

    if (ssid.equals("")) {
      Serial.println(F("[Soulmate-Wifi] No saved credentials."));
      haveCredentials = false;
      enterState(IDLE);
      vTaskDelete(NULL);
    }

    // Internal DRAM, not total free heap: the WiFi stack's RX buffers have to
    // come from internal DRAM, and running it low is one of the ways a large
    // panel takes the network down with it. Logged at every attempt so a
    // reconnect loop in the field says whether that's what's happening.
    Serial.printf("[Soulmate-Wifi] Attempt %u -> \"%s\" (free internal DRAM "
                  "%u, largest block %u)\n",
                  static_cast<unsigned>(attemptCount), ssid.c_str(),
                  static_cast<unsigned>(
                      heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                  static_cast<unsigned>(
                      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));

    WiFi.disconnect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    WiFi.mode(WIFI_STA);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    WiFi.begin(ssid.c_str(), pass.c_str());

    vTaskDelete(NULL);
  }

  void beginAttempt() {
    attemptCount++;
    enterState(CONNECTING);

    // Checked, unlike the five xTaskCreate calls this replaces. If the stack
    // can't be allocated the attempt silently never happened, and without
    // dropping back to IDLE the supervisor would wait for the association
    // timeout before noticing.
    if (xTaskCreate(connectTask, "SoulmateConnect", 4096, NULL, 1, NULL) !=
        pdPASS) {
      Serial.println(F("[Soulmate-Wifi] Could not start connect task."));
      enterState(IDLE);
    }
  }

  // Reconnect supervisor. Runs from loop(), once a second.
  //
  // Reconnects used to be scheduled from inside the WiFi event handler, and
  // only for two of the ~20 disconnect reason codes. Everything else —
  // BEACON_TIMEOUT (200), HANDSHAKE_TIMEOUT (204), AUTH_FAIL (202),
  // ASSOC_FAIL (203), 4WAY_HANDSHAKE_TIMEOUT (15) — printed "spurious
  // disconnect" and gave up. And since isConnected was already false by the
  // time a retry failed, *every* failed retry landed in that branch. One
  // badly-timed drop parked the lamp offline until it was power-cycled, which
  // is the reason the app stops working and Bluetooth is the only way back in.
  //
  // Now: one owner, every reason code treated the same, and retries forever
  // with backoff.
  void superviseConnection() {
    if (!haveCredentials)
      return;

    uint32_t now = millis();

    if (state == UP) {
      // Trust the driver over our own bookkeeping. If the two ever disagree,
      // WiFi.status() is the one that decides whether packets flow.
      if (WiFi.status() == WL_CONNECTED)
        return;
      Serial.println(F("[Soulmate-Wifi] Marked up but not connected."));
      isConnected = false;
      enterState(IDLE);
      return;
    }

    if (state == CONNECTING) {
      if (now - stateEnteredAt < SOULMATE_WIFI_ASSOC_TIMEOUT_MS)
        return;
      Serial.println(F("[Soulmate-Wifi] Association timed out."));
      enterState(IDLE);
      return;
    }

    // IDLE. attemptCount is reset to 0 whenever a connection comes up or a
    // working one drops, so the first retry after a real drop is immediate and
    // only repeated failures back off.
    if (attemptCount > 0) {
      uint32_t shift = attemptCount - 1;
      if (shift > 4)
        shift = 4;
      uint32_t backoff = SOULMATE_WIFI_BACKOFF_MIN_MS << shift;
      if (backoff > SOULMATE_WIFI_BACKOFF_MAX_MS)
        backoff = SOULMATE_WIFI_BACKOFF_MAX_MS;
      if (now - stateEnteredAt < backoff)
        return;
    }

    beginAttempt();
  }

  // WiFi configuration
  void connectTo(const char *ssid, const char *pass) {
    Serial.println(F("[Soulmate-Wifi] Connecting to WiFi"));
    preferences.begin("Wifi", false);
    preferences.putString("ssid", String(ssid));
    preferences.putString("pass", String(pass));
    preferences.end();

    haveCredentials = true;
    attemptCount = 0;
    beginAttempt();
  }

  void disconnect() {
    Serial.println(F("[Soulmate-Wifi] Disconnect WIFI now"));
    preferences.begin("Wifi", false);
    preferences.remove("ssid");
    preferences.remove("pass");
    preferences.end();

    // Before WiFi.disconnect(), or the supervisor races the credential removal
    // and starts one more doomed attempt.
    haveCredentials = false;
    isConnected = false;
    enterState(IDLE);
    WiFi.disconnect();
  }

  void connectToSavedWifi() {
    Serial.println(F("[Soulmate-Wifi] connectToSavedWifi"));
    haveCredentials = credentialsPresent();
    if (!haveCredentials)
      return;
    isConnected = false;
    attemptCount = 0;
    beginAttempt();
  }

  void reconnect() {
    Serial.println(F("[Soulmate-Wifi] Reconnecting..."));
    isConnected = false;
    attemptCount = 0;
    enterState(IDLE);
    // Left to the supervisor rather than started here: reconnect() is reachable
    // from consumeJson(), which runs on the AsyncTCP and NimBLE tasks.
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

  // Everything that has to happen once we have an IP but must not happen on the
  // WiFi event task.
  //
  // WiFi.onEvent() handlers run on arduino-esp32's `network_event` task, which
  // has a 4096-byte stack and is the sole consumer of a 32-deep event queue
  // (WiFiGeneric.cpp:64,110). fetchTime() used to run there: a blocking
  // HTTPClient GET to worldtimeapi.org, five-second default timeout plus DNS,
  // with a DynamicJsonBuffer and a String payload on that 4 KB stack. While it
  // blocked, no other WiFi event was dispatched — including the DISCONNECTED
  // event the reconnect logic depends on — and because postToSysQueue() enqueues
  // with portMAX_DELAY, the IDF event loop task backed up behind it too, which
  // stalls DHCP and lwIP event handling.
  bool postConnectRunning = false;

  void postConnectTask(void *parameter) {
    startMDNS();

    long receivedSeconds = fetchTime();
    if (receivedSeconds > 0) {
      unsigned long currentSeconds = millis() / 1000;
      unsigned long startedSeconds = receivedSeconds - currentSeconds;
      Circadian::startTrackingTime(startedSeconds);
    }

    connectHomekit();

    postConnectRunning = false;
    vTaskDelete(NULL);
  }

  // Registered once per boot. addHandler() appends unconditionally, so doing
  // this on every GOT_IP grew the handler list by a node per reconnect, and
  // hap_accessory_register() registered a duplicate accessory each time while
  // leaking the previous one.
  bool serversStarted = false;

  void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
    case SYSTEM_EVENT_STA_START:
      Serial.println(F("[Soulmate-Wifi] WiFi client started"));
      // Both of these need the STA interface up, which is what this event
      // means. esp_wifi_set_max_tx_power() used to be called from setup(),
      // before esp_wifi_init() had run — so it returned ESP_ERR_WIFI_NOT_INIT
      // and did nothing, and the return value was discarded.
      if (!WiFi.setSleep(SOULMATE_WIFI_POWER_SAVE))
        Serial.println(F("[Soulmate-Wifi] Could not set power save mode"));
      if (!WiFi.setTxPower(SOULMATE_WIFI_TX_POWER))
        Serial.println(F("[Soulmate-Wifi] Could not set TX power"));
      break;

    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println(F("[Soulmate-Wifi] Connected to access point"));
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.print(F("[Soulmate-Wifi] Disconnected. Reason code: "));
      Serial.println(info.disconnected.reason);

      if (state == UP) {
        teardownHomekit();
        isConnected = false;
        // 0, so the supervisor retries immediately. A connection that worked
        // and then dropped deserves one fast attempt before any backoff.
        attemptCount = 0;
        enterState(IDLE);
      } else if (state == CONNECTING &&
                 millis() - stateEnteredAt > kSelfDisconnectWindowMs) {
        // A real association failure rather than the WiFi.disconnect() our own
        // connect task issues. Every reason code is handled the same way —
        // which is the whole point, since the old code retried on two of them
        // and printed "spurious disconnect" for the rest.
        enterState(IDLE);
      }
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      if (state == UP) {
        Serial.println(F("[Soulmate-Wifi] Spurious got IP event."));
        break;
      }

      isConnected = true;
      attemptCount = 0;
      enterState(UP);
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());

      if (!serversStarted) {
        serversStarted = true;
        ws.onEvent(onEvent);
        socketServer.addHandler(&ws);
        socketServer.begin();
        server.begin();
      }

      // mDNS, time fetch and HomeKit registration, off this task. See
      // postConnectTask().
      if (!postConnectRunning) {
        postConnectRunning = true;
        if (xTaskCreate(postConnectTask, "SoulmatePostConnect", 8192, NULL, 1,
                        NULL) != pdPASS) {
          Serial.println(F("[Soulmate-Wifi] Could not start post-connect "
                           "task."));
          postConnectRunning = false;
        }
      }
      break;

    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println(F("[Soulmate-Wifi] Lost IP address"));
      isConnected = false;
      attemptCount = 0;
      enterState(IDLE);
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

    // Render cost, kept out of /status so it can't crowd that payload's buffer.
    server.on("/frame", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, F("application/json"), Soulmate.frameStats());
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
            esp_bt_controller_disable();

            if (!Update.begin()) {
              Update.printError(Serial);
              restartRequired = true;
              Soulmate.stopped = false;
            }
          }

          if (!Update.hasError()) {
            if (Update.write(data, len) != len) {
              Update.printError(Serial);
              restartRequired = true;
              Soulmate.stopped = false;
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
            Soulmate.stopped = false;
          }
        });
  }

  void loop() {
    EVERY_N_SECONDS(1) {
      superviseConnection();
    }

    EVERY_N_SECONDS(1) {
      if (
        Soulmate.currentRoutine == -2 && !isStreaming()
      ) {
        Soulmate.currentRoutine = 0;
      }
    }

    EVERY_N_SECONDS(2) {
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
  SoulmateWifi::connectTo(ssid, pass);
}

bool SoulmateLibrary::wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String SoulmateLibrary::ip() {
  return WiFi.localIP().toString();
}

#endif // SOULMATE_SOULMATEWIFI_H_
