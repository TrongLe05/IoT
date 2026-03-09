#include <DHT.h>

#define DHTPIN 14       // chân DATA của DHT11
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 Test");

  dht.begin();
}

void loop() {

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // kiểm tra lỗi đọc
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } 
  else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C  ");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  delay(2000); // DHT11 chỉ đọc mỗi 2 giây
}