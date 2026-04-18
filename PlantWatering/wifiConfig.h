#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
WebServer webServer(80);
#include <Ticker.h>
Ticker blinker;

String ssid;
String password;
#define ledPin 2
#define btnPin 0
unsigned long lastTimePress = millis();
#define PUSHTIME 5000
int wifiMode;
unsigned long blinkTime = millis();

bool needReconnect = false;
unsigned long reconnectTime = 0;
#define RECONNECT_DELAY 3000

const char html[] PROGMEM = R"html(
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Tưới Cây Thông Minh</title>
  <link href="https://fonts.googleapis.com/css2?family=DM+Mono:wght@400;500&family=Fraunces:ital,opsz,wght@0,9..144,300;0,9..144,600;1,9..144,300&display=swap" rel="stylesheet"/>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    :root {
      --soil:      #3d2b1f;
      --leaf:      #2d5a27;
      --leaf-mid:  #4a8c3f;
      --leaf-lite: #7abf6e;
      --cream:     #f5f0e8;
      --mist:      #e8f0e3;
      --rust:      #c0392b;
      --sun:       #e8a020;
      --text:      #1a1a1a;
      --text-sub:  #5a5a5a;
      --card-bg:   rgba(255,255,255,0.82);
      --border:    rgba(45,90,39,0.18);
      --shadow:    0 8px 40px rgba(45,90,39,0.13);
      --radius:    16px;
    }

    html, body {
      min-height: 100vh;
      font-family: 'DM Mono', monospace;
      background: var(--cream);
      color: var(--text);
      overflow-x: hidden;
    }

    body::before, body::after {
      content: '';
      position: fixed;
      border-radius: 50%;
      filter: blur(80px);
      z-index: 0;
      pointer-events: none;
    }
    body::before {
      width: 520px; height: 520px;
      background: radial-gradient(circle, #b5d9a8 0%, transparent 70%);
      top: -120px; right: -120px;
      opacity: 0.55;
    }
    body::after {
      width: 400px; height: 400px;
      background: radial-gradient(circle, #d4e8c2 0%, transparent 70%);
      bottom: -80px; left: -80px;
      opacity: 0.45;
    }

    .page {
      position: relative;
      z-index: 1;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 24px 16px;
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(18px);
      -webkit-backdrop-filter: blur(18px);
      border: 1.5px solid var(--border);
      border-radius: 24px;
      box-shadow: var(--shadow), inset 0 1px 0 rgba(255,255,255,0.7);
      padding: 36px 32px 32px;
      width: 100%;
      max-width: 420px;
      animation: slideUp 0.55s cubic-bezier(0.16,1,0.3,1) both;
    }

    @keyframes slideUp {
      from { opacity:0; transform: translateY(28px); }
      to   { opacity:1; transform: translateY(0); }
    }

    .header { text-align: center; margin-bottom: 28px; }

    .logo {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 62px; height: 62px;
      background: linear-gradient(135deg, var(--leaf) 0%, var(--leaf-mid) 60%, var(--leaf-lite) 100%);
      border-radius: 18px;
      margin-bottom: 14px;
      box-shadow: 0 4px 20px rgba(45,90,39,0.28);
      position: relative;
      overflow: hidden;
    }
    .logo::after {
      content: '';
      position: absolute; inset: 0;
      background: linear-gradient(135deg, rgba(255,255,255,0.18) 0%, transparent 60%);
    }
    .logo svg { width: 32px; height: 32px; z-index: 1; }

    .title {
      font-family: 'Fraunces', serif;
      font-size: 22px;
      font-weight: 600;
      color: var(--soil);
      letter-spacing: -0.3px;
      line-height: 1.2;
    }
    .subtitle {
      font-size: 11px;
      color: var(--text-sub);
      letter-spacing: 0.08em;
      text-transform: uppercase;
      margin-top: 4px;
    }

    .status-pill {
      display: flex;
      align-items: center;
      gap: 8px;
      background: var(--mist);
      border: 1px solid var(--border);
      border-radius: 99px;
      padding: 8px 14px;
      font-size: 12px;
      color: var(--text-sub);
      margin-bottom: 24px;
      min-height: 38px;
      transition: all 0.3s ease;
    }
    .status-pill .dot {
      width: 8px; height: 8px;
      border-radius: 50%;
      background: var(--sun);
      flex-shrink: 0;
      animation: pulse 1.6s ease-in-out infinite;
    }
    .status-pill.ok  .dot { background: var(--leaf-mid); animation: none; }
    .status-pill.err .dot { background: var(--rust);     animation: none; }
    .status-pill.idle .dot{ background: #aaa;            animation: none; }

    @keyframes pulse {
      0%,100% { opacity:1; transform:scale(1); }
      50%      { opacity:0.5; transform:scale(1.35); }
    }

    .field { margin-bottom: 16px; }
    .field label {
      display: block;
      font-size: 11px;
      font-weight: 500;
      letter-spacing: 0.07em;
      text-transform: uppercase;
      color: var(--text-sub);
      margin-bottom: 6px;
    }

    .field select,
    .field input[type="password"],
    .field input[type="text"] {
      width: 100%;
      height: 46px;
      padding: 0 14px;
      background: rgba(255,255,255,0.9);
      border: 1.5px solid var(--border);
      border-radius: var(--radius);
      font-family: 'DM Mono', monospace;
      font-size: 13.5px;
      color: var(--text);
      outline: none;
      transition: border-color 0.2s, box-shadow 0.2s;
      appearance: none;
      -webkit-appearance: none;
    }
    .field select:focus,
    .field input:focus {
      border-color: var(--leaf-mid);
      box-shadow: 0 0 0 3px rgba(74,140,63,0.13);
    }

    .select-wrap { position: relative; }
    .select-wrap::after {
      content: '';
      position: absolute;
      right: 14px; top: 50%;
      transform: translateY(-50%);
      width: 0; height: 0;
      border-left: 5px solid transparent;
      border-right: 5px solid transparent;
      border-top: 6px solid var(--leaf);
      pointer-events: none;
    }

    .pass-wrap { position: relative; }
    .pass-wrap input { padding-right: 46px; }
    .pass-toggle {
      position: absolute;
      right: 14px; top: 50%;
      transform: translateY(-50%);
      background: none; border: none;
      cursor: pointer; padding: 4px;
      color: var(--text-sub);
      font-size: 17px; line-height: 1;
      transition: color 0.2s;
    }
    .pass-toggle:hover { color: var(--leaf); }

    .divider { height: 1px; background: var(--border); margin: 20px 0; }

    .btn-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }

    .btn {
      height: 46px;
      border: none;
      border-radius: var(--radius);
      font-family: 'DM Mono', monospace;
      font-size: 12.5px;
      font-weight: 500;
      letter-spacing: 0.04em;
      cursor: pointer;
      transition: transform 0.15s, box-shadow 0.15s, opacity 0.15s;
      display: flex; align-items: center; justify-content: center; gap: 7px;
    }
    .btn:active { transform: scale(0.97); }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }

    .btn-primary {
      background: linear-gradient(135deg, var(--leaf) 0%, var(--leaf-mid) 100%);
      color: #fff;
      box-shadow: 0 4px 16px rgba(45,90,39,0.25);
    }
    .btn-primary:hover:not(:disabled) {
      box-shadow: 0 6px 22px rgba(45,90,39,0.35);
      transform: translateY(-1px);
    }

    .btn-danger {
      background: rgba(192,57,43,0.08);
      color: var(--rust);
      border: 1.5px solid rgba(192,57,43,0.22);
    }
    .btn-danger:hover:not(:disabled) {
      background: rgba(192,57,43,0.14);
      transform: translateY(-1px);
    }

    .btn-scan {
      width: 100%;
      height: 38px;
      margin-top: 6px;
      background: rgba(74,140,63,0.09);
      color: var(--leaf);
      border: 1.5px solid rgba(74,140,63,0.22);
      border-radius: 10px;
      font-family: 'DM Mono', monospace;
      font-size: 11.5px;
      font-weight: 500;
      letter-spacing: 0.05em;
      cursor: pointer;
      transition: background 0.2s, transform 0.15s;
      display: flex; align-items: center; justify-content: center; gap: 6px;
    }
    .btn-scan:hover { background: rgba(74,140,63,0.16); transform: translateY(-1px); }
    .btn-scan:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }

    .footer-note {
      text-align: center;
      font-size: 10.5px;
      color: #aaa;
      margin-top: 18px;
      letter-spacing: 0.04em;
    }

    .spinner {
      display: inline-block;
      width: 13px; height: 13px;
      border: 2px solid rgba(255,255,255,0.4);
      border-top-color: #fff;
      border-radius: 50%;
      animation: spin 0.7s linear infinite;
      flex-shrink: 0;
    }
    .spinner.dark {
      border-color: rgba(45,90,39,0.25);
      border-top-color: var(--leaf);
    }
    @keyframes spin { to { transform: rotate(360deg); } }

    @media (max-width: 400px) {
      .card { padding: 28px 20px 24px; }
      .title { font-size: 19px; }
    }
  </style>
