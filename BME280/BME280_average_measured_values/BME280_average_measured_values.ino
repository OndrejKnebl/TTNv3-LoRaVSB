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
 * Modified for BME280 - average measured values, 23. 7. 2022
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

//---------------------------------------------------------------------------------------------------------------

// Timer
unsigned long previousMillis = 0;             // Previous time
const long interval = 1000;                   // Interval 1000 ms
unsigned int countSeconds = 1;                // Counting seconds
unsigned int sendDataEvery = 300;             // Send data every 5 minutes

// BME280
float temp = 0.0;                             // Variable for temperature
float humi = 0.0;                             // Variable for humidity
float pressure = 0.0;                         // Variable for pressure
float numberOfSamples = 0;                    // Variable for number of measured samples by BME280

// Jobs
static osjob_t measurejob, sendjob;

// First value
bool isFirst = true;

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
            os_setCallback(&measurejob, do_measure);
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

    if(isFirst){
      
        lpp.reset();
        lpp.addDigitalInput(51, 1);                                   // First value
        
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
        isFirst = false;
    }else{
      
        lpp.reset();
        lpp.addTemperature(1, temp / numberOfSamples);                // Add the average temperature into channel 1
        lpp.addRelativeHumidity(2, humi / numberOfSamples);           // Add the average humidity into channel 2
        lpp.addBarometricPressure(3, pressure / numberOfSamples);     // Add the average pressure into channel 3
        
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
        
        resetValues();                                                // Reset values
    }
}


void resetValues() {              // Reset values
    temp = 0.0;
    humi = 0.0;
    pressure = 0.0;
    numberOfSamples = 0;
}


void measureTempHumiPress(){
    
    temp = temp + bme.readTemperature();                    // Add the current temperature to all measured temperatures
    humi = humi + bme.readHumidity();                       // Add the current humidity to all measured humidities
    pressure = pressure + (bme.readPressure() / 100.0F);    // Add the current pressure to all measured pressures

    numberOfSamples++;                                      // Add measured sample
}


void do_measure(osjob_t* j){

    unsigned long currentMillis = millis();             // Current millis

    if(currentMillis - previousMillis >= interval) {    // Timer set to 1 second
        previousMillis = currentMillis;
        
        if(countSeconds % 10 == 0){                     // Every 10 seconds
            measureTempHumiPress();
        }

        if(countSeconds % sendDataEvery == 0){          // Every 5 minutes
            os_setCallback(&sendjob, do_send);          // Call do_send
            countSeconds = 0;                           // Zero seconds counter
        }
        countSeconds++;                                 // + 1 second
    }

    if((countSeconds - 1) % sendDataEvery != 0){        // When is called do_send, don't call do_measure
        os_setCallback(&measurejob, do_measure);
    }
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
