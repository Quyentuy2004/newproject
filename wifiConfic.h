
#include <EEPROM.h> //Tên wifi và mật khẩu lưu vào ô nhớ 0->96
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h> //WebServer: Tạo web server để xử lý yêu cầu HTTP.
#include <Ticker.h>//Ticker: Tạo các tác vụ định kỳ (dùng cho nháy LED).
#include "dodht22.h"//dodht22.h: Thư viện riêng để đọc cảm biến DHT22.


#include <HTTPClient.h>
String Web_App_URL = "https://script.google.com/macros/s/AKfycbwYjRgvJWVResLPTg9Q5l83GIDQe2DTmwUF17Zkaxaog1bjVR3zU_qkFiE-OQ8iAOs/exec";
//https://script.google.com/macros/s/AKfycbzz5DVJuy-aRHueeO1nFMzWUQ3wLT82HoNlxX7CY3b-mYG8xQueVal4YWoTO02axYY/exec
unsigned long currentTime=millis();
unsigned long currentTime2 = millis();
 float temp1 ;   
 float humi1 ;
float tempWar=35;



#define ledPin 2
#define btnPin 0
#define PUSHTIME 5000

WebServer webServer(80); //Khởi tạo đối tượng webServer port 80


int wifiMode; // 0: Chế độ cấu hình web , 1: Chế độ kết nối, 2: Mất wifi
unsigned long blinkTime = millis();
int checklostwifi=0;//checklostwifi: Nếu mất Wi-Fi hơn 10 lần thì xóa EEPROM và reset.
unsigned long lastTimePress = millis();//lastTimePress: Dùng kiểm tra thời gian giữ nút để reset Wi-Fi.
Ticker blinker;
String ssid;// biến toàn cục lưu thông tin ssid của wifi
String password;//biến toàn cục lưu thông tin password của wifi
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

//Tạo biến chứa mã nguồn trang web HTML để hiển thị trình duyệt kết nối wifi bằng chế độ AP
const char html[] PROGMEM = R"html( 
  <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>SETTING WIFI INFORMATION</title>
        <style type="text/css">
          body{display: flex;justify-content: center;align-items: center;}
          button{width: 135px;height: 40px;margin-top: 10px;border-radius: 5px}
          label, span{font-size: 25px;}
          input{margin-bottom: 10px;width:275px;height: 30px;font-size: 17px;}
          select{margin-bottom: 10px;width: 280px;height: 30px;font-size: 17px;}
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

        <button onclick="saveWifi()" style="background-color: cyan;margin-right: 10px">SAVE</button>
        <button onclick="reStart()" style="background-color: pink;">RESTART</button>
      </div>
        <script type="text/javascript">
          window.onload=function(){
            scanWifi();
          }
          var xhttp = new XMLHttpRequest();
          
          function scanWifi(){
            xhttp.onreadystatechange = function(){
              if (xhttp.readyState == 4) {
                console.log ("qua buoc 1");
                if (xhttp.status == 200) {
                  console.log("Thành công buoc 2:", xhttp.responseText);
                  data = xhttp.responseText; //xhttp.responseText chứa danh sách SSID dưới dạng chuỗi JSON.
                  document.getElementById("info").innerHTML = "WiFi scan completed!";
                  var obj = JSON.parse(data);       //Chuyển đổi JSON thành object với JSON.parse(data).  
                  var select = document.getElementById("ssid");
                  for(var i=0; i<obj.length;++i){
                   select[select.length] = new Option(obj[i],obj[i]);//
            
                  }
                }
                else if (xhttp.status == 404) {
                  console.error("Lỗi 404: Tài nguyên không tìm thấy!");
                  alert("Lỗi 404: Trang không tồn tại. Vui lòng kiểm tra lại URL.");
                }
                else {
                  console.error("Lỗi khác: " + xhttp.status);
                }
              }
            }
            xhttp.open("GET","/scanWifi",true);
            xhttp.send();
          }
      
          function saveWifi(){
            ssid = document.getElementById("ssid").value;
            pass = document.getElementById("password").value;
            xhttp.onreadystatechange = function(){
              if(xhttp.readyState==4&&xhttp.status==200){
                data = xhttp.responseText;
                alert(data);
              }
            }
            xhttp.open("GET","/saveWifi?ssid="+ssid+"&pass="+pass,true);
            xhttp.send();
          }
          function reStart(){
             xhttp.onreadystatechange = function(){
              if(xhttp.readyState==4&&xhttp.status==200){
                data = xhttp.responseText;
                alert(data);
              }
            }
            xhttp.open("GET","/reStart",true);
            xhttp.send();
          }
        </script>
    </body>
  </html>
)html";

