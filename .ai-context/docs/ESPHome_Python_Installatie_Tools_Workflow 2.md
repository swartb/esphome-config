
# ESPHome – Python, Installatie, Tools & Workflow (Raspad / Raspberry Pi)

Deze handleiding beschrijft **van nul tot werkend** hoe je een solide ESPHome-
ontwikkelomgeving opzet op een Raspad / Raspberry Pi.

---

## 0. Uitgangspunten

- Raspberry Pi OS (liefst 64-bit)
- Internetverbinding
- Geen bestaande ESPHome-installatie vereist

---

## 1. Python installeren & controleren

### 1.1 Controleren of Python aanwezig is
```bash
python3 --version
```
Je wilt minimaal:
```
Python 3.9+
```

### 1.2 Python installeren (indien nodig)
```bash
sudo apt update
sudo apt install -y python3 python3-pip python3-venv
```

Controleer opnieuw:
```bash
python3 --version
pip3 --version
```

---

## 2. Systeemvoorbereiding

```bash
sudo apt update
sudo apt upgrade -y
sudo reboot
```

Installeer aanvullende tools:
```bash
sudo apt install -y git curl
```

---

## 3. ESPHome installeren (best practice: virtual environment)

### 3.1 Projectmap maken
```bash
mkdir -p ~/esphome
cd ~/esphome
```

### 3.2 Virtual environment aanmaken
```bash
python3 -m venv venv
source venv/bin/activate
```

Je prompt toont nu:
```
(venv)
```

### 3.3 ESPHome installeren
```bash
pip install --upgrade pip
pip install esphome
```

Test:
```bash
esphome version
```

---

## 4. ESPHome Dashboard starten

```bash
cd ~/esphome
source venv/bin/activate
esphome dashboard .
```

Bereikbaar via browser:
```
http://<raspad-ip>:6052
```

---

## 5. Eerste ESPHome device maken

Via Dashboard:
- + New Device
- Platform: ESP32
- Board: esp32-c3-devkitm-1 (voor ESP32-C3)
- WiFi instellen

ESPHome maakt automatisch een `.yaml`.

---

## 6. USB-rechten & eerste flash

### 6.1 USB-rechten
```bash
sudo usermod -aG dialout $USER
sudo reboot
```

Na reboot:
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

### 6.2 Eerste flash
- ESP aansluiten via USB
- Dashboard → Install → Plug into this computer

Daarna is **OTA standaard**.

---

## 7. OTA-workflow

- ESP verschijnt met IP-adres
- Volgende uploads:
  - Install → Wirelessly

Geen USB meer nodig.

---

## 8. Editors

### Visual Studio Code (aanrader)
**Waarom**
- Beste YAML-ondersteuning
- IntelliSense
- Git-integratie

**Aanbevolen extensies**
- YAML
- ESPHome
- GitLens (optioneel)

Gebruik VS Code voor:
- YAML schrijven
- Structuur en hergebruik (`!include`)

---

### Thonny
Gebruik voor:
- MicroPython (Pico / ESP8266)
- Snelle hardware-tests

Niet ideaal voor ESPHome YAML.

---

## 9. ESPHome CLI (optioneel)

```bash
esphome run device.yaml
esphome logs device.yaml
esphome config device.yaml
```

---

## 10. Projectstructuur (aanrader)

```text
esphome/
├─ common/
│  ├─ wifi.yaml
│  ├─ logger.yaml
├─ devices/
│  ├─ ld24120.yaml
├─ secrets.yaml
```

---

## 11. Logging & debug

Tijdens ontwikkeling:
```yaml
logger:
  level: DEBUG
```

UART vrijhouden:
```yaml
logger:
  baud_rate: 0
```

---

## 12. Remote werken

### VPN (aanrader)
- Tailscale (simpel)
- WireGuard (meer controle)

Hiermee:
- ESPHome Dashboard overal bereikbaar
- OTA blijft werken

---

## 13. Versiebeheer

### Git
Gebruik Git voor:
- ESPHome YAML
- configuraties
- documentatie

---

## 14. Aanbevolen workflow (samenvatting)

1. Python + venv
2. ESPHome installeren
3. Code schrijven in VS Code
4. Eerste flash via USB
5. Daarna alles OTA
6. Remote werken via VPN

---

_Einde document_
