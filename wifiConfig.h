#include <EEPROM.h> // Tên WiFi và mật khẩu lưu vào ô nhớ 0 -> 96
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h> // Thêm thư viện WebServer

#include <Ticker.h>
#include "dodht22.h"//dodht22.h: Thư viện riêng để đọc cảm biến DHT22.
Ticker blinker;


#define ledPin 2
#define btnPin 0
#define PUSHTIME 5000

WebServer webServer(80); // Khởi tạo đối tượng WebServer cổng 80

int wifiMode; // 0: Chế độ cấu hình, 1: Chế độ kết nối, 2: Mất WiFi
unsigned long blinkTime = millis();
unsigned long lastTimePress = millis();
String ssid;
String password;
float h;
float t;

//Tạo biến chứa mã nguồn trang web HTML để hiển thị trình hiển thị thông số Nhiệt độ và Độ ẩm 
const char html1[] PROGMEM = R"html(
  <!DOCTYPE html>
  <html lang="vi">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IoT - Giám sát Nhiệt độ và Độ ẩm</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
            color: #333;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2c3e50;
            text-align: center;
            margin-bottom: 30px;
        }
        .dashboard {
            display: flex;
            flex-wrap: wrap;
            gap: 20px;
            justify-content: center;
        }
        .card {
            background-color: #fff;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            transition: transform 0.3s ease;
            flex: 1;
            min-width: 250px;
        }
        .card:hover {
            transform: translateY(-5px);
        }
        .card-title {
            font-size: 18px;
            font-weight: bold;
            margin-bottom: 15px;
            color: #3498db;
            text-align: center;
        }
        .reading {
            font-size: 36px;
            font-weight: bold;
            text-align: center;
            margin: 20px 0;
        }
        .temperature {
            color: #e74c3c;
        }
        .humidity {
            color: #3498db;
        }
        .unit {
            font-size: 20px;
        }
        .simulated {
            text-align: center;
            margin-top: 10px;
            color: #7f8c8d;
            font-style: italic;
        }
        .sensor-icon {
            text-align: center;
            font-size: 24px;
            margin-bottom: 10px;
        }
        .section {
            margin-top: 30px;
            padding-top: 20px;
            border-top: 1px solid #eee;
        }
        .description {
            line-height: 1.6;
            margin-bottom: 20px;
        }
        .code-section {
            background-color: #f9f9f9;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 15px;
            margin: 20px 0;
            overflow-x: auto;
        }
        pre {
            margin: 0;
            white-space: pre-wrap;
        }
        code {
            font-family: monospace;
            color: #333;
        }
        .footer {
            text-align: center;
            margin-top: 30px;
            color: #7f8c8d;
            font-size: 14px;
        }
    </style>
  </head>
  <body>
    <div class="container">
        <h1>ESP32 IoT - Giám sát Nhiệt độ và Độ ẩm</h1>
        
        <div class="dashboard">
            <div class="card">
                <div class="sensor-icon">🌡️</div>
                <div class="card-title">Nhiệt độ</div>
                <div class="reading temperature" id="temperature">%TEMP%<span class="unit">°C</span></div>
            </div>
            
            <div class="card">
                <div class="sensor-icon">💧</div>
                <div class="card-title">Độ ẩm</div>
                <div class="reading humidity" id="humidity">%HUMI%<span class="unit">%</span></div>
            </div>
        </div>
        
        <div class="simulated">
            Đây là mô phỏng giao diện - dữ liệu thật từ ESP32 sẽ được hiển thị khi kết nối với thiết bị.
        </div>
        
        <div class="section">
            <h2>Mô tả dự án</h2>
            <div class="description">
                <p>Dự án này sử dụng ESP32 để tạo một thiết bị IoT đo nhiệt độ và độ ẩm. Thiết bị có các tính năng:</p>
                <ul>
                    <li>Cấu hình WiFi qua giao diện web</li>
                    <li>Đo nhiệt độ và độ ẩm sử dụng cảm biến DHT11</li>
                    <li>Hiển thị dữ liệu theo thời gian thực trên trang web</li>
                    <li>Lưu thông tin WiFi vào EEPROM để duy trì kết nối sau khi khởi động lại</li>
                    <li>LED trạng thái cho biết chế độ hoạt động của thiết bị</li>
                </ul>
            </div>
        </div>
        
        <div class="section">
            <h2>Chức năng chính</h2>
            <div class="code-section">
                <pre><code>
  // Chế độ Access Point
  - Khi khởi động lần đầu, ESP32 sẽ tạo một AP có tên "ESP32-xx"
  - Kết nối đến AP này để cấu hình Wi-Fi nhà bạn
  - Truy cập 192.168.4.1 để nhập SSID và mật khẩu Wi-Fi

  // Chế độ đo và hiển thị dữ liệu
  - Sau khi kết nối Wi-Fi thành công, ESP32 bắt đầu đọc cảm biến
  - Dữ liệu nhiệt độ và độ ẩm được cập nhật mỗi 2 giây
  - Truy cập IP của ESP32  để xem dữ liệu

  // Khôi phục cài đặt
  - Nhấn giữ nút RESET trong 5 giây để xóa thông tin Wi-Fi
  - Thiết bị sẽ khởi động lại ở chế độ Access Point
                </code></pre>
            </div>
        </div>
        
        <div class="footer">
            &copy; 2025 ESP32 IoT - Thiết bị giám sát nhiệt độ và độ ẩm
        </div>
    </div>
    
    <script>
    // Hàm setInterval() là hàm lặp lại trong 1 chu kỳ thời gian
      setInterval(function ( ) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (xhttp.readyState == 4 && xhttp.status == 200) {
              document.getElementById("temperature").innerHTML = xhttp.responseText + "<span class='unit'>°C</span>";
            }
        };
        xhttp.open("GET", "/temperature", true);
        xhttp.send();
      },2000 ) ;

      setInterval(function ( ) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (xhttp.readyState == 4 && xhttp.status == 200) {
           document.getElementById("humidity").innerHTML = xhttp.responseText + "<span class='unit'>%</span>";
          }
        };
        xhttp.open("GET", "/humidity", true);
        xhttp.send();
      }, 2000) ;
    </script>
 </body>
 </html>
)html";
// Tạo biến chứa mã nguồn trang web HTML để hiển thị trên trình duyệt
const char html[] PROGMEM = R"html( 
  <!DOCTYPE html>
  <html>
  <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>SETTING WIFI INFORMATION</title>
      <style type="text/css">
        body{display: flex; justify-content: center; align-items: center;}
        button{width: 135px; height: 40px; margin-top: 10px; border-radius: 5px}
        label, span{font-size: 25px;}
        input{margin-bottom: 10px; width: 275px; height: 30px; font-size: 17px;}
        select{margin-bottom: 10px; width: 280px; height: 30px; font-size: 17px;}
      </style>
  </head>
  <body>
    <div>
