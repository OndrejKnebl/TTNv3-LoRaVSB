# BME280 - will be added

Programs for temperature, humidity and pressure sensor using Adafruit Feather M0 with RFM95 LoRa Radio – 900 MHz and BME280.

## Sending measured values to TTS

We have prepared four programs for measuring temperature, humidity and pressure. The first program sends the current measured values. The second program sends averages of measured values over a certain period of time. The other two programs extend the first two with a low power mode, which is great when you run this sensor on battery power.

### Programs
- Current measured values
- Averages of measured values
- Current measured values with low power mode (Standby)
- Averages of measured values with low power mode (Standby)

### Activation
- OTAA

### More information
- About sensor and sensor assembly on the site: https://lora.vsb.cz/index.php/temperature-humidity-and-pressure-sensor-bme280/
- About Adafruit Feather M0 with RFM95 LoRa Radio – 900 MHz setup on the site: https://lora.vsb.cz/index.php/adafruit-feather-m0/

## Reference
- https://github.com/mcci-catena/arduino-lmic
- https://github.com/adafruit/Adafruit_BME280_Library
- https://github.com/ElectronicCats/CayenneLPP
- https://github.com/arduino-libraries/RTCZero
