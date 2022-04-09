/*******************************************************************************
* The Things Network - ABP Feather
*
* This uses ABP (Activation by Personalization), where session keys for
* communication would be assigned/generated by TTN and hard-coded on the device.
*
* Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
* Copyright (c) 2018 Terry Moore, MCCI
* Copyright (c) 2018 Brent Rubell, Adafruit Industries
*
* Permission is hereby granted, free of charge, to anyone
* obtaining a copy of this document and accompanying files,
* to do whatever they want with them without any restriction,
* including, but not limited to, copying, modification and redistribution.
* NO WARRANTY OF ANY KIND IS PROVIDED.
* 
* Reference: https://github.com/mcci-catena/arduino-lmic
* 
* Modified for DS18B20 - current measured values and Low Power Mode, 9. 4. 2022
*******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include "LowPower.h"                 // Low Power library

#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 12               // Pin 12 on the Adafruit  
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#include<CayenneLPP.h>                // Cayenne Low Power Payload (LPP)
CayenneLPP lpp(51);


//--------------------------------------- Here change your keys -------------------------------------------------

static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN NwkSKey, network session key, MSB
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN AppSKey, application session key, MSB
static const u4_t DEVADDR = 0x00000000;                                                                                                       // LoRaWAN end-device address (DevAddr), MSB

//---------------------------------------------------------------------------------------------------------------

const lmic_pinmap lmic_pins = {     // Pin mapping for the Adafruit Feather 32U4 LoRa
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {7, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,
    .spi_freq = 1000000,
};

static osjob_t sendjob;             // Job

void onEvent (ev_t ev) {
    if(ev == EV_TXCOMPLETE){
        for (int i=0; i<71; i++){
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);   // Sleep time = 8 * 71 = 568 s (9.467 minutes)
        }
        os_setCallback(&sendjob, do_send);                    // Do next transmission (transmission will be +- every 10 minutes)
    }
}

void do_send(osjob_t* j){
    float temp = 0.0;
    
    sensors.requestTemperatures();                // Send the command to get temperature readings from DS18B20
    temp = sensors.getTempCByIndex(0);            // Current temperature 

    lpp.reset();
    lpp.addTemperature(1, temp);                                  // Add the current temperature into channel 1
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
}

void setup() {
    os_init();
    LMIC_reset();

    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);

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
    
    LMIC_setLinkCheckMode(0);                                                         // Disable link check validation
    LMIC.dn2Dr = DR_SF9;                                                              // TTN uses SF9 for its RX2 window.
    LMIC_setDrTxpow(DR_SF9,14);                                                       // Set data rate and transmit power for uplink
    LMIC_setAdrMode(0); 

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    sensors.begin();       // DS18B20

    do_send(&sendjob);     // Start sendjob
}

void loop() {
    os_runloop_once();
}