<h3 style="text-align: center;">SETTING WIFI INFORMATION</h3>
      <p id="info" style="text-align: center;">Scanning wifi network...!</p>
      <label>Wifi name:</label><br>
      <select id="ssid">
        <option>Choose wifi name!</option>
      </select><br>
      <label>Password:</label><br>
      <input id="password" type="text"><br>

      <button onclick="saveWifi()" style="background-color: cyan; margin-right: 10px">SAVE</button>
      <button onclick="reStart()" style="background-color: pink;">RESTART</button>
    </div>
    <script type="text/javascript">
      window.onload=function(){
        scanWifi();
      }
      var xhttp = new XMLHttpRequest();
      function scanWifi(){
        xhttp.onreadystatechange = function(){
          if(xhttp.readyState==4 && xhttp.status==200){
            data = xhttp.responseText;
            document.getElementById("info").innerHTML = "WiFi scan completed!";
            var obj = JSON.parse(data);
            var select = document.getElementById("ssid");
            for(var i=0; i<obj.length;++i){
              select[select.length] = new Option(obj[i], obj[i]);
            }
          }
        }
        xhttp.open("GET", "/scanWifi", true);
        xhttp.send();
      }
      function saveWifi(){
        ssid = document.getElementById("ssid").value;
        pass = document.getElementById("password").value;
        xhttp.onreadystatechange = function(){
          if(xhttp.readyState==4 && xhttp.status==200){
            data = xhttp.responseText;
            alert(data);
          }
        }
        xhttp.open("GET", "/saveWifi?ssid=" + ssid + "&pass=" + pass, true);
        xhttp.send();
      }
      function reStart(){
        xhttp.onreadystatechange = function(){
          if(xhttp.readyState==4 && xhttp.status==200){
            data = xhttp.responseText;
            alert(data);
          }
        }
        xhttp.open("GET", "/reStart", true);
        xhttp.send();
      }
    </script>
  </body>
  </html>
)html";

void blinkLed(uint32_t t){
  if(millis() - blinkTime > t){
    digitalWrite(ledPin, !digitalRead(ledPin));
    blinkTime = millis();
  }
}

void ledControl(){
  if(digitalRead(btnPin) == LOW){
    if(millis() - lastTimePress < PUSHTIME){
      blinkLed(1000);
    }else{
      blinkLed(50);
    }
  }else{
    if(wifiMode == 0){
      blinkLed(50);
    }else if(wifiMode == 1){
      blinkLed(3000);
    }else if(wifiMode == 2){
      blinkLed(300);
    }
  }
}

// Xử lý sự kiện WiFi
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
        // Sự kiện khi ESP32 đã kết nối với AP thành công (chưa lấy IP)
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected!");
             wifiMode = 1; 
            break;

        // Sự kiện khi ESP32 bị mất kết nối WiFi
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            
                // Nếu số lần mất kết nối < 10 lần thì thử kết nối lại Wi-Fi
                Serial.println("WiFi lost connection.");
                wifiMode = 2; // Đặt chế độ Wi-Fi về 2 (đang cố gắng reconnect)
                       // Đợi ngắt kết nối hoàn tất
                WiFi.begin(ssid, password); // Gọi lại WiFi.begin để kết nối lại
                delay(2000);
            break;

        

        // Mặc định khi không khớp với bất kỳ sự kiện nào ở trên
        default:
            Serial.print("No ");
            break;
  }
}

