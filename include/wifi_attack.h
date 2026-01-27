/**
 * WiFi Attack Module - ESP32 Marauder
 * Scan APs + Deauth Attack
 */

#ifndef WIFI_ATTACK_H
#define WIFI_ATTACK_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

// Maximum number of APs to store
#define MAX_AP_COUNT 30

// Access Point info structure
struct APInfo {
    String ssid;
    uint8_t bssid[6];
    int32_t rssi;
    uint8_t channel;
    wifi_auth_mode_t encryption;
    bool selected;
};

// Initialize WiFi attack module
void wifi_attack_init();

// Scan functions
void wifi_scan_start();
void wifi_scan_stop();
int wifi_get_ap_count();
APInfo* wifi_get_ap(int index);
void wifi_list_aps();
void wifi_select_ap(int index);
void wifi_clear_selection();
APInfo* wifi_get_selected_ap();

// Deauth attack functions
void wifi_deauth_start();
void wifi_deauth_stop();
bool wifi_is_deauthing();

// Utility
String wifi_auth_mode_str(wifi_auth_mode_t mode);
String wifi_mac_to_string(uint8_t* mac);

#endif // WIFI_ATTACK_H