// Hàm quét các mạng Wi-Fi xung quanh và gửi danh sách SSID về client dưới dạng JSON
void scanWiFiNetworks() {
    Serial.println("Scanning WiFi..."); // In ra Serial để báo bắt đầu quét Wi-Fi

    int numNetworks = WiFi.scanNetworks(); // Quét và trả về số mạng Wi-Fi tìm thấy

    String json = "["; // Khởi tạo chuỗi JSON dạng mảng

    for (int i = 0; i < numNetworks; i++) {
        if (i) json += ","; // Nếu không phải phần tử đầu tiên thì thêm dấu phẩy phân cách

        // Thêm SSID của mạng thứ i vào chuỗi JSON, được đặt trong dấu ngoặc kép
        json += "\"" + WiFi.SSID(i) + "\"";
    }

    json += "]"; // Kết thúc mảng JSON

    Serial.println(json); // In ra Serial chuỗi JSON kết quả

    // Gửi chuỗi JSON về trình duyệt với mã HTTP 200 và kiểu nội dung là "application/json"
    webServer.send(200, "application/json", json);
}


// Hàm nhấp nháy LED với chu kỳ t (ms)
// t: thời gian giữa hai lần đổi trạng thái LED (tính bằng mili giây)
void blinkLed(uint32_t t) {
  // Kiểm tra nếu thời gian đã trôi qua lớn hơn t kể từ lần nháy trước
  if (millis() - blinkTime > t) {
    // Đảo trạng thái chân ledPin (nếu đang HIGH thì chuyển LOW và ngược lại)
    digitalWrite(ledPin, !digitalRead(ledPin));
    
    // Cập nhật thời điểm nháy LED gần nhất
    blinkTime = millis();
  }
}

// Hàm điều khiển nháy LED theo trạng thái nút nhấn và chế độ Wi-Fi
void ledControl() {
  // Nếu nút đang được nhấn (LOW vì nút nối với GND)
  if (digitalRead(btnPin) == LOW) {
    
    // Kiểm tra nếu thời gian nhấn ngắn hơn ngưỡng PUSHTIME
    if (millis() - lastTimePress < PUSHTIME) {
      // Nháy LED chậm (1 giây) - biểu thị nhấn ngắn
      blinkLed(1000);
    } else {
      // Nháy LED nhanh (50ms) - biểu thị nhấn giữ lâu
      blinkLed(50);
    }

  } else {
    // Nếu không nhấn nút, LED sẽ nháy theo chế độ Wi-Fi hiện tại

    if (wifiMode == 0) {
      // Chế độ 0: AP mode — nháy LED nhanh (50ms)
      blinkLed(50);

    } else if (wifiMode == 1) {
      // Chế độ 1: STA chưa kết nối — nháy LED chậm (3 giây)
      blinkLed(3000);

    } else if (wifiMode == 2) {
      // Chế độ 2: STA đã kết nối — nháy LED vừa (300ms)
      blinkLed(300);
    }
  }
}

