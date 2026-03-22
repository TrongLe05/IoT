#include <EEPROM.h> //Tên wifi và mật khẩu lưu vào ô nhớ 0->96
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h> //Thêm thư viện web server
WebServer webServer(80); //Khởi tạo đối tượng webServer port 80
#include <Ticker.h>
Ticker blinker;

String ssid;
String password;
#define ledPin 2
#define btnPin 0
unsigned long lastTimePress=millis();
#define PUSHTIME 5000
int wifiMode; //0:Chế độ cấu hình, 1:Chế độ kết nối, 2: Mất wifi
unsigned long blinkTime=millis();
//Tạo biến chứa mã nguồn trang web HTML để hiển thị trên trình duyệt
const char html[] PROGMEM = R"html( 
  <!doctype html>
  <html>
    <head lang="vi">
      <meta charset="utf-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1" />
      <title>Thiết bị tưới cây thông minh</title>

      <style>
        body {
          margin: 0;
          font-family: Arial, sans-serif;
          background: linear-gradient(135deg, #4caf50, #81c784);
          height: 100vh;
          display: flex;
          justify-content: center;
          align-items: center;
        }

        .container {
          background: white;
          padding: 25px;
          border-radius: 15px;
          box-shadow: 0 8px 25px rgba(0, 0, 0, 0.2);
          text-align: center;
        }

        h2 {
          margin-bottom: 10px;
          color: #2e7d32;
        }

        p {
          font-size: 14px;
          color: gray;
        }

        label {
          font-size: 14px;
          display: block;
          text-align: left;
          margin-top: 10px;
        }

        select,
        input {
          width: 100%;
          height: 40px;
          margin-top: 5px;
          margin-bottom: 10px;
          padding: 0 10px;

          border-radius: 8px;
          border: 1px solid #ccc;

          font-size: 16px;
          box-sizing: border-box;
        }

        button {
          width: 48%;
          height: 40px;
          margin-top: 15px;
          border: none;
          border-radius: 8px;
          color: white;
          cursor: pointer;
          font-weight: bold;
          transition: all 0.2s ease;
        }

        .password-box {
          position: relative;
          width: 100%;
        }

        .password-box input {
          width: 100%;
          height: 40px;
          padding: 0 40px 0 10px;
          border-radius: 8px;
          border: 1px solid #ccc;
          font-size: 16px;
          box-sizing: border-box;
        }

        /* icon con mắt */
        .toggle-pass {
          position: absolute;
          right: 10px;
          top: 50%;
          transform: translateY(-50%);
          cursor: pointer;
          font-size: 18px;
          color: #777;
          transition: 0.2s;
        }

        .toggle-pass:hover {
          color: #4caf50;
        }

        .btn-save {
          background: #4caf50;
        }

        .btn-restart {
          background: #f44336;
        }

        .btn-save:hover,
        .btn-restart:hover {
          transform: translateY(-1px);
          box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
        }

        .status {
          margin-top: 10px;
          font-size: 13px;
          color: #555;
        }

        .loading {
          color: orange;
          font-weight: bold;
        }
      </style>
    </head>

    <body>
      <div class="container">
        <h2>🌱 Hệ thống tưới cây thông minh</h2>
        <p id="info" class="loading">🔎 Đang tìm WiFi...</p>

        <label>Tên WiFi</label>
        <select id="ssid">
          <option>Loading...</option>
        </select>

        <label>Password</label>

        <div class="password-box">
          <input id="password" type="password" placeholder="Enter password" />
          <span class="toggle-pass" onclick="togglePassword()">👁️</span>
        </div>

        <div style="display: flex; justify-content: space-between">
          <button class="btn-save" onclick="saveWifi()">LƯU</button>
          <button class="btn-restart" onclick="reStart()">KHỞI ĐỘNG LẠI</button>
        </div>

        <div id="status" class="status"></div>
      </div>

      <script type="text/javascript">
        function togglePassword() {
          var input = document.getElementById("password");
          var icon = document.querySelector(".toggle-pass");

          if (input.type === "password") {
            input.type = "text";
            icon.innerHTML = "🙈";
          } else {
            input.type = "password";
            icon.innerHTML = "👁️";
          }
        }
        window.onload = function () {
          scanWifi();
        };
        var xhttp = new XMLHttpRequest();
        function scanWifi() {
          var xhr = new XMLHttpRequest(); // 🔥 tạo riêng

          xhr.onreadystatechange = function () {
            if (xhr.readyState == 4 && xhr.status == 200) {
              let data = xhr.responseText;

              document.getElementById("info").innerHTML =
                "✅ WiFi scan completed!";

              let obj = JSON.parse(data);
              let select = document.getElementById("ssid");

              select.innerHTML = ""; // 🔥 clear trước

              for (let i = 0; i < obj.length; ++i) {
                select[select.length] = new Option(obj[i], obj[i]);
              }
            }
          };

          xhr.open("GET", "/scanWifi", true);
          xhr.send();
        }
        function saveWifi() {
          var ssid = document.getElementById("ssid").value;
          var pass = document.getElementById("password").value;

          var xhr = new XMLHttpRequest();
                  
          xhr.onreadystatechange = function () {
            document.getElementById("info").innerHTML = 
            "⏳ ESP32 đang kết nối đến Wifi: " + ssid;
            if (xhr.readyState == 4 && xhr.status == 200) {
              if (xhr.responseText == "OK") {
                document.getElementById("password").value = "";
                document.getElementById("ssid").selectedIndex = 0;

                document.getElementById("info").innerHTML =
                  "✅ Kết nối thành công! ESP sẽ restart...";
                setTimeout(() => reStart(), 1000);
              } else {
                document.getElementById("info").innerHTML =
                  "❌ Sai mật khẩu hoặc không kết nối được!";
              }
            }
          };

          xhr.open("GET", "/saveWifi?ssid=" + ssid + "&pass=" + pass, true);
          xhr.send();
        }

        function checkESP(){
          var xhr = new XMLHttpRequest(); // 🔥 riêng

          xhr.onreadystatechange = function(){
            if(xhr.readyState==4){
              if(xhr.status==200){
                document.getElementById("info").innerHTML = "✅ ESP đã khởi động lại thành công!";
              }else{
                setTimeout(checkESP, 1000);
              }
            }
          }

          xhr.open("GET","/",true);
          xhr.send();
        }

        function reStart(){
          var xhr = new XMLHttpRequest(); // 🔥 riêng

          xhr.onreadystatechange = function(){
            if(xhr.readyState==4){
              document.getElementById("info").innerHTML = "🔄️ ESP đang khởi động lại...";
              setTimeout(checkESP, 2000);
            }
          }

          xhr.open("GET","/reStart",true);
          xhr.send();
        }
      </script>
    </body>
  </html>
)html";

