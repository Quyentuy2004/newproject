#include <HardwareSerial.h>

#define RX2 16  // bạn có thể dùng GPIO16
#define TX2 17  // bạn có thể dùng GPIO17

HardwareSerial moduleSim(2);  // UART2

String phone = "+84866249118";
unsigned long previousMillis = -10000;
const long delaySMS = 20000;

void send_SMS(String phone, String content) {
  moduleSim.println("AT+CMGS=\"" + phone + "\"");
  delay(3000);
  moduleSim.print(content);
  moduleSim.write(26); // Ctrl+Z
  delay(1000);
  Serial.println("Đã gửi tin nhắn đến: " + phone);
}

void setupModuleSim() {
  moduleSim.println("ATE0");
  delay(1000);
  moduleSim.println("AT+CMGF=1");
  delay(1000);
}

class SMS {
public:
  void begin() {
    Serial.begin(115200);
    moduleSim.begin(115200, SERIAL_8N1, RX2, TX2);  // tốc độ thường là 9600 cho SIM800L
    delay(5000); // đợi module khởi động
    setupModuleSim();
  }

  void run() {
    if (millis() - previousMillis >= delaySMS) {
      previousMillis = millis();
      send_SMS(phone, "Nhiet do da len hon 30 do!");
    }
  }
} guiSMS;