// Hàm xử lý các sự kiện WiFi, tự động gọi khi có sự kiện xảy ra
void WiFiEvent(WiFiEvent_t event) {
   

    switch (event) {
        // Sự kiện khi ESP32 đã kết nối với AP thành công (chưa lấy IP)
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected!");
             wifiMode = 0; 
            break;

        // Sự kiện khi ESP32 bị mất kết nối WiFi
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            checklostwifi+=1;
                // Nếu số lần mất kết nối < 10 lần thì thử kết nối lại Wi-Fi
               if (checklostwifi==10){
                // Xoá toàn bộ vùng nhớ EEPROM (giả sử 100 byte được dùng để lưu SSID, password,...)
      for (int i = 0; i < 100; i++) {
        EEPROM.write(i, 0);  // Ghi giá trị 0 vào từng ô nhớ
      }

      EEPROM.commit();  // Lưu thay đổi vào bộ nhớ thực tế
      Serial.println("✅ EEPROM memory erased!");

      delay(2000);       // Đợi 2 giây trước khi khởi động lại
      ESP.restart();     // Khởi động lại ESP32 để vào chế độ cấu hình Wi-Fi
                checklostwifi=0;
               }
                Serial.println("WiFi lost connection.");
                wifiMode = 2; // Đặt chế độ Wi-Fi về 2 (đang cố gắng reconnect)
                       // Đợi ngắt kết nối hoàn tất
                WiFi.begin(ssid, password); // Gọi lại WiFi.begin để kết nối lại
                
    

   
            
            break;

        // Sự kiện khi ESP32 đã nhận được địa chỉ IP từ Wi-Fi
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP()); // In địa chỉ IP đã nhận được ra Serial
            break;

        // Mặc định khi không khớp với bất kỳ sự kiện nào ở trên
        default:
            Serial.print("No ");
            break;
    }
}

void setupWifi(){
  

  // Đăng ký hàm xử lý sự kiện WiFi, chỉ cần gọi một lần duy nhất
  WiFi.onEvent(WiFiEvent); // Hàm WiFiEvent sẽ được gọi mỗi khi có sự kiện WiFi xảy ra

  // Nếu SSID hợp lệ (người dùng đã cấu hình trước đó)
  if(ssid.length() > 0){
    Serial.println("Connecting to wifi...!");
    WiFi.mode(WIFI_STA);               // Chuyển ESP32 sang chế độ Station (kết nối WiFi)
    WiFi.begin(ssid, password);        // Kết nối đến WiFi với SSID và mật khẩu đã lưu

    Serial.print("Đang kết nối tới WiFi");
    // Vòng lặp chờ cho đến khi kết nối thành công
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }

    Serial.println("\n✅ Đã hoàn thành kết nối WiFi!");
    checklostwifi = 0;  // Reset bộ đếm mất kết nối
  } 
  else {
    // Nếu không có SSID trong EEPROM, khởi tạo chế độ Access Point để người dùng cấu hình WiFi
    Serial.println("🚀 ESP32 wifi network created!");
    WiFi.mode(WIFI_AP);  // Chuyển sang chế độ Access Point

    

    // Tạo tên SSID cho AP dựa trên 2 byte cuối của địa chỉ MAC
    String ssid_ap = "ESP32" ;
    

    // Khởi động Access Point với SSID mới tạo
    WiFi.softAP(ssid_ap.c_str());

    Serial.println("Tên mạng Access Point: " + ssid_ap);
    Serial.println("Truy cập Web Server tại địa chỉ: " + WiFi.softAPIP().toString());

    wifiMode = 0; // Đặt trạng thái WiFi = 0 (chế độ AP)
  }
}








