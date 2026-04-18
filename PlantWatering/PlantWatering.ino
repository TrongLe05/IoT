#define BLYNK_TEMPLATE_ID "TMPL6JEE9x7XR"
#define BLYNK_TEMPLATE_NAME "Plant water"
#define BLYNK_AUTH_TOKEN "HoRDoN2MBjAq2DN-3Qsj6oTgs5hLca8I"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <wifiConfig.h>
#include <DHTesp.h>

// ====== PIN ======
#define soilSensor 33
#define rainSensor 32
#define relay 4
#define IN1 26
#define IN2 27
#define DHT_PIN 25
#define buzzer 14

// ===== PWM BUZZER =====
#define BUZZER_RESOLUTION 8

LiquidCrystal_I2C lcd(0x27, 16, 4);
BlynkTimer timer;

char auth[] = BLYNK_AUTH_TOKEN;

// ===== DHT =====
DHTesp dht;

// ===== MODE + CONTROL =====
bool manualMode = true;
bool manualActive = false;
bool manualState = false;
unsigned long manualTime = 0;

#define MANUAL_TIMEOUT 5000

// ===== PUMP STATE =====
bool pumpState = false;
bool lastPumpState = false;

// ===== MOTOR STATE =====
int motorState = 0;

// ===== DOOR STATE =====
int doorState = 2;  // Mặc định: đã đóng

// ===== MOTOR TIMER =====
bool motorTimerActive = false;
unsigned long motorTimerStart = 0;
#define MOTOR_DURATION 6000

// ===== RAIN STATE =====
bool lastRainState = true;
bool isRaining = false;

// ===== SOIL THRESHOLD =====
int soilThreshold = 30;

// ===== HÀM ĐIỀU KHIỂN ĐỘNG CƠ =====
void motorLeft()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  motorState = 1;
}

void motorRight()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  motorState = 2;
}

void motorStop()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  motorState = 0;
  motorTimerActive = false;
}

void motorStartWithTimer(int direction)
{
  if (direction == 1)
  {
    motorLeft();
    doorState = 1;
    Serial.println("[MOTOR] Quay trái (MỞ) 6s");
    Blynk.virtualWrite(V4, 1);
    Blynk.virtualWrite(V5, 0);
    Blynk.virtualWrite(V7, 1);
  }
  else if (direction == 2)
  {
    motorRight();
    doorState = 2;
    Serial.println("[MOTOR] Quay phải (ĐÓNG) 6s");
    Blynk.virtualWrite(V5, 1);
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V7, 2);
  }

  motorTimerActive = true;
  motorTimerStart = millis();
}

void handleMotorTimer()
{
  if (motorTimerActive)
  {
    if (millis() - motorTimerStart >= MOTOR_DURATION)
    {
      motorStop();
      Serial.println("[MOTOR] Hết giờ → Dừng");

      if (doorState == 1)
      {
        Blynk.virtualWrite(V4, 0);
        Blynk.virtualWrite(V5, 0);
        Blynk.virtualWrite(V7, 1);
        Serial.println("[DOOR] Đã MỞ → chỉ cho bấm ĐÓNG");
      }
      else if (doorState == 2)
      {
        Blynk.virtualWrite(V5, 0);
        Blynk.virtualWrite(V4, 0);
        Blynk.virtualWrite(V7, 2);
        Serial.println("[DOOR] Đã ĐÓNG → chỉ cho bấm MỞ");
      }
    }
  }
}

void handleRainMotor()
{
  int rain = digitalRead(rainSensor);
  bool currentRain = (rain == LOW);

  // Cập nhật V9 khi trạng thái mưa thay đổi
  if (currentRain != isRaining)
  {
    Blynk.virtualWrite(V9, currentRain ? "🌧 Đang mưa" : "☀ Không mưa");
    Serial.print("[RAIN] Trạng thái thay đổi: ");
    Serial.println(currentRain ? "Đang mưa" : "Không mưa");
  }

  isRaining = currentRain;

  if (rain == LOW && lastRainState == true)
  {
    if (!motorTimerActive && doorState != 2)
    {
      Serial.println("[RAIN] Phát hiện mưa → Motor quay phải (ĐÓNG) 6s");
      motorStartWithTimer(2);
    }
  }

  lastRainState = (rain == LOW) ? false : true;
}

void playTone(int freq, int duration)
{
  ledcWriteTone(buzzer, freq);
  delay(duration);
  ledcWriteTone(buzzer, 0);
  delay(50);
}

// ================== SETUP ==================
void setup()
{
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

  ledcAttach(buzzer, 2000, BUZZER_RESOLUTION);
  dht.setup(DHT_PIN, DHTesp::DHT11);

  lcd.setCursor(1, 0);
  lcd.print("System Loading");

  delay(2000);

  playTone(659, 150); playTone(659, 150); delay(150);
  playTone(659, 150); delay(150);
  playTone(523, 150); playTone(659, 150); delay(150);
  playTone(784, 150); delay(300);
  playTone(392, 150);

  lcd.clear();

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.config(auth);
    Blynk.connect();
    Serial.println("Blynk connected!");

    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V6, 1);
    Blynk.virtualWrite(V8, soilThreshold);
    Blynk.virtualWrite(V9, "☀ Không mưa"); // mặc định khi khởi động
    Blynk.virtualWrite(V5, 0);   // ← THÊM: nút đóng tắt
    Blynk.virtualWrite(V4, 0);   // ← THÊM: nút mở sẵn sàng
    Blynk.virtualWrite(V7, 2);   // ← THÊM: hiển thị trạng thái CLOSED
  }
  else
  {
    Serial.println("No WiFi");
  }

  timer.setInterval(2000L, readSensors);
}