void blinkLed(uint32_t t){
  if(millis()-blinkTime>t){
    digitalWrite(ledPin,!digitalRead(ledPin));
    blinkTime=millis();
  }
}

void ledControl(){
  if(digitalRead(btnPin)==LOW){
    if(millis()-lastTimePress<PUSHTIME){
      blinkLed(1000);
    }else{
      blinkLed(50);
    }
  }else{
    if(wifiMode==0){
      blinkLed(50);
    }else if(wifiMode==1){
      blinkLed(3000);
    }else if(wifiMode==2){
      blinkLed(300);
    }
  }
} 

//Chương trình xử lý sự kiện wifi
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("Connected to WiFi");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      wifiMode = 1; // ✅ đã kết nối thành công
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi");

      wifiMode = 2; // 🔄 đang reconnect

      WiFi.begin(ssid.c_str(), password.c_str());
      break;

    default:
      break;
  }
}

void setupWifi(){
  Serial.println("Starting WiFi...");

  // 🔥 Luôn bật cả 2 mode
  WiFi.mode(WIFI_AP_STA);

  // 🔥 Tạo AP (luôn có)
  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);
  String ssid_ap = "ESP32-" + String(macAddr[4], HEX) + String(macAddr[5], HEX);
  ssid_ap.toUpperCase();

  WiFi.softAP(ssid_ap.c_str());

  Serial.println("Access point name: " + ssid_ap);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // 🔥 Nếu có WiFi đã lưu → kết nối thêm STA
  if(ssid != ""){
    Serial.println("Connecting to WiFi: " + ssid);

    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.onEvent(WiFiEvent);

    wifiMode = 2; // đang thử kết nối
  } else {
    Serial.println("No saved WiFi → AP only mode");
    wifiMode = 0;
  }
}