// Hàm thiết lập WebServer cho ESP32
void setupWebServer() {
  // Nếu ESP32 đang kết nối Wi-Fi (chế độ STA)
  if (WiFi.status() == WL_CONNECTED) {
    
    // Thiết lập route "/" trả về giao diện giám sát html1
    webServer.on("/", [] {
      webServer.send(200, "text/html", html1);
    });
      
      // Thiết lập API đọc nhiệt độ từ DHT22, trả về dạng text
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
  else {  // Nếu chưa kết nối Wi-Fi (chế độ AP để cấu hình Wi-Fi)

    // Route "/" trả về giao diện cấu hình Wi-Fi html
    webServer.on("/", [] {
      webServer.send(200, "text/html", html);
    });

    // Route "/scanWifi" gọi hàm quét mạng Wi-Fi và trả danh sách SSID
    webServer.on("/scanWifi", scanWiFiNetworks);

    // Route "/reStart" để khởi động lại ESP32
    webServer.on("/reStart", [] {
      webServer.send(200, "text/plain", "Esp32 is restarting!");
      delay(3000);
      ESP.restart();  // Khởi động lại
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
    webServer.send(200,"text/plain","Wifi has been saved!");// Kiểm tra nếu có đủ thông tin từ người dùng

    });
  }

  // Khởi động WebServer
  webServer.begin();
}

// Hàm kiểm tra nút nhấn để khôi phục cấu hình Wi-Fi mặc định nếu giữ nút đủ lâu
void checkButton() {
  // Kiểm tra nếu nút được nhấn (mức LOW do nút kéo xuống GND)
  if (digitalRead(btnPin) == LOW) {
    Serial.println("Press and hold for 5 seconds to reset to default!");

    // Nếu nút được nhấn liên tục quá thời gian định nghĩa (PUSHTIME)
    if (millis() - lastTimePress > PUSHTIME) {
      // Xoá toàn bộ vùng nhớ EEPROM (giả sử 100 byte được dùng để lưu SSID, password,...)
      for (int i = 0; i < 100; i++) {
        EEPROM.write(i, 0);  // Ghi giá trị 0 vào từng ô nhớ
      }

      EEPROM.commit();  // Lưu thay đổi vào bộ nhớ thực tế
      Serial.println("✅ EEPROM memory erased!");

      delay(2000);       // Đợi 2 giây trước khi khởi động lại
      ESP.restart();     // Khởi động lại ESP32 để vào chế độ cấu hình Wi-Fi
    }

    delay(1000);  // Đợi thêm 1 giây để tránh đọc nhấn nhiều lần liên tục
  } else {
    // Nếu nút không nhấn, cập nhật thời gian cuối cùng nút được nhả ra
    lastTimePress = millis();
  }
}

void writeSheet(){
  String Send_Data_URL = Web_App_URL + "?sts=write";
  Send_Data_URL += "&temp=" + String(temp1);
  Send_Data_URL += "&humi=" + String(humi1);
  //Serial.println();
  //Serial.println("-------------");
 // Serial.println("Send data to Google Spreadsheet...");
 // Serial.print("URL : ");
  //Serial.println(Send_Data_URL);
  // Initialize HTTPClient as "http".
  HTTPClient http;

  // HTTP GET Request.
  http.begin(Send_Data_URL.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Gets the HTTP status code.
  int httpCode = http.GET(); 
 // Serial.print("HTTP Status Code : ");
 // Serial.println(httpCode);

  // Getting response from google sheets.
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
   // Serial.println("Payload : " + payload);    
  }
  
  http.end();
}
String dataForm(float value, int leng, int decimal){
  String str = String(value,decimal);
  if(str.length()<leng){
    int space = leng-str.length();
    for(int i=0;i<space;++i){
      str = " "+str;
    }
  }
  return str;
}



class Config{
public:
  void begin(){
    DHT22.begin();// cấu hình dht22 
    pinMode(ledPin,OUTPUT);// cấu hình led để hiển thị trạng thái Wifi
    pinMode(btnPin,INPUT_PULLUP);
    blinker.attach_ms(50, ledControl);
      EEPROM.begin(100);
    char ssid_temp[32], password_temp[64];
    EEPROM.readString(0,ssid_temp, sizeof(ssid_temp));
    EEPROM.readString(32,password_temp,sizeof(password_temp));
    ssid = String(ssid_temp);
    password = String(password_temp);

    setupWifi();// cấu hình Wifi
    if(wifiMode==0) setupWebServer();
    delay(100);  // Đợi WiFi khởi động
  }
  void run(){
    checkButton();
    if(wifiMode==0)webServer.handleClient();
    if (WiFi.status() == WL_CONNECTED) {
     t = DHT22.runTemp();
     h = DHT22.runHum();
      temp1 = t;
      humi1 = h;
 if(millis()-currentTime2>180000){
    writeSheet();
    currentTime2=millis();
  }
  if ( t >= tempWar) {
      digitalWrite(LEDWAR, HIGH);
      digitalWrite(COIWAR, HIGH);
     // guiSMS.run();
    }
   else {
    digitalWrite(LEDWAR, LOW);
    digitalWrite(COIWAR, LOW);
   }
  }
  }
} wifiConfig;

