#include <TroykaDHT.h>  // TroykaDHT
DHT dht(12, DHT22);     // pin 12 and DTH22 type

float humi = 0.0;       // variable for humidity
float temp = 0.0;       // variable for temperature

void setup() {
    Serial.begin(9600);
}

void loop() {
    dht.read();

    if(dht.getState() == DHT_OK){
      
      temp = dht.getTemperatureC();   // current temperature
      humi = dht.getHumidity();       // current humidity

      Serial.print("Temp: ");
      Serial.print(temp);
      Serial.println(" °C");

      Serial.print("Humi: ");
      Serial.print(humi);
      Serial.println(" %");

      delay(10000);
   }
}
