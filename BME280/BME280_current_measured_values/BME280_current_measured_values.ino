/*******************************************************************************
 * The Things Network - OTAA Feather M0
 * 
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 * 
 * Reference: https://github.com/mcci-catena/arduino-lmic
 *
 * Modified for BME280 - current measured values, 22. 7. 2022
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <Wire.h>                     // BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

#include <CayenneLPP.h>               // Cayenne Low Power Payload (LPP)
CayenneLPP lpp(51);


//--------------------------------------- Here change your keys -------------------------------------------------

static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // AppEUI, LSB
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}                        

static const u1_t PROGMEM DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // DevEUI, LSB
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // AppKey, MSB
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//---------------------------------------------------------------------------------------------------------------

const lmic_pinmap lmic_pins = {                 // Pin mapping for the Adafruit Feather M0
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,
    .spi_freq = 8000000,
};

static osjob_t sendjob;                         // Job

const unsigned TX_INTERVAL = 300;               // 5 miutes

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            break;
        case EV_BEACON_FOUND:
            break;
        case EV_BEACON_MISSED:
            break;
        case EV_BEACON_TRACKED:
            break;
        case EV_JOINING:
            break;
        case EV_JOINED:
            LMIC_setLinkCheckMode(0);             // Disable link check validation
            LMIC.dn2Dr = DR_SF9;                  // TTS uses SF9 for its RX2 window.
            LMIC_setDrTxpow(DR_SF9,14);           // Set data rate and transmit power for uplink
            LMIC_setAdrMode(0);                   // Adaptive data rate disabled
            break;
        case EV_JOIN_FAILED:
            break;
        case EV_REJOIN_FAILED:
            break;
        case EV_TXCOMPLETE:
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);  // Schedule next transmission
            break;
        case EV_LOST_TSYNC:
            break;
        case EV_RESET:
            break;
        case EV_RXCOMPLETE:
            break;
        case EV_LINK_DEAD:
            break;
        case EV_LINK_ALIVE:
            break;
        case EV_TXSTART:
            break;
        case EV_TXCANCELED:
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            break;
        default:
            break;
    }
}

void do_send(osjob_t* j){
    
    float temp = 0.0;                             // Variable for temperature
    float humi = 0.0;                             // Variable for humidity
    float pressure = 0.0;                         // Variable for pressure

    temp = bme.readTemperature();                 // Current temperature
    humi = bme.readHumidity();                    // Current humidity
    pressure = bme.readPressure() / 100.0F;       // Current pressure

    lpp.reset();
    lpp.addTemperature(1, temp);                  // Add the current temperature into channel 1
    lpp.addRelativeHumidity(2, humi);             // Add the current humidity into channel 2
    lpp.addBarometricPressure(3, pressure);       // Add the current pressure into channel 3
    
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
}


void setup() {
    digitalWrite(LED_BUILTIN, LOW);
    
    os_init();
    LMIC_reset();

    //EU868
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

    LMIC_setLinkCheckMode(0);         // Disable link check validation
    LMIC.dn2Dr = DR_SF9;              // TTS uses SF9 for its RX2 window.
    LMIC_setDrTxpow(DR_SF9,14);       // Set data rate and transmit power for uplink
    LMIC_setAdrMode(0);               // Adaptive data rate disabled

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); 

    bme.begin();                      // BME280

    do_send(&sendjob);                // Start sendjob
}

void loop() {
    os_runloop_once();
}
