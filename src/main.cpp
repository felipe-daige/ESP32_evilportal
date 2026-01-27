/**
 * ESP32 Marauder - Evil Portal + Deauth
 * Main entry point (Arduino Framework)
 */

#include <Arduino.h>
#include "wifi_attack.h"
#include "evil_portal.h"

// Version
#define VERSION "1.0.0"

// Button for manual trigger
#define BTN_TRIGGER GPIO_NUM_0

void print_banner() {
    Serial.println();
    Serial.println("  ███╗   ███╗ █████╗ ██████╗  █████╗ ██╗   ██╗██████╗ ███████╗██████╗ ");
    Serial.println("  ████╗ ████║██╔══██╗██╔══██╗██╔══██╗██║   ██║██╔══██╗██╔════╝██╔══██╗");
    Serial.println("  ██╔████╔██║███████║██████╔╝███████║██║   ██║██║  ██║█████╗  ██████╔╝");
    Serial.println("  ██║╚██╔╝██║██╔══██║██╔══██╗██╔══██║██║   ██║██║  ██║██╔══╝  ██╔══██╗");
    Serial.println("  ██║ ╚═╝ ██║██║  ██║██║  ██║██║  ██║╚██████╔╝██████╔╝███████╗██║  ██║");
    Serial.println("  ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝");
    Serial.println();
    Serial.printf("  Evil Portal + Deauth  |  v%s  |  ESP32-WROOM\n", VERSION);
    Serial.println("  Type 'help' for commands");
    Serial.println();
}

void print_help() {
    Serial.println();
    Serial.println("=== WiFi Scan & Attack ===");
    Serial.println("  scanap           Scan for access points");
    Serial.println("  listap           List found APs");
    Serial.println("  select <n>       Select AP by index");
    Serial.println("  deauth           Start deauth attack on selected AP");
    Serial.println("  stopscan         Stop current scan/attack");
    Serial.println();
    Serial.println("=== Evil Portal ===");
    Serial.println("  evilportal start <ssid>   Start portal with SSID");
    Serial.println("  evilportal stop           Stop portal");
    Serial.println("  evilportal status         Show portal status");
    Serial.println("  evilportal creds          Show captured credentials");
    Serial.println("  evilportal clear          Clear captured credentials");
    Serial.println("  evilportal templates      List available HTML templates");
    Serial.println("  evilportal template <n>   Set HTML template");
    Serial.println();
    Serial.println("=== System ===");
    Serial.println("  help             Show this menu");
    Serial.println("  reboot           Restart device");
    Serial.println();
}

