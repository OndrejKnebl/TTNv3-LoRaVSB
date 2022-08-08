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
 * Modified for TTS Network Time, 8. 8. 2022
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <TimeLib.h>
#include <Timezone.h>

// Timer
unsigned long previousMillis = 0;             // Previous time
const long interval = 1000;                   // Interval 1000 ms
unsigned int countSeconds = 1;                // Counting seconds
unsigned int sendDataEvery = 600;             // Send data every 10 minutes

uint32_t user_time_correction = -18;          // User time correction in seconds
int countSends = 0;                           // Counting sends
int requestTimeEverySend = 10;                // Request time every 10 send

uint32_t userUTCTime;                         // Seconds since the UTC epoch

//--------------------------------------- Here change your keys -------------------------------------------------

static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // AppEUI, LSB
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}                        

static const u1_t PROGMEM DEVEUI[8]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // DevEUI, LSB
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // AppKey, MSB
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//---------------------------------------------------------------------------------------------------------------


// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BST, GMT);

// UTC
TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
Timezone UTC(utcRule);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

// US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

// US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
Timezone usMT(usMDT, usMST);

// Arizona is US Mountain Time Zone but does not use DST
Timezone usAZ(usMST);

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

// Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
Timezone ausET(aEDT, aEST);

// Moscow Standard Time (MSK, does not observe DST)
TimeChangeRule msk = {"MSK", Last, Sun, Mar, 1, 180};
Timezone tzMSK(msk);

//---------------------------------------------------------------------------------------------------------------

static uint8_t mydata[] = "Hello, world!";
static osjob_t showjob, sendjob;


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
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            LMIC_setLinkCheckMode(0);             // Disable link check validation
            LMIC.dn2Dr = DR_SF9;                  // TTS uses SF9 for its RX2 window.
            LMIC_setDrTxpow(DR_SF9,14);           // Set data rate and transmit power for uplink
            LMIC_setAdrMode(0);                   // Adaptive data rate disabled
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            os_setCallback(&showjob, do_show);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}


void user_request_network_time_callback(void *pVoidUserUTCTime, int flagSuccess){

    uint32_t *pUserUTCTime = (uint32_t *) pVoidUserUTCTime;                                 // Explicit conversion from void* to uint32_t* to avoid compiler errors

    lmic_time_reference_t lmicTimeReference;

    if(flagSuccess != 1){
        Serial.println("USER CALLBACK: Not a success");
        return;
    }

    flagSuccess = LMIC_getNetworkTimeReference(&lmicTimeReference);                         // Populate "lmic_time_reference"
    if (flagSuccess != 1) {
        Serial.println("USER CALLBACK: LMIC_getNetworkTimeReference didn't succeed");
        return;
    }
    
    *pUserUTCTime = lmicTimeReference.tNetwork + 315964800;                                 // Update userUTCTime, considering the difference between the GPS and UTC epoch, and the leap seconds

    ostime_t ticksNow = os_getTime();                                                       // Current time, in ticks
    ostime_t ticksRequestSent = lmicTimeReference.tLocal;                                   // Time when the request was sent, in ticks
    uint32_t requestDelaySec = osticks2ms(ticksNow - ticksRequestSent) / 1000;
    *pUserUTCTime += requestDelaySec;
    
    *pUserUTCTime += user_time_correction;                                                  // User time correction

    setTime(*pUserUTCTime);                                                                 // Update the system time with the time read from the network

    Serial.println("USER CALLBACK: Success");
}


void printDigits(int digits){
    if(digits < 10){
        Serial.print('0');
    }
    Serial.print(digits);
}


void printDateTime(Timezone tz, time_t utc, const char *descr){                               
    
    TimeChangeRule *tcr;
    time_t t = tz.toLocal(utc, &tcr);

    printDigits(hour(t));
    Serial.print(':');
    printDigits(minute(t));
    Serial.print(':');
    printDigits(second(t));

    Serial.print(" ");
    printDigits(day(t));
    Serial.print('/');
    printDigits(month(t));
    Serial.print('/');
    Serial.print(year(t));

    Serial.print(" ");
    Serial.print(tcr -> abbrev);

    String timeZone = tcr -> abbrev; 
    if(timeZone.length() == 3){
        Serial.print(" ");
    }

    Serial.print(" ");
    Serial.println(descr);
}


void do_send(osjob_t* j) {
    if (!(LMIC.opmode & OP_TXRXPEND)){
        
        if(countSends % requestTimeEverySend == 0){
            LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);      // Schedule a network time request at the next possible time
            countSends = 0;
        }
        countSends++;
        
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);                                    // Prepare upstream data transmission at the next possible time.
    }
}


void printTimes(){
    time_t utc = now();
    
    printDateTime(CE, utc, "Ostrava");
    printDateTime(UK, utc, "London");
    printDateTime(UTC, utc, "Universal Coordinated Time");
    printDateTime(usET, utc, "New York");
    printDateTime(usCT, utc, "Chicago");
    printDateTime(usMT, utc, "Denver");
    printDateTime(usAZ, utc, "Phoenix");
    printDateTime(usPT, utc, "Los Angeles");
    printDateTime(ausET, utc, "Sydney");
    printDateTime(tzMSK, utc, "Moscow");

    Serial.println();
}


void do_show(osjob_t* j){

    unsigned long currentMillis = millis();             // Current millis

    if(currentMillis - previousMillis >= interval) {    // Timer set to 1 second
        previousMillis = currentMillis;
    
        printTimes();                                   // Print times
        
        if(countSeconds % sendDataEvery == 0){          // Every 10 minutes
            os_setCallback(&sendjob, do_send);          // Call do_send
            countSeconds = 0;                           // Zero seconds counter
        }
        countSeconds++;                                 // + 1 second
    }

    if((countSeconds - 1) % sendDataEvery != 0){        // When is called do_send, don't call do_measure
        os_setCallback(&showjob, do_show);
    }
}


void setup() {

    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB
    }
  
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

    do_send(&sendjob);                // Start sendjob
}

void loop() {
    os_runloop_once();
}