void setupWifi(){

  WiFi.onEvent(WiFiEvent); // Đăng ký chương trình bắt sự kiện WiFi
  if(ssid != ""){
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Đang kết nối tới WiFi");
    // Vòng lặp chờ cho đến khi kết nối thành công
       // Vòng lặp chờ đến khi kết nối thành công
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
       wifiMode = 1;
    }
    Serial.print("📡 Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("\n✅ Đã hoàn thành kết nối WiFi!");
  }else{
    Serial.println("ESP32 WiFi network created!");
    WiFi.mode(WIFI_AP);
    String ssid_ap = "ESP32" ;
    WiFi.softAP(ssid_ap.c_str());
    Serial.println("Access Point name: " + ssid_ap);
    Serial.println("Web server access address: " + WiFi.softAPIP().toString());
    wifiMode = 0;
  }
}

void setupWebServer(){
  // Thiết lập xử lý các yêu cầu từ client (trình duyệt web)
  if (WiFi.status() == WL_CONNECTED) {
    // Thiết lập route "/" trả về giao diện giám sát html1
    webServer.on("/", [] {
      webServer.send(200, "text/html", html1);
      });
      
      // Thiết lập API đọc nhiệt độ từ DHT22, trả về `dạng text
      webServer.on("/temperature", HTTP_GET, [] {
        webServer.send(200, "text/plain", String(t));
      });

      // Thiết lập API đọc độ ẩm từ DHT22, trả về dạng text
      webServer.on("/humidity", HTTP_GET, [] {
        webServer.send(200, "text/plain", String(h));
      });
   

    delay(2000);  // Delay nhẹ để ổn định hệ thống
    Serial.println("✅ Web server started (STA mode)");
    } 
  else {
    webServer.on("/", []{
      webServer.send(200, "text/html", html);
    });

     webServer.on("/scanWifi", []{
       Serial.println("Scanning WiFi network...");
       int wifi_nets = WiFi.scanNetworks(true, true);
       const unsigned long t = millis();
       while(wifi_nets < 0 && millis() - t < 10000){
         delay(20);
         wifi_nets = WiFi.scanComplete();
        }
       DynamicJsonDocument doc(200);
       for(int i = 0; i < wifi_nets; ++i){
          Serial.println(WiFi.SSID(i));
          doc.add(WiFi.SSID(i));
        }
       String wifiList = "";
       serializeJson(doc, wifiList);
       Serial.println("WiFi list: " + wifiList);
       webServer.send(200, "application/json", wifiList);
      });

   // Route "/saveWifi" lưu SSID và mật khẩu vào EEPROM
    webServer.on("/saveWifi", []() {
      String ssid_temp = webServer.arg("ssid");
      String password_temp = webServer.arg("pass");
      Serial.println("SSID:"+ssid_temp);
      Serial.println("PASS:"+password_temp);
      EEPROM.writeString(0,ssid_temp);
      EEPROM.writeString(32,password_temp);
      EEPROM.commit();
      webServer.send(200,"text/plain","Wifi has been saved!");
    });

   webServer.on("/reStart", []{
    webServer.send(200, "text/plain", "ESP32 is restarting!");
    delay(3000);
    ESP.restart();
   });
  }
  webServer.begin(); // Khởi chạy dịch vụ WebServer trên ESP32
}

void checkButton(){
  if(digitalRead(btnPin) == LOW){
    Serial.println("Nhấn và giữ 5 giây để reset về mặc định!");
    if(millis() - lastTimePress > PUSHTIME){
      for(int i = 0; i < 100; i++){
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.println("Đã xóa dữ liệu EEPROM!");
      delay(2000);
      ESP.restart();
    }
    delay(1000);
  }else{
    lastTimePress = millis();
  }
}

class Config {
public:
  void begin(){
     DHT22.begin();// cấu hình dht22 
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT_PULLUP);
    blinker.attach_ms(50, ledControl);
    
     EEPROM.begin(100);
    char ssid_temp[32], password_temp[64];
    EEPROM.readString(0,ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32,password_temp,sizeof(password_temp));
    ssid = String(ssid_temp);
    password = String(password_temp);


    if(ssid != ""){
      Serial.println("WiFi name: " + ssid);
      Serial.println("Password: " + password);
    }
    setupWifi(); // Thiết lập WiFi
    if(wifiMode == 0||wifiMode == 1 ) setupWebServer(); // Thiết lập WebServer
     delay(100);  // Đợi WiFi khởi động
  }

  void run(){
    checkButton();
    if(wifiMode == 0 ||wifiMode == 1 ) webServer.handleClient(); // Lắng nghe yêu cầu từ client
    if (WiFi.status() == WL_CONNECTED) {
     t = DHT22.runTemp();
     h = DHT22.runHum();

  }
  }
} wifiConfig;
