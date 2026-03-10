#include <DHT.h>

#define DHTPIN 25
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) {
    Serial.println("DHT error");
  } else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print("  Hum: ");
    Serial.println(h);
  }

  delay(2000);
}