</head>
<body>
<div class="page">
  <div class="card">

    <div class="header">
      <div class="logo">
        <svg viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
          <path d="M16 28 C16 28 6 22 6 13 C6 8 10.5 4 16 4 C21.5 4 26 8 26 13 C26 22 16 28 16 28Z" fill="rgba(255,255,255,0.3)"/>
          <path d="M16 28 C16 20 10 16 7 12" stroke="rgba(255,255,255,0.6)" stroke-width="1.5" stroke-linecap="round"/>
          <path d="M16 28 C16 20 22 16 25 12" stroke="rgba(255,255,255,0.6)" stroke-width="1.5" stroke-linecap="round"/>
          <circle cx="16" cy="24" r="2" fill="rgba(255,255,255,0.5)"/>
        </svg>
      </div>
      <div class="title">Tưới Cây Thông Minh</div>
      <div class="subtitle">Cấu hình kết nối WiFi</div>
    </div>

    <div class="status-pill" id="statusPill">
      <span class="dot"></span>
      <span id="statusText">Đang khởi động...</span>
    </div>

    <div class="field">
      <label>Mạng WiFi</label>
      <div class="select-wrap">
        <select id="ssid">
          <option value="">-- Chọn mạng WiFi --</option>
        </select>
      </div>
      <button class="btn-scan" id="btnScan" onclick="scanWifi()">
        <span id="scanIcon">⟳</span>
        <span id="scanText">Quét lại danh sách</span>
      </button>
    </div>

    <div class="field">
      <label>Mật khẩu</label>
      <div class="pass-wrap">
        <input id="password" type="password" placeholder="Nhập mật khẩu WiFi..."/>
        <button class="pass-toggle" type="button" onclick="togglePassword()" id="eyeBtn">👁</button>
      </div>
    </div>

    <div class="divider"></div>

    <div class="btn-row">
      <button class="btn btn-primary" id="btnSave" onclick="saveWifi()">
        <span id="saveIcon">💾</span>
        <span id="saveText">Lưu & Kết nối</span>
      </button>
      <button class="btn btn-danger" onclick="reStart()">
        ↺ Khởi động lại
      </button>
    </div>

  </div>
  <div class="footer-note">ESP32 · PlantWater v2.0</div>
