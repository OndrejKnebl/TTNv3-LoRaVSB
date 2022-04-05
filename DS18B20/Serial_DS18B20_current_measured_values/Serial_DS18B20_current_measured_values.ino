#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 12                 // Pin 12 on the Adafruit  
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
    Serial.begin(9600);
    sensors.begin();                    // DS18B20
}


void loop() {
    float temp = 0.0;                   // Variable for temperature

    sensors.requestTemperatures();      // Send the command to get temperature readings from DS18B20
    temp = sensors.getTempCByIndex(0);  // Current temperature

    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.println(" °C");

    delay(10000);
}
