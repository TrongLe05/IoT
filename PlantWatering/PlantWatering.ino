#define BLYNK_TEMPLATE_ID "TMPL6JEE9x7XR"
#define BLYNK_TEMPLATE_NAME "Plant water"
#define BLYNK_AUTH_TOKEN "HoRDoN2MBjAq2DN-3Qsj6oTgs5hLca8I"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHTesp.h>  // ✅ Dùng thư viện mới

// ====== PIN ======
#define soilSensor 33
#define rainSensor 32
#define relay      4
#define DHTPIN     25

DHTesp dht;  // ✅ Khai báo kiểu mới
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

char auth[] = "HoRDoN2MBjAq2DN-3Qsj6oTgs5hLca8I";
char ssid[] = "FakeTaxi";
char pass[] = "0123456789";

bool manualControl = false;
float lastTemp = 25.0;
float lastHum  = 60.0;

// ================== HÀM ĐỌC DHT AN TOÀN ==================
void readDHT() {
  for (int i = 0; i < 5; i++) {
    TempAndHumidity data = dht.getTempAndHumidity(); // ✅ Đọc 1 lần duy nhất
    float t = data.temperature;
    float h = data.humidity;

    if (!isnan(t) && !isnan(h) && t > 0 && t < 80 && h > 0 && h <= 100) {
      lastTemp = t;
      lastHum  = h;
      Serial.print("✅ Temp: "); Serial.print(lastTemp);
      Serial.print("C  Hum: "); Serial.print(lastHum);
      Serial.print("%  (lan thu: "); Serial.print(i + 1); Serial.println(")");
      return;
    }

    Serial.print("⚠ Lan thu "); Serial.print(i + 1); Serial.println(" that bai...");
    delay(1000); // DHTesp cần 1 giây
  }

  Serial.println("❌ DHT11 that bai sau 5 lan - Giu gia tri cu");
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(relay, OUTPUT);
  pinMode(rainSensor, INPUT);
  digitalWrite(relay, HIGH);

  // ✅ Khởi động DHTesp
  dht.setup(DHTPIN, DHTesp::DHT11);
  Serial.println("DHT started...");
  delay(3000);

  Serial.println("=== Test DHT truoc WiFi ===");
  readDHT();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("System Loading");

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  Serial.println("=== Test DHT sau WiFi ===");
  readDHT();

  lcd.clear();
  timer.setInterval(5000L, readSensors);
}

// ================== ĐỌC CẢM BIẾN ==================
void readSensors() {

  int raw  = analogRead(soilSensor);
  int soil = map(raw, 3200, 1200, 0, 100);
  soil     = constrain(soil, 0, 100);

  int rain = digitalRead(rainSensor);

  readDHT();

  Serial.print("Soil: "); Serial.print(soil); Serial.println("%");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Soil:"); lcd.print(soil); lcd.print("%");

  lcd.setCursor(9, 0);
  lcd.print(lastTemp, 1); lcd.print("C");

  lcd.setCursor(0, 1);
  if (rain == LOW) {
    lcd.print("Rain  ");
  } else {
    lcd.print("NoRain");
  }

  Blynk.virtualWrite(V0, soil);
  Blynk.virtualWrite(V2, lastTemp);
  Blynk.virtualWrite(V3, lastHum);

  if (!manualControl) {
    if (soil < 30 && rain == HIGH) {
      digitalWrite(relay, LOW);
    } else {
      digitalWrite(relay, HIGH);
    }
  }
}

// ================== NÚT BLYNK ==================
BLYNK_WRITE(V1) {
  bool Relay = param.asInt();
  manualControl = true;

  if (Relay == 1) {
    digitalWrite(relay, LOW);
    lcd.setCursor(10, 1);
    lcd.print("ON ");
  } else {
    digitalWrite(relay, HIGH);
    lcd.setCursor(10, 1);
    lcd.print("OFF");
  }
}

// ================== LOOP ==================
void loop() {
  Blynk.run();
  timer.run();
}