</div>

<script>
  const pill    = document.getElementById('statusPill');
  const pillTxt = document.getElementById('statusText');
  const btnSave = document.getElementById('btnSave');
  const btnScan = document.getElementById('btnScan');

  function setStatus(msg, type) {
    pillTxt.textContent = msg;
    pill.className = 'status-pill ' + (type || '');
  }

  function setSaving(on) {
    btnSave.disabled = on;
    document.getElementById('saveIcon').innerHTML = on ? '<span class="spinner"></span>' : '💾';
    document.getElementById('saveText').textContent = on ? 'Đang kết nối...' : 'Lưu & Kết nối';
  }

  function setScanning(on) {
    btnScan.disabled = on;
    document.getElementById('scanIcon').innerHTML = on ? '<span class="spinner dark"></span>' : '⟳';
    document.getElementById('scanText').textContent = on ? 'Đang quét...' : 'Quét lại danh sách';
  }

  function togglePassword() {
    const inp = document.getElementById('password');
    const btn = document.getElementById('eyeBtn');
    if (inp.type === 'password') { inp.type = 'text';     btn.textContent = '🙈'; }
    else                         { inp.type = 'password'; btn.textContent = '👁';  }
  }

  window.onload = scanWifi;

  function scanWifi() {
    setScanning(true);
    setStatus('Đang quét mạng WiFi...', '');
    const xhr = new XMLHttpRequest();
    xhr.timeout = 20000;
    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4) return;
      setScanning(false);
      if (xhr.status === 200) {
        const list = JSON.parse(xhr.responseText);
        const sel  = document.getElementById('ssid');
        sel.innerHTML = '<option value="">-- Chọn mạng WiFi --</option>';
        list.forEach(function(n) {
          const opt = document.createElement('option');
          opt.value = opt.textContent = n;
          sel.appendChild(opt);
        });
        setStatus('Tìm thấy ' + list.length + ' mạng WiFi', 'ok');
      } else {
        setStatus('Quét thất bại — nhấn quét lại', 'err');
      }
    };
    xhr.open('GET', '/scanWifi', true);
    xhr.send();
  }

  function saveWifi() {
    const ssid = document.getElementById('ssid').value;
    const pass = document.getElementById('password').value;
    if (!ssid) { setStatus('Vui lòng chọn mạng WiFi', 'err'); return; }
    setSaving(true);
    setStatus('Đang kết nối đến: ' + ssid, '');
    const xhr = new XMLHttpRequest();
    xhr.timeout = 30000;
    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4) return;
      setSaving(false);
      if (xhr.status === 200 && xhr.responseText === 'OK') {
        setStatus('Kết nối thành công! Đang khởi động lại...', 'ok');
        setTimeout(reStart, 1500);
      } else {
        setStatus('Sai mật khẩu hoặc không kết nối được', 'err');
      }
    };
    xhr.open('GET', '/saveWifi?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass), true);
    xhr.send();
  }

  function checkESP() {
    const xhr = new XMLHttpRequest();
    xhr.timeout = 3000;
    xhr.onreadystatechange = function () {
      if (xhr.readyState === 4) {
        if (xhr.status === 200) setStatus('ESP đã khởi động lại thành công', 'ok');
        else setTimeout(checkESP, 1500);
      }
    };
    xhr.open('GET', '/', true);
    xhr.send();
  }

  function reStart() {
    setStatus('Đang khởi động lại ESP32...', '');
    const xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState === 4) setTimeout(checkESP, 3000);
    };
    xhr.open('GET', '/reStart', true);
    xhr.send();
  }
