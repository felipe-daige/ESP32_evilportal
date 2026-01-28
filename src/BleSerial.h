#ifndef BLE_SERIAL_H
#define BLE_SERIAL_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// Default BLE passkey (6 digits). Change if needed.
#define BLE_PASSKEY 123456

class BleSerial : public Stream {
public:
  BleSerial();
  void begin(const char *name);
  void end();
  bool connected();

  // Optional: set MTU (max chunk size) used for notifications
  void setMTU(uint16_t mtu);

  // Authorization state (set after successful BLE pairing/bonding)
  bool isAuthorized();
  void setAuthorized(bool v);

  // Stream implementation
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;

private:
  bool _connected;
  NimBLEServer *_server;
  NimBLEService *_service;
  NimBLECharacteristic *_txCharacteristic;
  NimBLECharacteristic *_rxCharacteristic;

  // Simple ring buffer or similar for RX
  std::string _rxBuffer;
  SemaphoreHandle_t _rxMutex;
  // TX protection and MTU-aware chunking
  SemaphoreHandle_t _txMutex;
  uint16_t _mtu;
  bool _authorized;

  friend class ServerCallbacks;
  friend class CharacteristicCallbacks;
};

#endif
