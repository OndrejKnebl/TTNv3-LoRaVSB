/*******************************************************************************
 * The Things Network - ABP Feather M0
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
 * Modified for Receiving and decoding downlinks, 21. 2. 2023
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <CayenneLPP.h>
CayenneLPP lpp(51);


//--------------------------------------- Here change your keys -------------------------------------------------

static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN NwkSKey, network session key, MSB
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN AppSKey, application session key, MSB
static const u4_t DEVADDR = 0x00000000;                                                                                                       // LoRaWAN end-device address (DevAddr), MSB

//---------------------------------------------------------------------------------------------------------------      

const lmic_pinmap lmic_pins = {           // Pin mapping for Adafruit Feather M0 LoRa
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,
    .spi_freq = 8000000,
};


static osjob_t sendjob;                   // Job

static uint8_t mydata[] = "Hello, LoRa!"; // Data
const unsigned TX_INTERVAL = 30;          // Transmission interval in seconds

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
    if(ev == EV_TXCOMPLETE) {
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));

        Serial.print(F("My Data: "));
        for (int i = LMIC.dataBeg; i < LMIC.dataBeg+LMIC.dataLen; i++)
          printHex2(LMIC.frame[i]);

        Serial.println();

        DynamicJsonDocument jsonBuffer(1024);
        JsonObject root = jsonBuffer.to<JsonObject>();
        byte downlinkData[1024] = {0};

        byte j = 0;
        for(byte i = LMIC.dataBeg; i < LMIC.dataBeg + LMIC.dataLen; i++){
          downlinkData[j] = LMIC.frame[i];
          j++;
        }

        lpp.decodeTTN(downlinkData, LMIC.dataLen, root);

        serializeJsonPretty(root, Serial);
        Serial.println();
      }
      os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);  // Schedule next transmission
    }
}

void do_send(osjob_t* j){
    Serial.println("Sending - Hello, LoRa!");           // Print to Serial

    LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);    // Prepare upstream data transmission at the next possible time.
}

void setup() {
    Serial.begin(9600);
    
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
    
    LMIC_setLinkCheckMode(0);       // Disable link check validation
    LMIC.dn2Dr = DR_SF9;            // TTS uses SF9 for its RX2 window.
    LMIC_setDrTxpow(DR_SF9,14);     // Set data rate and transmit power for uplink
    LMIC_setAdrMode(0);             // Adaptive data rate disabled

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); 

    do_send(&sendjob);     // Start sendjob
}

void loop() {
    os_runloop_once();
}