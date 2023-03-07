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
 * Modified for PZEM - current measured values, 7. 3. 2023
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <PZEM004Tv30.h>              // PZEM
PZEM004Tv30 pzem(Serial1, PZEM_DEFAULT_ADDR);

#include <CayenneLPP.h>               // Cayenne Low Power Payload (LPP)
CayenneLPP lpp(51);

//---------------------------------------------------------------------------------------------------------------

// PZEM
float voltage = 0.0;
float current = 0.0;
float power = 0.0;
float energy = 0.000;
float frequency = 0.0;
float pf = 0.0;

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

const unsigned TX_INTERVAL = 300;               // 5 minutes

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

void measurePZEM(){

    float currentVoltage = pzem.voltage();
    float currentCurrent = pzem.current();
    float currentPower = pzem.power();
    float currentEnergy = pzem.energy();
    float currentFrequency = pzem.frequency();
    float currentPf = pzem.pf();

     if(isnan(currentVoltage)){
        Serial.println("Error reading voltage");
    } else if (isnan(currentCurrent)) {
        Serial.println("Error reading current");
    } else if (isnan(currentPower)) {
        Serial.println("Error reading power");
    } else if (isnan(currentEnergy)) {
        Serial.println("Error reading energy");
    } else if (isnan(currentFrequency)) {
        Serial.println("Error reading frequency");
    } else if (isnan(currentPf)) {
        Serial.println("Error reading power factor");
    } else {
        voltage = currentVoltage;
        frequency = currentFrequency;
        energy = currentEnergy;
        current = currentCurrent;
        power = currentPower;
        pf = currentPf;
     }
}


void do_send(osjob_t* j){
    
    measurePZEM();

    lpp.reset();
    lpp.addVoltage(1, voltage);                               // Add Power Line Voltage into channel 1
    lpp.addFrequency(2, frequency);                           // Add Power Line Frequency into channel 2
    lpp.addEnergy(3, energy);                                 // Add Active Energy into channel 3
    lpp.addCurrent(4, current);                               // Add Current into channel 4
    lpp.addPower(5, power);                                   // Add Active Power into channel 5
    lpp.addAnalogOutput(6, pf);                               // Add Power factor into channel 6

    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
    resetValues();                                                // Reset values
}

void resetValues() {              // Reset values

    if(energy >= 0.001)
        pzem.resetEnergy();

    voltage = 0.0;
    current = 0.0;
    power = 0.0;
    energy = 0.0;
    frequency = 0.0;
    pf = 0.0;
}


void setup() {
    digitalWrite(LED_BUILTIN, LOW);

    Serial1.begin(9600);                // PZEM
    
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

    do_send(&sendjob);                // Start sendjob
}

void loop() {
    os_runloop_once();
}