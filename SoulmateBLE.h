// Copyright 2019 Soulmate Lighting, LLC

#ifndef BUILDER_LIBRARIES_SOULMATE_BLE_H_
#define BUILDER_LIBRARIES_SOULMATE_BLE_H_

#include "NimBLEDevice.h"

#include <string>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

namespace BLE {
  bool willNotify = false;
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    pServer->getAdvertising()->start();
  }

  void onDisconnect(BLEServer *pServer) {
    Serial.println(F("A device disconnected."));
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
  }

  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    String input = value.c_str();

    StaticJsonDocument<200> jsonBuffer;
    // JsonObject root = jsonBuffer.parseObject(input);
    deserializeJson(jsonBuffer, input);
    JsonObject root = doc.as<JsonObject>();
    Soulmate.consumeJson(root);

    BLE::willNotify = true;
  }
};

namespace BLE {
  BLECharacteristic *pCharacteristic;
  BLEService *pService;
  BLEServer *pServer;
  bool bluetoothBackupChecked = false;
  double check = millis();

  void start() {
    // esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    Serial.println(F("Start bluetooth"));

    // BLEDevice::setMTU(512);
    // String name = "Soulmate-" + String(random(255));
    // BLEDevice::init(name.c_str());

    Serial.println(F("Init Bluetooth device..."));
    // TODO: Should this be Soulmate.name?
    BLEDevice::init("Soulmate");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ |
                                 NIMBLE_PROPERTY::WRITE |
                                 NIMBLE_PROPERTY::WRITE_NR);

    pCharacteristic->createDescriptor(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR,
        25);

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->setValue(Soulmate.status(false).c_str());

    btStart();
    pService->start();

    pServer->getAdvertising()->setScanResponse(true);
    NimBLEAdvertisementData advertisementData;
    advertisementData.setName("Soulmate");
    pServer->getAdvertising()->setAdvertisementData(advertisementData);
    pServer->getAdvertising()->setMinPreferred(
        0x06); // functions that help with iPhone connections issue
    pServer->getAdvertising()->setMinPreferred(0x12);
    pServer->getAdvertising()->start();
  }

  void stop() {
    btStop();
    pServer->getAdvertising()->stop();
    // BLEDevice::deinit(true);
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  }

  void notify() {
    String status = Soulmate.status(false);
    pCharacteristic->setValue(status.c_str());
    pCharacteristic->notify();
  }

  void setup() {
    start();
  }

  long lastNotifiedAt = millis();
  int bluetoothCooloff = 50;
  void loop() {
    if (willNotify && millis() - lastNotifiedAt > bluetoothCooloff) {
      notify();
      willNotify = false;
      lastNotifiedAt = millis();
    }
  }
} // namespace BLE

void SoulmateLibrary::BluetoothLoop() {
  BLE::loop();
}

void SoulmateLibrary::BluetoothSetup() {
  BLE::setup();
}

void SoulmateLibrary::StartBluetooth() {
  BLE::start();
}

void SoulmateLibrary::StopBluetooth() {
  BLE::stop();
}

#endif