</script>
</body>
</html>
)html";

void blinkLed(uint32_t t) {
  if (millis() - blinkTime > t) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    blinkTime = millis();
  }
}

void ledControl() {
  if (digitalRead(btnPin) == LOW) {
    if (millis() - lastTimePress < PUSHTIME) blinkLed(1000);
    else blinkLed(50);
  } else {
    if      (wifiMode == 0) blinkLed(50);
    else if (wifiMode == 1) blinkLed(3000);
    else if (wifiMode == 2) blinkLed(300);
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("Connected to WiFi");
      Serial.print("IP: "); Serial.println(WiFi.localIP());
      wifiMode = 1;
      needReconnect = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      wifiMode = 2;
      if (ssid != "") { needReconnect = true; reconnectTime = millis(); }
      break;
    default: break;
  }
}

void setupWifi() {
  Serial.println("Starting WiFi...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.onEvent(WiFiEvent);

  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  String ap = "ESP32-" + String(mac[4], HEX) + String(mac[5], HEX);
  ap.toUpperCase();
  WiFi.softAP(ap.c_str());
  Serial.println("AP: " + ap + " | IP: " + WiFi.softAPIP().toString());

  if (ssid != "") {
    Serial.println("Connecting: " + ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    wifiMode = 2;
  } else {
    Serial.println("No saved WiFi");
    wifiMode = 0;
  }
}

void setupWebServer() {
  webServer.on("/", [] {
    webServer.send(200, "text/html", html);
  });

  webServer.on("/scanWifi", [] {
    Serial.println("Scanning...");
    int nets = WiFi.scanNetworks(false, false);
    DynamicJsonDocument doc(1024);
    for (int i = 0; i < nets; ++i) {
      String s = WiFi.SSID(i);
      if (!s.length()) continue;
      bool dup = false;
      for (JsonVariant v : doc.as<JsonArray>())
        if (v.as<String>() == s) { dup = true; break; }
      if (!dup) doc.add(s);
    }
    String out; serializeJson(doc, out);
    webServer.send(200, "application/json", out);
  });

  webServer.on("/saveWifi", [] {
    String st = webServer.arg("ssid");
    String pt = webServer.arg("pass");
    Serial.println("Save: " + st);

    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false); delay(200);
    WiFi.begin(st.c_str(), pt.c_str());

    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis()-t < 12000) { delay(500); Serial.print("."); }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Retry...");
      WiFi.disconnect(false); delay(500);
      WiFi.begin(st.c_str(), pt.c_str());
      t = millis();
      while (WiFi.status() != WL_CONNECTED && millis()-t < 12000) { delay(500); Serial.print("."); }
      Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
      EEPROM.writeString(0, st); EEPROM.writeString(32, pt); EEPROM.commit();
      ssid = st; password = pt; wifiMode = 1; needReconnect = false;
      webServer.send(200, "text/plain", "OK");
    } else {
      webServer.send(200, "text/plain", "FAIL");
      if (ssid != "" && ssid != st) { WiFi.disconnect(false); delay(200); WiFi.begin(ssid.c_str(), password.c_str()); }
      else WiFi.disconnect(true);
    }
  });

  webServer.on("/reStart", [] {
    webServer.send(200, "text/plain", "OK");
    webServer.client().stop();
    delay(500); ESP.restart();
  });

  webServer.begin();
}

void checkButton() {
  if (digitalRead(btnPin) == LOW) {
    if (millis() - lastTimePress > PUSHTIME) {
      for (int i = 0; i < 100; i++) EEPROM.write(i, 0);
      EEPROM.commit();
      Serial.println("EEPROM cleared!");
      delay(2000); ESP.restart();
    }
    delay(1000);
  } else {
    lastTimePress = millis();
  }
}

class Config {
public:
  void begin() {
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT_PULLUP);
    blinker.attach_ms(50, ledControl);
    EEPROM.begin(100);
    char sb[32], pb[64];
    EEPROM.readString(0, sb, sizeof(sb));
    EEPROM.readString(32, pb, sizeof(pb));
    ssid = String(sb); password = String(pb);
    if (ssid != "") Serial.println("Saved WiFi: " + ssid);
    setupWifi();
    setupWebServer();
  }

  void run() {
    checkButton();
    webServer.handleClient();
    if (needReconnect && ssid != "" && millis()-reconnectTime >= RECONNECT_DELAY) {
      Serial.println("[WiFi] Reconnecting: " + ssid);
      WiFi.begin(ssid.c_str(), password.c_str());
      needReconnect = false;
      reconnectTime = millis();
    }
  }
} wifiConfig;