void setupWebServer(){
  //Thiết lập xử lý các yêu cầu từ client(trình duyệt web)
  webServer.on("/",[]{
    webServer.send(200, "text/html", html); //Gửi nội dung HTML
  });
  webServer.on("/scanWifi",[]{
    Serial.println("Scanning wifi network...!");

    // 🔥 TẮT STA tạm để scan ổn định
    WiFi.disconnect(true);
    delay(100);

    int wifi_nets = WiFi.scanNetworks();

    DynamicJsonDocument doc(512);

    if(wifi_nets <= 0){
      Serial.println("No WiFi found!");
    } else {
      for(int i=0; i<wifi_nets; ++i){
        Serial.println(WiFi.SSID(i));
        doc.add(WiFi.SSID(i));
      }
    }

    String wifiList = "";
    serializeJson(doc, wifiList);

    webServer.send(200,"application/json",wifiList);

    // 🔥 connect lại nếu có wifi đã lưu
    if(ssid != ""){
      WiFi.begin(ssid.c_str(), password.c_str());
    }
  });
  webServer.on("/saveWifi",[]{
      String ssid_temp = webServer.arg("ssid");
      String password_temp = webServer.arg("pass");

      Serial.println("SSID:" + ssid_temp);
      Serial.println("PASS:" + password_temp);

      WiFi.mode(WIFI_AP_STA); // 🔥 GIỮ AP + KẾT NỐI STA
      WiFi.begin(ssid_temp.c_str(), password_temp.c_str());

      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 7000) {
          delay(500);
          Serial.print(".");
      }

      if (WiFi.status() == WL_CONNECTED) {
          EEPROM.writeString(0, ssid_temp);
          EEPROM.writeString(32, password_temp);
          EEPROM.commit();

          ssid = ssid_temp;
          password = password_temp;

          webServer.send(200, "text/plain", "OK");
      } else {
          webServer.send(200, "text/plain", "FAIL");

          // 🔥 QUAN TRỌNG: ngắt thử kết nối sai
          WiFi.disconnect(true);
      }
  });
  webServer.on("/reStart",[]{
      webServer.send(200,"text/plain","ESP restarting...");
      webServer.client().stop();  // 🔥 đảm bảo gửi response xong
      delay(1000);                // delay ngắn thôi
      ESP.restart();
  });
  webServer.begin(); //Khởi chạy dịch vụ web server trên ESP32
}

void checkButton(){
  if(digitalRead(btnPin)==LOW){
    Serial.println("Press and hold for 5 seconds to reset to default!");
    if(millis()-lastTimePress>PUSHTIME){
      for(int i=0; i<100;i++){
        EEPROM.write(i,0);
      }
      EEPROM.commit();
      Serial.println("EEPROM memory erased!");
      delay(2000);
      ESP.restart();
    }
    delay(1000);
  }else{
    lastTimePress=millis();
  }
}

class Config{
public:
  void begin(){
    pinMode(ledPin,OUTPUT);
    pinMode(btnPin,INPUT_PULLUP);
    blinker.attach_ms(50, ledControl);
    EEPROM.begin(100);
    char ssid_temp[32], password_temp[64];
    EEPROM.readString(0,ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32,password_temp,sizeof(password_temp));
    ssid = String(ssid_temp);
    password = String(password_temp);
    if(ssid!=""){
      Serial.println("Wifi name:"+ssid);
      Serial.println("Password:"+password);
    }
    setupWifi(); //Thiết lập wifi
    setupWebServer();
  }
  void run(){
    checkButton();
    webServer.handleClient(); // 🔥 bỏ điều kiện wifiMode
  }
} wifiConfig;
