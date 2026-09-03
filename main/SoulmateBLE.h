// Copyright 2019 Soulmate Lighting, LLC

#ifndef SOULMATE_BLE_H_
#define SOULMATE_BLE_H_

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

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(input);
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
  // NimBLECharacteristic's destructor deletes its descriptors but NOT its
  // callbacks object, so this has to be owned here and freed in stop(). It never
  // mattered before, because nothing ever tore BLE down.
  MyCallbacks *pCharacteristicCallbacks = nullptr;
  // Set by stop() once teardown has been attempted; anything holding NimBLE
  // pointers must check it before running.
  bool released = false;
  // Whether that teardown actually left the controller down. Cached so repeat
  // calls to stop() answer honestly instead of returning a bare true.
  bool controllerDown = false;

  void start() {
    // After stop() the controller memory is released and cannot be reclaimed
    // without a reboot, so re-entering here would build a server on a controller
    // that can never come back. StartBluetooth() is public and has no in-tree
    // caller after setup(), but that is a fact about today's call sites, not a
    // guarantee.
    if (released) {
      Serial.println(F("[Soulmate-BLE] Not restarting: released this boot"));
      return;
    }

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

    pCharacteristicCallbacks = new MyCallbacks();
    pCharacteristic->setCallbacks(pCharacteristicCallbacks);
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

  // Full teardown, host first. Returns true if the controller actually went down.
  //
  // The ordering matters and is easy to get wrong: btStop() only disables and
  // deinitialises the CONTROLLER, so calling it directly leaves NimBLE's host
  // task and queues running on top of a controller that no longer exists — and
  // releasing controller memory underneath a live host is worse still.
  // NimBLEDevice::deinit() is the supported sequence: nimble_port_stop(), then
  // nimble_port_deinit(), then esp_nimble_hci_and_controller_deinit(). Passing
  // clearAll also deletes the server and advertising objects (and, through the
  // server, its services and characteristics), which is why the pointers below
  // are nulled rather than left dangling.
  //
  // Note that deinit does all of that ONLY if nimble_port_stop() succeeds; if it
  // refuses, nothing is stopped, deinitialised or deleted. That is why the return
  // value here is the controller's actual status rather than an assumption, and
  // why `released` is latched either way — see below.
  //
  // (Upstream already had `// BLEDevice::deinit(true);` here, commented out.)
  bool stop() {
    if (released)
      return controllerDown;

    // Set before tearing anything down: loop() runs on loopTask and notify()
    // dereferences pCharacteristic, which deinit(true) is about to delete. A BLE
    // write landing just before this leaves willNotify true, and the next loop()
    // would otherwise notify through freed memory.
    //
    // Deliberately never cleared. If the teardown below fails partway we cannot
    // tell whether the objects were freed, so BLE stays out of service for this
    // boot rather than being handed back in an unknown state. A power cycle
    // restores it. That is a real (if rare) cost: a panel whose teardown fails
    // keeps modem sleep AND loses BLE until it is restarted.
    released = true;

    NimBLEDevice::deinit(true);

    pServer = nullptr;
    pService = nullptr;
    pCharacteristic = nullptr;

    // Safe here and not before: deinit(true) has already destroyed the
    // characteristic that referenced this, and the host is stopped, so nothing
    // can call into it.
    delete pCharacteristicCallbacks;
    pCharacteristicCallbacks = nullptr;

    controllerDown =
        esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE;
    if (!controllerDown)
      return false;

    // Only valid with the controller IDLE. Irreversible until reboot, which is
    // the point: a power cycle is how BLE — and therefore provisioning — returns.
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    return true;
  }

  void notify() {
    String status = Soulmate.status(false);
    const char* data = status.c_str();
    pCharacteristic->setValue((unsigned char *)data, status.length());
    pCharacteristic->notify();
  }

  void setup() {
    start();
  }

  long lastNotifiedAt = millis();
  int bluetoothCooloff = 50;
  void loop() {
    if (released)
      return;
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

bool SoulmateLibrary::StopBluetooth() {
  return BLE::stop();
}

#endif