// Parse command with arguments
void process_command(String input) {
    input.trim();
    if (input.length() == 0) return;

    // Split command and arguments
    int spaceIdx = input.indexOf(' ');
    String cmd = (spaceIdx > 0) ? input.substring(0, spaceIdx) : input;
    String args = (spaceIdx > 0) ? input.substring(spaceIdx + 1) : "";
    args.trim();

    cmd.toLowerCase();

    // === System Commands ===
    if (cmd == "help" || cmd == "?") {
        print_help();
    }
    else if (cmd == "reboot" || cmd == "restart") {
        Serial.println("[*] Rebooting...");
        delay(500);
        ESP.restart();
    }

    // === WiFi Scan Commands ===
    else if (cmd == "scanap") {
        wifi_scan_start();
    }
    else if (cmd == "listap" || cmd == "list") {
        wifi_list_aps();
    }
    else if (cmd == "select") {
        if (args.length() == 0) {
            Serial.println("[!] Usage: select <ap_index>");
            return;
        }
        int idx = args.toInt();
        wifi_select_ap(idx);
    }

    // === Deauth Commands ===
    else if (cmd == "deauth") {
        wifi_deauth_start();
    }
    else if (cmd == "stopscan" || cmd == "stop") {
        wifi_deauth_stop();
    }

    // === Evil Portal Commands ===
    else if (cmd == "evilportal" || cmd == "ep") {
        // Parse subcommand
        int subSpaceIdx = args.indexOf(' ');
        String subCmd = (subSpaceIdx > 0) ? args.substring(0, subSpaceIdx) : args;
        String subArgs = (subSpaceIdx > 0) ? args.substring(subSpaceIdx + 1) : "";
        subArgs.trim();
        subCmd.toLowerCase();

        if (subCmd == "start") {
            if (subArgs.length() == 0) {
                // Try to use selected AP name
                APInfo* ap = wifi_get_selected_ap();
                if (ap != NULL) {
                    evil_portal_start(ap->ssid.c_str());
                } else {
                    Serial.println("[!] Usage: evilportal start <ssid>");
                    Serial.println("[!] Or select an AP first with 'select <n>'");
                }
            } else {
                // Remove quotes if present
                if (subArgs.startsWith("\"") && subArgs.endsWith("\"")) {
                    subArgs = subArgs.substring(1, subArgs.length() - 1);
                }
                evil_portal_start(subArgs.c_str());
            }
        }
        else if (subCmd == "stop") {
            evil_portal_stop();
        }
        else if (subCmd == "status") {
            evil_portal_status();
        }
        else if (subCmd == "creds" || subCmd == "credentials") {
            evil_portal_list_creds();
        }
        else if (subCmd == "clear") {
            evil_portal_clear_creds();
        }
        else if (subCmd == "templates" || subCmd == "list") {
            evil_portal_list_templates();
        }
        else if (subCmd == "template") {
            if (subArgs.length() == 0) {
                Serial.println("[!] Usage: evilportal template <filename>");
            } else {
                evil_portal_set_template(subArgs.c_str());
            }
        }
        else {
            Serial.println("[!] Unknown subcommand. Use 'help' for commands.");
        }
    }

    // === Quick Attack Combo ===
    else if (cmd == "attack") {
        // Quick attack: start deauth + evil portal together
        APInfo* ap = wifi_get_selected_ap();
        if (ap == NULL) {
            Serial.println("[!] No AP selected. Run 'scanap', 'listap', then 'select <n>'");
            return;
        }

        Serial.printf("[*] Starting combo attack on: %s\n", ap->ssid.c_str());

        // Start evil portal with same SSID
        evil_portal_start(ap->ssid.c_str());

        // Note: Deauth requires switching channel, which may conflict with AP mode
        // For now, just start the portal. Deauth needs to be run separately.
        Serial.println("[*] Portal started. Run 'deauth' in a separate session for full effect.");
    }

    // === Unknown Command ===
    else {
        Serial.printf("[!] Unknown command: %s\n", cmd.c_str());
        Serial.println("[!] Type 'help' for available commands");
    }

    Serial.print("\n> ");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Configure trigger button
    pinMode(BTN_TRIGGER, INPUT_PULLUP);

    // Initialize modules
    wifi_attack_init();
    evil_portal_init();

    // Print banner
    print_banner();
    Serial.print("> ");
}

void loop() {
    // Process serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        process_command(cmd);
    }

    // Process Evil Portal DNS
    evil_portal_loop();

    // Button trigger (GPIO0) - Quick demo
    static bool lastBtnState = HIGH;
    bool btnState = digitalRead(BTN_TRIGGER);

    if (btnState == LOW && lastBtnState == HIGH) {
        delay(50); // Debounce
        if (digitalRead(BTN_TRIGGER) == LOW) {
            Serial.println("\n[*] Button pressed - Quick Status:");
            if (evil_portal_is_running()) {
                evil_portal_status();
            } else if (wifi_is_deauthing()) {
                Serial.println("[*] Deauth attack in progress...");
            } else {
                Serial.println("[*] Idle. Type 'help' for commands.");
            }
            Serial.print("\n> ");
        }
    }
    lastBtnState = btnState;

    delay(10);
}
