#include "BleSerial.h"

// Nordic UART Service UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class ServerCallbacks : public NimBLEServerCallbacks {
  BleSerial *_pBleSerial;

public:
  ServerCallbacks(BleSerial *s) : _pBleSerial(s) {}
  // Called when a peer connects (basic)
  void onConnect(NimBLEServer *pServer) override {
    _pBleSerial->_connected = true;
  }
  // Called with connection descriptor - use to detect MTU and start security
  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override {
    _pBleSerial->_connected = true;
    // Get peer MTU (server provides helper)
    uint16_t mtu = pServer->getPeerMTU(desc->conn_handle);
    // Characteristic notifications use ATT payload = MTU - 3
    if (mtu > 3)
      _pBleSerial->setMTU(mtu - 3);
    // Start security (pairing) for this connection
    NimBLEDevice::startSecurity(desc->conn_handle);
  }
  void onDisconnect(NimBLEServer *pServer) override {
    _pBleSerial->_connected = false;
    _pBleSerial->setAuthorized(false);
    // Advertise again so others can connect (or re-connect)
    pServer->getAdvertising()->start();
  }
};

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  BleSerial *_pBleSerial;

public:
  CharacteristicCallbacks(BleSerial *s) : _pBleSerial(s) {}
  void onWrite(NimBLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      xSemaphoreTake(_pBleSerial->_rxMutex, portMAX_DELAY);
      _pBleSerial->_rxBuffer += value;
      xSemaphoreGive(_pBleSerial->_rxMutex);
    }
  }
};

// Simple Security Callbacks to update authorized state
class SimpleSecurityCallbacks : public NimBLESecurityCallbacks {
  BleSerial *_pBleSerial;

public:
  SimpleSecurityCallbacks(BleSerial *s) : _pBleSerial(s) {}
  uint32_t onPassKeyRequest() override {
    return NimBLEDevice::getSecurityPasskey();
  }
  void onPassKeyNotify(uint32_t pass_key) override {}
  bool onSecurityRequest() override { return true; }
  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    if (desc->sec_state.encrypted) {
      _pBleSerial->setAuthorized(true);
      Serial.println("[BLE] Peer authenticated and link encrypted");
    }
  }
  bool onConfirmPIN(uint32_t pin) override { return true; }
};

BleSerial::BleSerial()
    : _connected(false), _server(NULL), _txCharacteristic(NULL),
      _rxCharacteristic(NULL), _mtu(128), _authorized(false) {
  _rxMutex = xSemaphoreCreateMutex();
  _txMutex = xSemaphoreCreateMutex();
}

void BleSerial::begin(const char *name) {
  NimBLEDevice::init(name);

  // Configure security: require bonding + MITM protection
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(ESP_IO_CAP_IO);
  // Force a known passkey to simplify pairing workflows
  NimBLEDevice::setSecurityPasskey(BLE_PASSKEY);
  NimBLEDevice::setSecurityCallbacks(new SimpleSecurityCallbacks(this));

  // Create Server
  _server = NimBLEDevice::createServer();
  _server->setCallbacks(new ServerCallbacks(this));

  // Create Service
  _service = _server->createService(SERVICE_UUID);

  // Create RX Characteristic (Phone -> ESP32)
  _rxCharacteristic = _service->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  _rxCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

  // Create TX Characteristic (ESP32 -> Phone)
  _txCharacteristic = _service->createCharacteristic(
      CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  // Start Service
  _service->start();

  // Request ATT MTU negotiation (ask for 247 by default)
  NimBLEDevice::setMTU(247);

  // Optimize for iOS
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max Power

  // Start Advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  // Critical for iOS visibility but relaxed for WiFi coexistence
  pAdvertising->setScanResponse(true);

  // Set Advertising Interval: 800ms (0x500) to 1000ms (0x640) * 0.625ms
  // Very relaxed to allow WiFi functioning
  pAdvertising->setMinInterval(0x500);
  pAdvertising->setMaxInterval(0x640);

  // Preferred Connection Interval: Relaxed
  pAdvertising->setMinPreferred(0x50);
  pAdvertising->setMaxPreferred(0x100);

  pAdvertising->start();
}

void BleSerial::end() {
  if (_server) {
    NimBLEDevice::deinit(true);
    _server = NULL;
    _connected = false;
  }
}

bool BleSerial::connected() { return _connected; }

size_t BleSerial::write(uint8_t c) { return write(&c, 1); }

size_t BleSerial::write(const uint8_t *buffer, size_t size) {
  if (!_connected || !_txCharacteristic)
    return 0;
  // Send in MTU-sized chunks to avoid exceeding peer limits and to be
  // resilient across different clients. Protect against concurrent
  // notifications from multiple tasks using _txMutex.
  if (_mtu == 0)
    _mtu = 128;

  xSemaphoreTake(_txMutex, portMAX_DELAY);
  size_t sent = 0;
  while (sent < size) {
    size_t remain = size - sent;
    size_t toSend = (remain > _mtu) ? _mtu : remain;
    _txCharacteristic->setValue(buffer + sent, toSend);
    _txCharacteristic->notify();
    sent += toSend;
    // Yield a tick to allow BLE stack to process notifications
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  xSemaphoreGive(_txMutex);
  return size;
}

void BleSerial::setMTU(uint16_t mtu) { _mtu = mtu; }

bool BleSerial::isAuthorized() { return _authorized; }

void BleSerial::setAuthorized(bool v) { _authorized = v; }

int BleSerial::available() {
  xSemaphoreTake(_rxMutex, portMAX_DELAY);
  int len = _rxBuffer.length();
  xSemaphoreGive(_rxMutex);
  return len;
}

int BleSerial::read() {
  xSemaphoreTake(_rxMutex, portMAX_DELAY);
  if (_rxBuffer.length() == 0) {
    xSemaphoreGive(_rxMutex);
    return -1;
  }
  char c = _rxBuffer[0];
  _rxBuffer.erase(0, 1);
  xSemaphoreGive(_rxMutex);
  return (uint8_t)c;
}

int BleSerial::peek() {
  xSemaphoreTake(_rxMutex, portMAX_DELAY);
  if (_rxBuffer.length() == 0) {
    xSemaphoreGive(_rxMutex);
    return -1;
  }
  int c = (uint8_t)_rxBuffer[0];
  xSemaphoreGive(_rxMutex);
  return c;
}

void BleSerial::flush() {
  // No specific flush needed for NimBLE notify, it sends immediately
}
