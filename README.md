# ESP32 Evil Portal 🎭

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![Version](https://img.shields.io/badge/Version-1.0.0-blue)](https://github.com/)
[![License](https://img.shields.io/badge/License-Educational-red)](LICENSE)

Um **Captive Portal** para ESP32 com funcionalidades de ataque WiFi para fins educacionais e testes de segurança autorizados.

---

## ⚠️ Aviso Legal

> **IMPORTANTE**: Este projeto é destinado **EXCLUSIVAMENTE** para fins educacionais e testes de segurança em redes de sua propriedade ou com autorização expressa do proprietário.
>
> O uso indevido desta ferramenta pode violar leis locais e federais. O autor não se responsabiliza pelo uso inadequado.

---

## 📋 Índice

- [Sobre o Projeto](#sobre-o-projeto)
- [Funcionalidades](#funcionalidades)
- [Hardware Necessário](#hardware-necessário)
- [Diagrama de Conexões](#diagrama-de-conexões)
- [Instalação](#instalação)
- [Configuração](#configuração)
- [Comandos](#comandos)
- [Uso](#uso)
- [Estrutura do Projeto](#estrutura-do-projeto)
- [Personalização](#personalização)
- [Troubleshooting](#troubleshooting)
- [Changelog](#changelog)

---

## 📖 Sobre o Projeto

O **ESP32 Evil Portal** é uma ferramenta de pentesting WiFi que cria um ponto de acesso falso (Rogue AP) com um portal cativo personalizado. Quando usuários se conectam, são redirecionados para uma página de login onde suas credenciais podem ser capturadas.

### Principais Características:
- Portal cativo compatível com **Android**, **iOS** e **Windows**
- Interface web personalizada (tema UNIGRAN Educacional)
- Máscaras de entrada para **RGM**, **CPF** e **Data de Nascimento**
- Varredura de redes WiFi próximas
- Ataque de desautenticação (Deauth)
- Interface de comando via Serial
- LED de status para indicação visual

---

## 🚀 Funcionalidades

### Evil Portal
- ✅ Criação de AP falso com SSID personalizado
- ✅ DNS Spoofing para redirecionamento de tráfego
- ✅ Compatibilidade com detecção de Captive Portal (Android/iOS/Windows)
- ✅ Captura e armazenamento de credenciais
- ✅ Template HTML personalizável
- ✅ Máscaras de input para formatação automática

### WiFi Attack
- ✅ Varredura de Access Points próximos
- ✅ Listagem com RSSI, canal e tipo de criptografia
- ✅ Seleção de AP alvo
- ✅ Ataque de desautenticação (Deauth)

### Sistema
- ✅ Interface de comando via Serial Monitor
- ✅ LED de status (GPIO 32)
- ✅ Botão de trigger (GPIO 0 - BOOT)
- ✅ Reinicialização remota

---

## 🔧 Hardware Necessário

### Componentes
| Componente | Quantidade | Descrição |
|------------|------------|-----------|
| ESP32 DevKit V1 WROOM | 1 | Microcontrolador principal |
| LED (opcional) | 1 | Indicador de status (usar GPIO 32) |
| Resistor 220Ω (opcional) | 1 | Para o LED externo |
| Cabo USB | 1 | Alimentação e programação |

### ESP32 DevKit V1 WROOM

```
                    ┌──────────────────┐
                    │                  │
              EN ─○─┤                  ├─○─ VIN (5V)
         GPIO 36 ─○─┤                  ├─○─ GND
         GPIO 39 ─○─┤                  ├─○─ GPIO 23
         GPIO 34 ─○─┤                  ├─○─ GPIO 22
         GPIO 35 ─○─┤   ESP32-WROOM    ├─○─ TX (GPIO 1)
 LED ─── GPIO 32 ─○─┤                  ├─○─ RX (GPIO 3)
         GPIO 33 ─○─┤                  ├─○─ GPIO 21
         GPIO 25 ─○─┤                  ├─○─ GND
         GPIO 26 ─○─┤                  ├─○─ GPIO 19
         GPIO 27 ─○─┤                  ├─○─ GPIO 18
         GPIO 14 ─○─┤                  ├─○─ GPIO 5
         GPIO 12 ─○─┤                  ├─○─ GPIO 17
             GND ─○─┤                  ├─○─ GPIO 16
         GPIO 13 ─○─┤                  ├─○─ GPIO 4
              D2 ─○─┤                  ├─○─ GPIO 0 ─── BOOT Button
              D3 ─○─┤                  ├─○─ GPIO 2
             CMD ─○─┤                  ├─○─ GPIO 15
             CLK ─○─┤                  ├─○─ D1
                    │        USB       │
                    └────────┬─┬───────┘
                             │ │
```

**Pinout Utilizado:**
- **GPIO 32**: LED de status (HIGH = credenciais capturadas)
- **GPIO 0**: Botão BOOT (status rápido ao pressionar)

---

## 📦 Instalação

### Pré-requisitos
- [PlatformIO](https://platformio.org/) (VS Code Extension ou CLI)
- Python 3.x (para PlatformIO)
- Driver USB CP2102/CH340 (dependendo do seu ESP32)

### Passo a Passo

1. **Clone o repositório:**
```bash
git clone https://github.com/felipe-daige/ESP32_evilportal.git
cd ESP32_evilportal
```

2. **Abra no VS Code com PlatformIO:**
```bash
code .
```

3. **Compile o projeto:**
```bash
pio run
```

4. **Faça o upload para o ESP32:**
```bash
pio run -t upload
```

5. **Abra o Serial Monitor (115200 baud):**
```bash
pio device monitor
```

---

## ⚙️ Configuração

### platformio.ini

O projeto já vem configurado para ESP32-WROOM DevKit:

```ini
[env:marauder]
platform = espressif32
framework = arduino
board = esp32dev
board_build.partitions = min_spiffs.csv
monitor_speed = 115200
upload_speed = 921600

lib_deps =
    me-no-dev/ESPAsyncWebServer @ ^1.2.3
    me-no-dev/AsyncTCP @ ^1.1.1
    bblanchon/ArduinoJson @ ^7.0.0
```

### Ambiente de Debug

Para debug verbose, use o ambiente `marauder-debug`:
```bash
pio run -e marauder-debug -t upload
```

---

## 📟 Comandos

### WiFi Scan & Attack

| Comando | Descrição |
|---------|-----------|
| `scanap` | Escaneia access points próximos |
| `listap` | Lista APs encontrados |
| `select <n>` | Seleciona AP pelo índice |
| `deauth` | Inicia ataque deauth no AP selecionado |
| `stopscan` | Para scan/ataque atual |

### Evil Portal

| Comando | Descrição |
|---------|-----------|
| `evilportal start <ssid>` | Inicia portal com SSID especificado |
| `evilportal stop` | Para o portal |
| `evilportal status` | Mostra status do portal |
| `evilportal creds` | Lista credenciais capturadas |
| `evilportal clear` | Limpa credenciais |
| `evilportal templates` | Lista templates HTML |
| `evilportal template <n>` | Define template HTML |

### Sistema

| Comando | Descrição |
|---------|-----------|
| `help` | Mostra menu de ajuda |
| `reboot` | Reinicia o dispositivo |
| `attack` | Combo rápido: Portal + info de Deauth |

### Atalhos

- `ep` = `evilportal`
- `?` = `help`
- `list` = `listap`
- `stop` = `stopscan`

---

## 📱 Uso

### Exemplo de Fluxo Completo

```bash
# 1. Iniciar o portal com um SSID personalizado
> evilportal start "WiFi_Gratis"

[Portal] AP IP: 172.0.0.1
[Portal] ATIVO: WiFi_Gratis (IP: 172.0.0.1)
[Portal] Aguardando conexoes...

# 2. Aguardar conexões e capturas...
# (Quando um usuário conectar e submeter o formulário)

========== CAPTURA ==========
RGM:  123.45678
CPF:  123.456.789-00
NASC: 01/01/2000
PASS: senha123
==============================

[Portal] >>> Credencial #1 SALVA! <<<

# 3. Listar credenciais capturadas
> evilportal creds

[0] RGM:123.45678 CPF:123.456.789-00 NASC:01/01/2000 | senha123 | IP:172.0.0.2

# 4. Parar o portal
> evilportal stop

[Portal] Parado
```

### Workflow de Ataque Avançado

```bash
# 1. Escanear redes próximas
> scanap

[WiFi] Scanning...
[WiFi] Found 5 AP(s)

# 2. Listar redes encontradas
> listap

[0] UNIGRAN_NET      CH:6  RSSI:-45  WPA2
[1] Vizinho_5G       CH:36 RSSI:-72  WPA2
[2] Cafe_Wifi        CH:1  RSSI:-65  OPEN

# 3. Selecionar alvo
> select 0

[WiFi] Selected: UNIGRAN_NET

# 4. Iniciar portal clonando o SSID
> evilportal start

[Portal] ATIVO: UNIGRAN_NET (IP: 172.0.0.1)
```

---

## 📁 Estrutura do Projeto

```
ESP32_evilportal/
├── include/
│   ├── evil_portal.h      # Header do Evil Portal
│   ├── wifi_attack.h      # Header do módulo WiFi Attack
│   └── README             # Documentação headers
├── src/
│   ├── main.cpp           # Ponto de entrada principal
│   ├── evil_portal.cpp    # Implementação do Captive Portal
│   └── wifi_attack.cpp    # Implementação do scanner/deauth
├── data/
│   └── index.html         # Template HTML (opcional)
├── test/
│   └── README             # Documentação de testes
├── platformio.ini         # Configuração PlatformIO
├── GUIA_USO.md            # Guia rápido de uso
└── README.md              # Este arquivo
```

---

## 🎨 Personalização

### Template HTML

O template atual está embutido em `src/evil_portal.cpp` na constante `PAGINA`. Para personalizar:

1. Edite o HTML dentro da string raw literal:
```cpp
const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
...
</html>
)rawliteral";
```

2. O formulário deve usar `action="/login"` e `method="get"`

3. Campos esperados:
   - `name="rgm"` - Registro Geral de Matrícula
   - `name="cpf"` - CPF
   - `name="nasc"` - Data de Nascimento
   - `name="pass"` - Senha

### Máscaras de Input

As máscaras JavaScript aplicam formatação automática:

- **RGM**: `XXX.XXXXX` (8 dígitos)
- **CPF**: `XXX.XXX.XXX-XX` (11 dígitos)
- **Data**: `DD/MM/AAAA` (8 dígitos)

### LED de Status

- **LED APAGADO**: Portal inativo ou sem capturas
- **LED ACESO**: Credenciais capturadas

---

## 🔍 Troubleshooting

### Portal não aparece no celular

**Android:**
- Certifique-se de que "Dados móveis" está desativado
- Aguarde a notificação "Fazer login na rede WiFi"
- Se não aparecer, abra o navegador e acesse qualquer site HTTP

**iOS:**
- O portal deve aparecer automaticamente
- Se não, vá em Ajustes > WiFi > (i) ao lado da rede

**Windows:**
- O portal deve aparecer automaticamente
- Se não, abra o navegador e acesse `http://172.0.0.1`

### Erro de compilação

```bash
# Limpe o cache e recompile
pio run -t clean
pio run
```

### ESP32 não entra em modo de upload

1. Pressione e segure o botão **BOOT**
2. Pressione e solte o botão **EN** (Reset)
3. Solte o botão **BOOT**
4. Execute o upload

### Monitor Serial não mostra nada

- Verifique se o baud rate está em **115200**
- Pressione o botão **EN** para reiniciar

---

## 📝 Changelog

### v1.0.0 (2026-01-27)
- ✅ Release inicial
- ✅ Evil Portal funcional com template UNIGRAN
- ✅ Máscaras de input para RGM, CPF e Data de Nascimento
- ✅ Compatibilidade com Android, iOS e Windows
- ✅ Scanner de Access Points
- ✅ Ataque de desautenticação
- ✅ Interface de comando via Serial
- ✅ LED de status
- ✅ Documentação completa

---

## 👤 Autor

Desenvolvido por **Felipe Daige**

---

## 📄 Licença

Este projeto é disponibilizado apenas para fins **educacionais**. 

**Use com responsabilidade e somente em ambientes de teste autorizados.**

---

<p align="center">
  <b>⚡ Powered by ESP32 + PlatformIO ⚡</b>
</p>
