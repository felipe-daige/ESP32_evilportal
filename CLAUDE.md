# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ESP32 Marauder** - Evil Portal + Deauth attack platform using PlatformIO with Arduino framework.

### Hardware Context
- **MCU**: ESP32-WROOM DevKit (4MB Flash)
- **Periféricos**: Botão GPIO0 (trigger), Serial CLI
- **Storage**: LittleFS (templates HTML na flash interna)
- **Expansão Futura**: SD Card, CC1101 (Sub-GHz), PN532 (NFC)

### Current Features
- **Evil Portal**: Captive portal com captura de credenciais
- **Deauth Attack**: Desconecta clientes de APs alvo
- **WiFi Scanner**: Escaneia e lista Access Points
- **Headless Mode**: Controle via Serial CLI (115200 baud)

## Build Commands

```bash
# Build firmware
pio run -e marauder

# Upload firmware to ESP32
pio run -e marauder --target upload

# Upload HTML templates (LittleFS)
pio run -e marauder --target uploadfs

# Monitor serial output
pio device monitor

# Full deploy (firmware + templates + monitor)
pio run -e marauder --target upload && pio run -e marauder --target uploadfs && pio device monitor

# Clean build
pio run --target clean
```

## Project Structure

```
/Marauder
├── platformio.ini          # Build configuration
├── include/
│   ├── evil_portal.h       # Evil Portal header
│   └── wifi_attack.h       # WiFi Attack header
├── src/
│   ├── main.cpp            # Entry point + CLI
│   ├── evil_portal.cpp     # Captive portal implementation
│   └── wifi_attack.cpp     # WiFi scan + deauth
├── data/                   # LittleFS templates
│   ├── index.html          # Default login page
│   ├── portal_google.html  # Google-style portal
│   └── portal_facebook.html # Facebook-style portal
└── CLAUDE.md
```

## CLI Commands

### WiFi Scan & Attack
```
scanap              Escaneia Access Points próximos
listap              Lista APs encontrados
select <n>          Seleciona AP por índice
deauth              Inicia ataque deauth no AP selecionado
stopscan            Para scan/ataque atual
```

### Evil Portal
```
evilportal start <ssid>     Inicia portal com SSID
evilportal start            Usa SSID do AP selecionado
evilportal stop             Para o portal
evilportal status           Status do portal
evilportal creds            Lista credenciais capturadas
evilportal clear            Limpa credenciais
evilportal templates        Lista templates HTML disponíveis
evilportal template <file>  Define template HTML
```

### Sistema
```
help                Menu de ajuda
reboot              Reinicia o dispositivo
```

## Fluxo de Ataque Típico

### Evil Portal Simples
```
> evilportal start "Free WiFi"
> evilportal creds
```

### Evil Portal + Deauth (Mais Efetivo)
```
> scanap                     # Escaneia redes
> listap                     # Lista APs
> select 0                   # Seleciona alvo (ex: "CafeWiFi")
> evilportal start           # Cria AP clone
> deauth                     # Desconecta vítimas da rede real
> evilportal creds           # Verifica credenciais capturadas
> stopscan                   # Para deauth
```

## Technical Notes

### Dependencies (platformio.ini)
- ESPAsyncWebServer @ ^1.2.3
- AsyncTCP @ ^1.1.1
- ArduinoJson @ ^7.0.0
- LittleFS (built-in)
- DNSServer (built-in)

### Captive Portal Detection
O DNS Server redireciona TODAS as queries para o IP do ESP32, forçando o popup de captive portal em:
- iOS: `captive.apple.com`
- Android: `connectivitycheck.gstatic.com`
- Windows: `www.msftconnecttest.com`
- Firefox: `detectportal.firefox.com`

### Memory Usage
- RAM: ~15% (48KB / 320KB)
- Flash: ~44% (855KB / 1.9MB)
- Partition: min_spiffs (~190KB para templates)

## Reference Repositories

- [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) - Referência principal
- [Bruce Firmware](https://github.com/BruceDevices/firmware) - Features adicionais
- [M5Launcher](https://github.com/bmorcelli/Launcher) - Multi-firmware bootloader
