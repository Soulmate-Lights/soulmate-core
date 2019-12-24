#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "./SoulmateMain.h"

#include "hap.h"

#include "rom/ets_sys.h"
#include "driver/gpio.h"

#define TAG "SOULMATE"

#define SERIAL_NUMBER "12341234"
#define FIRMWARE_REVISION "0.1"

#define ACCESSORY_NAME  "Soulmate"
#define MANUFACTURER_NAME   "Soulmate Lighting, LLC"
#define MODEL_NAME  "DEMO"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define EXAMPLE_ESP_WIFI_SSID "MyCharterWiFi56-2G"
#define EXAMPLE_ESP_WIFI_PASS "festivegadfly738"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void* acc;
static SemaphoreHandle_t ev_mutex;
static int brightness = 10;
static void* _brightness_ev_handle = NULL;

void temperature_humidity_monitoring_task(void* arm) {
    while (1) {
        ESP_LOGI("MAIN", "RAM LEFT %d", esp_get_free_heap_size());
        ESP_LOGI("MAIN", "TASK STACK : %d", uxTaskGetStackHighWaterMark(NULL));

        if (_brightness_ev_handle) {
            ESP_LOGI("MAIN", "HAP Event response");
            hap_event_response(acc, _brightness_ev_handle, (void*)brightness);
        }

        vTaskDelay( 3000 / portTICK_RATE_MS );
    }
}

static void* _brightness_read(void* arg) {
    ESP_LOGI("MAIN", "_brightness_read");
    return (void*)brightness;
}

void _brightness_notify(void* arg, void* ev_handle, bool enable) {
    ESP_LOGI("MAIN", "_brightness_notify");
    if (enable) {
        _brightness_ev_handle = ev_handle;
    } else {
        _brightness_ev_handle = NULL;
    }
}

static bool _identifed = false;
void* identify_read(void* arg) {
    return (void*)true;
}

static void* _ev_handle;
static int led = false;

void* led_read(void* arg) {
  printf("[MAIN] LED READ\n");
  return (void*)Soulmate.on;
}

void led_write(void* arg, void* value, int len) {
  printf("[MAIN] LED WRITE. %d\n", (int)value);

  led = (int)value;
  if (value) {
      led = true;
      Soulmate.on = true;
  }
  else {
      led = false;
      Soulmate.on = false;
  }

  if (_ev_handle)
      hap_event_response(acc, _ev_handle, (void*)led);

  return;
}

void led_notify(void* arg, void* ev_handle, bool enable) {
  if (enable) {
      _ev_handle = ev_handle;
  }
  else {
      _ev_handle = NULL;
  }
}

void hap_object_init(void* arg) {
  void* accessory_object = hap_accessory_add(acc);
  struct hap_characteristic cs[] = {
      {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
      {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
      {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.0", NULL, NULL, NULL, NULL},
  };
  hap_service_and_characteristics_add(acc, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

  struct hap_characteristic cc[] = {
      {HAP_CHARACTER_ON, (void*)led, NULL, led_read, led_write, led_notify},
  };
  hap_service_and_characteristics_add(acc, accessory_object, HAP_SERVICE_SWITCHS, cc, ARRAY_SIZE(cc));

  // struct hap_characteristic brightness_sensor[] = {
  //     {HAP_CHARACTER_CURRENT_RELATIVE_HUMIDITY, (void*)brightness, NULL, _brightness_read, NULL, _brightness_notify},
  //     {HAP_CHARACTER_NAME, (void*)"Soulmate", NULL, NULL, NULL, NULL},
  // };
  // hap_service_and_characteristics_add(acc, accessory_object, HAP_SERVICE_LIGHTBULB, brightness_sensor, ARRAY_SIZE(brightness_sensor));
}



// static esp_err_t event_handler(void *ctx, system_event_t *event)
// {
//     switch(event->event_id) {
//     case SYSTEM_EVENT_STA_START:
//         esp_wifi_connect();
//         break;
//     case SYSTEM_EVENT_STA_GOT_IP:
//         ESP_LOGI(TAG, "got ip:%s",
//                  ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
//         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//         {
//             hap_init();

//             uint8_t mac[6];
//             esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
//             char accessory_id[32] = {0,};
//             sprintf(accessory_id, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//             hap_accessory_callback_t callback;
//             callback.hap_object_init = hap_object_init;
//             acc = hap_accessory_register(
//                 (char*)ACCESSORY_NAME,
//                 accessory_id,
//                 (char*)"111-11-111",
//                 (char*)MANUFACTURER_NAME,
//                 HAP_ACCESSORY_CATEGORY_OTHER,
//                 811,
//                 1,
//                 NULL,
//                 &callback
//             );
//         }
//         break;
//     case SYSTEM_EVENT_STA_DISCONNECTED:
//         esp_wifi_connect();
//         xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
//         break;
//     default:
//         break;
//     }
//     return ESP_OK;
// }

// void wifi_init_sta()
// {
//     wifi_event_group = xEventGroupCreate();

//     tcpip_adapter_init();
//     ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     wifi_config_t wifi_config;
//     memset(&wifi_config, 0, sizeof(wifi_config));
//     memcpy(wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID, strlen(EXAMPLE_ESP_WIFI_SSID));
//     memcpy(wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS, strlen(EXAMPLE_ESP_WIFI_PASS));

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
//     ESP_ERROR_CHECK(esp_wifi_start() );

//     ESP_LOGI(TAG, "wifi_init_sta finished.");
//     ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
//              EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
// }
