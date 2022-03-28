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
* Modified for DHT22 - average measured values, 28. 3. 2022
*******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <TroykaDHT.h>        // TroykaDHT
DHT dht(12, DHT22);           // Pin 12 and DTH22 type

#include<CayenneLPP.h>        // Cayenne Low Power Payload (LPP)
CayenneLPP lpp(51);

//---------------------------------------------------------------------------------------------------------------

// Timer
unsigned long previousMillis = 0;             // Previous time
const long interval = 1000;                   // Interval 1000 ms
unsigned int countSeconds = 1;                // Counting seconds
unsigned int sendDataEvery = 300;             // Send data every 5 minutes


// Temperature and humidity sensor DHT22
float humi = 0.0;                             // Variable for humidity
float temp = 0.0;                             // Variable for temperature
float numberOfSamplesDHT = 0;                 // Variable for number of measured samples by DHT
bool errorDHT = false;                        // DHT Error

// Jobs
static osjob_t measurejob, sendjob;

//--------------------------------------- Here change your keys -------------------------------------------------

static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN NwkSKey, network session key, MSB
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // LoRaWAN AppSKey, application session key, MSB
static const u4_t DEVADDR = 0x00000000;                                                                                                       // LoRaWAN end-device address (DevAddr), MSB

//---------------------------------------------------------------------------------------------------------------

const lmic_pinmap lmic_pins = {         // Pin mapping for the Adafruit Feather 32U4 LoRa
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {7, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,
    .spi_freq = 1000000,
};

void onEvent (ev_t ev) {
    if(ev == EV_TXCOMPLETE){
        os_setCallback(&measurejob, do_measure);
    }
}

void do_send(osjob_t* j){
    lpp.reset();
    lpp.addTemperature(1, temp / numberOfSamplesDHT);             // Add the average temperature into channel 1
    lpp.addRelativeHumidity(2, humi / numberOfSamplesDHT);        // Add the average humidity into channel 2
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);        // Prepare upstream data transmission at the next possible time.
  
    resetValues();                                                // Reset values
}


void resetValues() {                    // Reset values
    temp = 0.0;
    humi = 0.0;  
    numberOfSamplesDHT = 0;
    errorDHT = false;
}


void measureTempsAndHumi(){
    dht.read();

    if(dht.getState() == DHT_OK){
        temp = temp + dht.getTemperatureC();      // Add the current temperature to all measured temperatures
        humi = humi + dht.getHumidity();          // Add the current humidity to all measured humidities

        numberOfSamplesDHT++;                     // Add measured sample  
        errorDHT = false;                         // There wasn't DHT error
        
    }else{
        errorDHT = true;                          // There was DHT error, measureTempsAndHumi() will be called again
    }
}

void do_measure(osjob_t* j){

    if(errorDHT){                                       // There was DHT error, measureTempsAndHumi() will be called again
        measureTempsAndHumi();
    }

    unsigned long currentMillis = millis();             // Current millis

    if(currentMillis - previousMillis >= interval) {    // Timer set to 1 second
        previousMillis = currentMillis;
        
        if(countSeconds % 10 == 0){                     // Every 10 seconds
            measureTempsAndHumi();
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

    do_send(&sendjob);     // Start sendjob
}

void loop() {
    os_runloop_once();
}
