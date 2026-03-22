#define BLYNK_TEMPLATE_ID "TMPL6JEE9x7XR"
#define BLYNK_TEMPLATE_NAME "Plant water"
#define BLYNK_AUTH_TOKEN "HoRDoN2MBjAq2DN-3Qsj6oTgs5hLca8I"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <wifiConfig.h>

// ====== PIN ======
#define soilSensor 33
#define rainSensor 32
#define relay 4
#define IN1 26
#define IN2 27

LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

char auth[] = BLYNK_AUTH_TOKEN;

// ===== MODE + CONTROL =====
bool manualMode = false;      // V6
bool manualActive = false;    // override trong AUTO
bool manualState = false;     // ON/OFF
unsigned long manualTime = 0;

#define MANUAL_TIMEOUT 5000 // 5 giây

// ===== PUMP STATE =====
bool pumpState = false;
bool lastPumpState = false;

// ================== SETUP ==================
void setup() {

  Serial.begin(115200);

  wifiConfig.begin();

  lcd.init();
  lcd.backlight();

  pinMode(relay, OUTPUT);
  pinMode(rainSensor, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(relay, HIGH);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  lcd.setCursor(1,0);
  lcd.print("System Loading");

  delay(2000);
  lcd.clear();

  // ⏳ đợi WiFi
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(auth);
    Blynk.connect();
    Serial.println("Blynk connected!");
  } else {
    Serial.println("No WiFi");
  }

  timer.setInterval(2000L, readSensors);
}

// ================== ĐỌC CẢM BIẾN ==================
void readSensors() {

  int raw = analogRead(soilSensor);
  int soil = map(raw, 3200, 1200, 0, 100);
  soil = constrain(soil, 0, 100);

  int rain = digitalRead(rainSensor);

  // ===== LCD =====
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Soil:");
  lcd.print(soil);
  lcd.print("%");

  lcd.setCursor(0,1);

  if(rain == LOW){
    lcd.print("Rain ");

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    lcd.print("NoRain");

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // ===== LOGIC BƠM =====
  if(manualMode){
    // 🎮 MANUAL MODE
    if(manualState){
      digitalWrite(relay, LOW);
      pumpState = true;
    } else {
      digitalWrite(relay, HIGH);
      pumpState = false;
    }
  }
  else{
    // 🤖 AUTO MODE

    if(manualActive && millis() - manualTime < MANUAL_TIMEOUT){
      // override
      if(manualState){
        digitalWrite(relay, LOW);
        pumpState = true;
      } else {
        digitalWrite(relay, HIGH);
        pumpState = false;
      }
    }
    else{
      manualActive = false;

      if(soil < 30 && rain == HIGH){
        digitalWrite(relay, LOW);
        pumpState = true;
      } else {
        digitalWrite(relay, HIGH);
        pumpState = false;
      }
    }
  }

  // ===== GỬI BLYNK =====
  Blynk.virtualWrite(V0, soil);

  // 🔥 SYNC TRẠNG THÁI BƠM
  if(pumpState != lastPumpState){
    Blynk.virtualWrite(V1, pumpState);
    lastPumpState = pumpState;
  }
}

// ================== BLYNK ==================

// 🎮 MODE
BLYNK_WRITE(V6) {
  manualMode = param.asInt();

  lcd.clear();
  lcd.setCursor(0,0);

  if(manualMode){
    lcd.print("Mode: MANUAL");
    Serial.println("MANUAL MODE");
  } else {
    lcd.print("Mode: AUTO");
    Serial.println("AUTO MODE");
  }
}

// 💧 BƠM
BLYNK_WRITE(V1) {

  int Relay = param.asInt();

  manualActive = true;
  manualTime = millis();

  if(Relay == 1){
    manualState = true;
    digitalWrite(relay, LOW);
    pumpState = true;
  }
  else{
    manualState = false;
    digitalWrite(relay, HIGH);
    pumpState = false;
  }
}

// ================== LOOP ==================
void loop() {
  wifiConfig.run();

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  timer.run();
}