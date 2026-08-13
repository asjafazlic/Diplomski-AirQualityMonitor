# Realizacija sistema za mjerenje kvaliteta zraka korištenjem IoT platforme bazirane na ESP32 SoC i MQTT protokolu

**DIPLOMSKI RAD** \
Univerzitet u Tuzli\
Fakultet elektrotehnike Tuzla\
Student: Asja Fazlić\
Mentor: dr. sci. Jakub Osmić\
Godina: 2026.

---

Repozitorij sadrži izvorni kod za praktični dio realizacije diplomskog rada. Sistem je baziran na ESP32 mikrokontroleru i služi za kontinuirano praćenje parametara kvaliteta zraka. Podaci se prikupljaju u realnom vremenu sa odgovarajućih senzora, te se šalju putem MQTT protokola i prikazuju na responzivnom web interfejsu (Dashboard).

---

## Hardver i Softver

### Hardverske komponente:
* **Mikrokontroler:** ESP32 DevKit V1
* **Senzori:** BME280, CCS811, PMS5003

### Softver i biblioteke
* **Razvojno okruženje:** Arduino IDE
* **Tehnologije:** C++ (Arduino), HTML5, CSS3, JavaScript
* **Korištene biblioteke:**
  * 'WiFi.h'
  * 'PubSubClient.h'
  * 'Wire.h'
  * 'Adafruit_BME280.h'
  * 'Adafruit_CCS811.h'
  * 'PMS.h'
