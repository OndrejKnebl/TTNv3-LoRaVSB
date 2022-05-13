#include <TinyGPSPlus.h>
TinyGPSPlus gps;

// switch tracking / not tracking (for charging only)
int switch_charging = 18;
int switch_tracking = 19;

// LED colors
int green_LED = 22;
int red_LED = 23;

// LED blink
bool green_led_on = false;
bool red_led_on = false;

bool GPS_Error = false;

uint32_t latitude;
uint32_t longitude;
uint16_t altitude;
uint8_t hdop;

//----------------------------------------------------------------------------------------------------------------------

void turnOffLEDs(){
    digitalWrite(red_LED, HIGH);                                    // Turn the red LED off by making the voltage HIGH
    digitalWrite(green_LED, HIGH);                                  // Turn the green LED off by making the voltage HIGH
    green_led_on = false;
}

void blinkLEDTracking(){
    digitalWrite(red_LED, HIGH);                                    // Turn the red LED off by making the voltage HIGH
   
    if (green_led_on){
        digitalWrite(green_LED, HIGH);                              // Turn the green LED off by making the voltage HIGH
    }else{
        digitalWrite(green_LED, LOW);                               // Turn the green LED on by making the voltage LOW
    }
    green_led_on = !green_led_on;
    
    Serial.println(F("Tracking and charging"));
}

void blinkLEDDoingNothing(){
    digitalWrite(green_LED, HIGH);                              // Turn the green LED off by making the voltage HIGH
    digitalWrite(red_LED, HIGH);                                // Turn the red LED on by making the voltage LOW

    if (green_led_on) {
        digitalWrite(green_LED, HIGH);                          // Turn the green LED off by making the voltage HIGH
        digitalWrite(red_LED, LOW);                             // Turn the red LED on by making the voltage LOW
    } else {
        digitalWrite(green_LED, LOW);                           // Turn the green LED on by making the voltage LOW
        digitalWrite(red_LED, HIGH);                            // Turn the red LED off by making the voltage HIGH
    }
    green_led_on = !green_led_on;
    
    Serial.println(F("Not charging and not tracking"));
}

void blinkLEDCharging(){
    digitalWrite(green_LED, HIGH);                              // Turn the green LED off by making the voltage HIGH
    
    if (red_led_on){     
         digitalWrite(red_LED, HIGH);                           // Turn the red LED on by making the voltage LOW
    }else{
         digitalWrite(red_LED, LOW);                            // Turn the red LED on by making the voltage LOW
    }
    red_led_on = !red_led_on;
    
    Serial.println(F("Charging only"));
}



void get_coords(){
    bool newData = false;

    for(unsigned long start = millis(); millis() - start < 1000;){
        while(Serial1.available()){
            if(gps.encode(Serial1.read())){
                newData = true;    
            }
        }
    }

    if(newData && gps.location.isValid() && gps.altitude.isValid() && gps.hdop.isValid() && (gps.location.age() < 1000)){
        GPS_Error = false;
        saveCoordinates();
    }else{
        Serial.println("No GPS or data aren't valid!");
        GPS_Error = true;
    }
}

 
void saveCoordinates(){
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    altitude = gps.altitude.meters();
    hdop = gps.hdop.value();
}

void printCoordinates(){
    Serial.print("latitude: ");
    Serial.println(latitude);
    Serial.print("longitude: ");
    Serial.println(longitude);
    Serial.print("altitude: ");
    Serial.println(altitude);
    Serial.print("hdop: ");
    Serial.println(hdop);    
}

void doAssemblyTest(){
    if(digitalRead(switch_tracking) == LOW){                   // Tracking and charging
        blinkLEDTracking();
        
        get_coords();

        if(!GPS_Error){
            turnOffLEDs();
            printCoordinates();
        }else{
            Serial.println(F("GPS Error"));
        }
    }else if(digitalRead(switch_charging) == LOW){              // Charging only
        blinkLEDCharging();
    }else if((digitalRead(switch_tracking) == HIGH) && (digitalRead(switch_charging) == HIGH)){ // Not charging and not tracking
        blinkLEDDoingNothing();
    }
}


void setup() {
    Serial.begin(9600);
    while(!Serial){}
    Serial.println(F("Starting"));

    Serial1.begin(9600);

    pinMode(green_LED, OUTPUT);             // Green LED
    pinMode(red_LED, OUTPUT);               // Red LED
    digitalWrite(green_LED, HIGH);          // Turn the green LED off by making the voltage HIGH
    digitalWrite(red_LED, HIGH);            // Turn the red LED off by making the voltage HIGH

    pinMode(switch_charging, INPUT);
    pinMode(switch_tracking, INPUT);
    digitalWrite(switch_charging, HIGH);    // Activate arduino internal pull up
    digitalWrite(switch_tracking, HIGH);    // Activate arduino internal pull up
}

void loop() {
    doAssemblyTest();
    delay(1000);
}
