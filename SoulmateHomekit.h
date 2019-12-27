// TODO: Notify Bluetooth when value changes (without crashing)

#include "driver/gpio.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <string.h>

#include "./SoulmateMain.h"

#include "hap.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define TAG "SOULMATE"

#define SERIAL_NUMBER "12341234"
#define FIRMWARE_REVISION "0.1"

#define ACCESSORY_NAME "Soulmate"
#define MANUFACTURER_NAME "Soulmate Lighting, LLC"
#define MODEL_NAME "DEMO"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void *acc;
static SemaphoreHandle_t ev_mutex;

static int brightness = 10;
static int led = false;
static int hue = 128;
static int saturation = 128;

static bool _identifed = false;
static void *_on_handle;
static void *_brightness_handle;
static void *_hue_handle;
static void *_saturation_handle;

void *identify_read(void *arg) { return (void *)true; }

void *on_read(void *arg) {
  // printf("[MAIN] LED READ\n");
  return (void *)Soulmate.on;
}

// void update() {
//   Soulmate.updateWifiClients();
//   // BLE::willNotify = true;
// }

void on_write(void *arg, void *value, int len) {
  // printf("[MAIN] LED WRITE. %d\n", (int)value);
  led = (bool)value;
  Soulmate.on = (bool)value;
  if (_on_handle) hap_event_response(acc, _on_handle, (void *)led);
  return;
}

void on_notify(void *arg, void *ev_handle, bool enable) {
  _on_handle = enable ? ev_handle : NULL;
}

void *brightness_read(void *arg) {
  // printf("[MAIN] brightness READ\n");
  int brightness = (float)Soulmate.brightness / 255.0 * 100.0;
  Serial.println(brightness);
  return (void *)brightness;
}

void brightness_write(void *arg, void *value, int len) {
  // printf("[MAIN] brightness WRITE. %d\n", (int)value);
  int soulmateBrightness = (float)(int)value / 100.0 * 255;
  Soulmate.brightness = soulmateBrightness;
  if (_brightness_handle) hap_event_response(acc, _brightness_handle, (void *)brightness);
  return;
}

void brightness_notify(void *arg, void *ev_handle, bool enable) {
  _on_handle = enable ? ev_handle : NULL;
}

void *led_saturation_read(void *arg) {
  // printf("[MAIN] LED SATURATION READ\n");
  // int saturation = 100; // (float)Soulmate.saturation / 255.0 * 100.0;
  return (void *)saturation;
}

void led_saturation_write(void *arg, void *value, int len) {
  // printf("[MAIN] LED SATURATION WRITE. %d\n", (int)value);
  saturation = (int)value;
  Soulmate.saturation = (float)saturation / 100.0 * 255.0;
  // if (_saturation_handle) hap_event_response(acc, _saturation_handle, (void*)saturation);
}

void led_saturation_notify(void *arg, void *saturation_handle, bool enable) {
  if (enable) {
    Serial.println("led_saturation_notify on");
    _saturation_handle = saturation_handle;
  } else {
    Serial.println("led_saturation_notify off");
    _saturation_handle = NULL;
  }
}

void *led_hue_read(void *arg) {
  // printf("[MAIN] LED HUE READ\n");
  int hue = (float)Soulmate.hue / 255.0 * 360.0;
  return (void *)hue;
}

void led_hue_write(void *arg, void *value, int len) {
  // printf("[MAIN] LED HUE WRITE. %d\n", (int)value);
  hue = (int)value;
  Soulmate.hue = (float)hue / 360.0 * 255.0;
  Soulmate.currentRoutine = -1;
  // if (_hue_handle) hap_event_response(acc, _hue_handle, (void*)hue);
}

void led_hue_notify(void *arg, void *hue_handle, bool enable) {
  _hue_handle = enable ? hue_handle : NULL;
}

void hap_object_init(void *arg) {
  void *accessory_object = hap_accessory_add(acc);
  struct hap_characteristic cs[] = {
      {HAP_CHARACTER_IDENTIFY, (void *)true, NULL, identify_read, NULL, NULL},     {HAP_CHARACTER_MANUFACTURER, (void *)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_MODEL, (void *)MODEL_NAME, NULL, NULL, NULL, NULL},           {HAP_CHARACTER_NAME, (void *)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_SERIAL_NUMBER, (void *)"0123456789", NULL, NULL, NULL, NULL}, {HAP_CHARACTER_FIRMWARE_REVISION, (void *)"1.0", NULL, NULL, NULL, NULL},
  };
  hap_service_and_characteristics_add(acc, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

  struct hap_characteristic cc[] = {
      {HAP_CHARACTER_ON, (void *)led, NULL, on_read, on_write, on_notify},
      {HAP_CHARACTER_BRIGHTNESS, (void *)brightness, NULL, brightness_read, brightness_write, brightness_notify},
      {HAP_CHARACTER_HUE, (void *)hue, NULL, led_hue_read, led_hue_write, led_hue_notify},
      {HAP_CHARACTER_SATURATION, (void *)saturation, NULL, led_saturation_read, led_saturation_write, led_saturation_notify},
  };
  hap_service_and_characteristics_add(acc, accessory_object, HAP_SERVICE_LIGHTBULB, cc, ARRAY_SIZE(cc));
}