// ================== ĐỌC CẢM BIẾN ==================
void readSensors()
{
  int raw = analogRead(soilSensor);
  int soil = map(raw, 3200, 1200, 0, 100);
  soil = constrain(soil, 0, 100);

  int rain = digitalRead(rainSensor);

  TempAndHumidity data = dht.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  lcd.clear();

  // ===== DÒNG 0: Nhiệt độ | Độ ẩm không khí =====
  // "T:28.5C   Hum:65% "
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C");

  lcd.setCursor(10, 0);
  lcd.print("Hum:");
  lcd.print((int)humidity);
  lcd.print("%");

  // ===== DÒNG 1: Độ ẩm đất | Ngưỡng =====
  // "Soil:45%  Thr:30% "
  lcd.setCursor(0, 1);
  lcd.print("Soil:");
  lcd.print(soil);
  lcd.print("%");

  lcd.setCursor(10, 1);
  lcd.print("Thr:");
  lcd.print(soilThreshold);
  lcd.print("%");

  // ===== DÒNG 2: Mưa | Trạng thái cửa =====
  // "Rain:No   OPEN    "
  lcd.setCursor(0, 2);
  lcd.print("Rain:");
  lcd.print(rain == LOW ? "Yes" : "No ");

  lcd.setCursor(10, 2);
  if (motorTimerActive)
    lcd.print(motorState == 1 ? "OPENING   " : "CLOSING   ");
  else
    lcd.print(doorState == 1 ? "OPEN      " : doorState == 2 ? "CLOSED    " : "UNKNOWN   ");

  // ===== DÒNG 3: Chế độ | Bơm =====
  // "Mode:AUTO  P:OFF  "
  lcd.setCursor(0, 3);
  lcd.print("Mode:");
  lcd.print(manualMode ? "MAN" : "AUTO");

  lcd.setCursor(10, 3);
  lcd.print("Pump:");
  lcd.print(pumpState ? "ON " : "OFF");

  // ===== LOGIC BƠM =====
  if (manualMode)
  {
    pumpState = manualState;
    digitalWrite(relay, manualState ? LOW : HIGH);
  }
  else
  {
    if (manualActive && millis() - manualTime < MANUAL_TIMEOUT)
    {
      pumpState = manualState;
      digitalWrite(relay, manualState ? LOW : HIGH);
    }
    else
    {
      manualActive = false;
      if (soil < soilThreshold && rain == HIGH)
      {
        digitalWrite(relay, LOW);
        pumpState = true;
      }
      else
      {
        digitalWrite(relay, HIGH);
        pumpState = false;
      }
    }
  }

  // ===== GỬI BLYNK =====
  Blynk.virtualWrite(V0, soil);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, humidity);
  Blynk.virtualWrite(V9, isRaining ? "🌧 Đang mưa" : "☀ Không mưa");

  if (pumpState != lastPumpState)
  {
    Blynk.virtualWrite(V1, pumpState);
    lastPumpState = pumpState;
  }

  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" | Hum: "); Serial.print(humidity);
  Serial.print(" | Soil: "); Serial.print(soil);
  Serial.print("% | Threshold: "); Serial.print(soilThreshold);
  Serial.print("% | Door: ");
  Serial.print(doorState == 1 ? "OPEN" : doorState == 2 ? "CLOSED" : "UNKNOWN");
  Serial.print(" | Rain: ");
  Serial.println(isRaining ? "YES" : "NO");
}

// ================== BLYNK ==================

BLYNK_WRITE(V6)
{
  manualMode = param.asInt();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(manualMode ? "Mode: MANUAL" : "Mode: AUTO  ");
}

BLYNK_WRITE(V1)
{
  int val = param.asInt();
  manualActive = true;
  manualTime = millis();
  manualState = (val == 1);
  digitalWrite(relay, manualState ? LOW : HIGH);
  pumpState = manualState;
}

BLYNK_WRITE(V8)
{
  soilThreshold = param.asInt();
  soilThreshold = constrain(soilThreshold, 0, 100);

  Serial.print("[THRESHOLD] Ngưỡng mới: ");
  Serial.print(soilThreshold);
  Serial.println("%");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Threshold set:");
  lcd.setCursor(0, 1);
  lcd.print(soilThreshold);
  lcd.print("% soil moisture");
}

BLYNK_WRITE(V4)
{
  if (param.asInt() != 1) return;

  if (motorTimerActive)
  {
    Serial.println("[V4] Đang chạy, bỏ qua");
    Blynk.virtualWrite(V4, 0);
    return;
  }

  if (doorState == 1)
  {
    Serial.println("[V4] Đã MỞ rồi, bỏ qua");
    Blynk.virtualWrite(V4, 0);
    return;
  }

  if (isRaining)
  {
    Serial.println("[V4] Đang mưa, không cho mở");
    Blynk.virtualWrite(V4, 0);
    return;
  }

  motorStartWithTimer(1);
}

BLYNK_WRITE(V5)
{
  if (param.asInt() != 1) return;

  if (motorTimerActive)
  {
    Serial.println("[V5] Đang chạy, bỏ qua");
    Blynk.virtualWrite(V5, 0);
    return;
  }

  if (doorState == 2)
  {
    Serial.println("[V5] Đã ĐÓNG rồi, bỏ qua");
    Blynk.virtualWrite(V5, 0);
    return;
  }

  motorStartWithTimer(2);
}

BLYNK_WRITE(V7)
{
  // Chỉ hiển thị
}

// ================== LOOP ==================
void loop()
{
  wifiConfig.run();

  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
  }

  timer.run();

  handleMotorTimer();
  handleRainMotor();
}