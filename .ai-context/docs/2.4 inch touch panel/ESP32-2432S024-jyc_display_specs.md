# ESP32-2432S024-jyc (2.4") – handige hardware-specs (display/touch)

Deze notitie is samengesteld uit de meegeleverde vendor-documentatie (TFT_eSPI `User_Setup.h`) en het schema (`SCH_ESP32-2.4TFT_2023-12-19.png`).

## Overzicht

- **Board**: ESP32-2432S024-jyc (ESP32-WROOM-32D)
- **Display**: 2.4" TFT, **240×320**
- **Display-controller**: **ILI9341**
- **Interface**: SPI
- **Touch**: **XPT2046** (resistive touch), SPI
- **Backlight**: via MOSFET (gate aangestuurd door GPIO27)
- **MicroSD/TF-card**: aanwezig, **eigen SPI-bus** (niet dezelfde als TFT)

## Pinout – TFT (ILI9341) via SPI

| Functie | GPIO | Opmerking |
|---|---:|---|
| SCLK | **14** | SPI clock |
| MOSI | **13** | SPI data out |
| MISO | **12** | SPI data in |
| CS | **15** | TFT chip select |
| DC | **2** | data/command |
| RST | **-1** | reset via ESP32 reset/EN (meestal **geen** reset_pin instellen) |
| BL (backlight) | **27** | backlight gate via MOSFET |

## Pinout – Touch (XPT2046)

Touch deelt **dezelfde SPI-lijnen** als TFT (SCLK/MOSI/MISO), maar heeft een eigen CS (+ IRQ):

| Functie | GPIO |
|---|---:|
| TP_CS | **33** |
| TP_IRQ | **36** (optioneel) |
| TP_DIN (MOSI) | **13** |
| TP_OUT (MISO) | **12** |
| TP_CLK | **14** |

## Pinout – MicroSD / TF-card (aparte SPI-bus)

| Functie | GPIO |
|---|---:|
| TF_CS | **5** |
| TF_CLK | **18** |
| TF_MOSI | **23** |
| TF_MISO | **19** |

## On-board RGB LED (3 losse kanalen)

Schema toont 3 LED-kanalen op:

- **GPIO17**
- **GPIO4**
- **GPIO16**

Let op: door de aansluiting (common naar 3.3V via weerstanden) zijn deze doorgaans **active-low** (GPIO LOW = LED aan).

---

## ESPHome – minimale werkende basis (TFT + backlight)

> Start conservatief met **40MHz**. In de vendor-setup staat 80MHz, maar ESPHome is vaak stabieler op 40MHz bij dit soort modules.

```yaml
esphome:
  name: ili9341-test

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
api:

ota:
  platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

spi:
  clk_pin: GPIO14
  mosi_pin: GPIO13
  miso_pin: GPIO12

output:
  - platform: ledc
    pin: GPIO27
    id: backlight_pwm

light:
  - platform: monochromatic
    name: "ILI9341 Backlight"
    output: backlight_pwm
    restore_mode: ALWAYS_ON

display:
  - platform: ili9xxx
    model: ili9341
    cs_pin: GPIO15
    dc_pin: GPIO2
    # reset_pin: (bewust weggelaten; docs zeggen -1)
    color_palette: 8BIT
    color_order: bgr
    invert_colors: false
    data_rate: 40MHz
    update_interval: 1s
    show_test_card: true
```

### Als je wel backlight hebt maar rare kleuren
- Wissel `color_order: bgr` ↔ `rgb`
- Probeer `invert_colors: true` (soms nodig per paneel)

## ESPHome – touch toevoegen (XPT2046)

```yaml
touchscreen:
  - platform: xpt2046
    cs_pin: GPIO33
    # irq_pin: GPIO36   # optioneel, kan later
    update_interval: 50ms
    threshold: 400
```

Kalibratie (swap/invert/min/max) verschilt per montage/rotatie; log 4 hoek-touches en stel daarna in.

---

## Bronnen in de meegeleverde zip

- `.../TFT_eSPI bottom layer replacement file/User_Setup.h`  
- `5-Schematic/SCH_ESP32-2.4TFT_1-ESP32-2.4TFT_2023-12-19.png`

## Bijlagen (handige schema-crops)

- `schematic_tft_connector_zoom.png` (TFT/backlight nets)
- `schematic_touch_crop.png` (XPT2046 touch mapping)
- `schematic_tfcard_crop.png` (TF-card mapping)
- `schematic_led_rgb_crop.png` (RGB LED pins)
