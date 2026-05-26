# JC3248W535 (JC3248W535C_I_Y) – handige specs (ESP32-S3 3.5" capacitive touch display)

## Overzicht (uit meegeleverde documentatie)
- **Formaat:** 3.5" TFT **IPS**
- **Resolutie:** **320 × 480 px**
- **Driver IC (LCD):** **AXS15231B**
- **Touch:** **Capacitief** (I²C)
- **Voeding:** **5V**
- **Verbruik:** ~**150 mA** (indicatief)
- **Actief schermoppervlak:** **73.4 × 49.0 mm**
- **Module-afmeting:** **94.5 × 62.0 mm**
- **Temperatuur:** **-20°C ~ 70°C** (bedrijf), **-30°C ~ 80°C** (opslag)

### MCU-module (ESP32-S3-WROOM-1)
- **CPU:** dual-core, tot **240 MHz**
- **Geheugen (volgens specs PDF):** **512 KB SRAM**, **384 KB ROM**
- **PSRAM:** **8 MB**
- **Flash:** **16 MB**
- **Connectiviteit:** Wi‑Fi + Bluetooth

> Bron: *JC3248W535 Specifications-EN.pdf* (meegeleverd in jouw ZIP)

---

## Interfaces en pinmapping (uit schema/IO pin distribution)

### LCD (QSPI)
| Functie | GPIO |
|---|---:|
| LCD_CS | **45** |
| LCD_CLK / SCK | **47** |
| LCD_D0 (SDA0) | **21** |
| LCD_D1 (SDA1) | **48** |
| LCD_D2 (SDA2) | **40** |
| LCD_D3 (SDA3) | **39** |
| LCD_TE (tearing effect, optioneel) | **38** |
| LCD_RST | **RST** (gedeeld met ESP32 reset, geen aparte GPIO) |

### Backlight (PWM)
| Functie | GPIO |
|---|---:|
| BL / backlight enable/PWM (via MOSFET) | **1** |

### Touch (capacitief, I²C)
| Functie | GPIO |
|---|---:|
| I²C SCL (TP_SCL) | **4** |
| I²C SDA (TP_SDA) | **8** |
| TP_INT | In schema op display-connector aanwezig, maar lijkt **niet als interrupt naar ESP32** doorgevoerd (controleer in jouw board/repo als je interrupt wilt gebruiken). |

### microSD/TF (SPI)
| Functie | GPIO |
|---|---:|
| TF_CS | **10** |
| SPI MOSI (MCU_MOSI) | **11** |
| SPI CLK (TF_CLK) | **12** |
| SPI MISO (MCU_MISO) | **13** |

### USB (indicatief uit schema)
| Functie | GPIO |
|---|---:|
| USB D- | **19** |
| USB D+ | **20** |

### Audio (I²S – aanwezig in schema)
In het schema zijn o.a. deze I²S-signalen te zien richting een audio/amp-circuit:
- **LRCLK/WS:** GPIO **2**
- **BCLK:** GPIO **42**
- **DIN:** GPIO **41**

---

## Praktische notes (ESPHome / development)
- Dit board gebruikt **QSPI** voor het display en een **AXS15231B** LCD-driver.
- In de meegeleverde Arduino demo’s wordt een custom driver gebruikt (`esp_lcd_axs15231b.*` + LVGL).  
  In ESPHome is dit vaak nét even anders dan “klassieke” SPI-schermen (zoals ILI9341 via `ili9xxx`).
- Voor jouw eigen ESPHome-config is deze file vooral bedoeld als **hardware referentie** (pins, driver, resolutie).

---

## Bestanden in deze ZIP
- `JC3248W535_display_specs.md` (dit document)
- `JC3248W535 Specifications-EN.pdf` (originele productspecs)
- `5-IO pin distribution/JC3248W535-1.png`
- `5-IO pin distribution/JC3248W535-2.png`
- `crops/` met ingezoomde schema-crops (LCD pinblok e.d.)

