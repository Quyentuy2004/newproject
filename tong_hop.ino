#include<WiFi.h>
#include<Firebase_ESP_Client.h>
#include"addons/TokenHelper.h"
#include"addons/RTDBHelper.h"
#include"dodht22.h"
#include "time.h"

// #define Wifi_SSID "TP-Link_12A4"
// #define Wifi_PASSWORD "25311839"

// #define Wifi_SSID "TP-LINK_8D46"
// #define Wifi_PASSWORD "62037891"

#define Wifi_SSID "Phuong Anh"
#define Wifi_PASSWORD "20062023"
#define API_KEY "AIzaSyB_aDU1LR4WPYFR7DPktUHW3tQdmKxYkuM"
#define DATABASE_URL "https://doan22-e15f5-default-rtdb.asia-southeast1.firebasedatabase.app/"


FirebaseData fbdo, fbdo_s2, fbdo_s1,fbdo_s3;
FirebaseAuth auth;
FirebaseConfig config;

struct tm timeinfo;
unsigned long deleteHisMillis = 0;
unsigned long deleteWarMillis = 0;
unsigned long sendDataPrevMillis = 0;
unsigned long prevMillisMain = 0;
unsigned long prevMillisHis = 0;
unsigned long prevMillisWar = 0;
unsigned long sendDataWarning=0;
unsigned long countHis = 0;
int checktime=0;
unsigned long countWar=0;
bool signupOk = false;
float Temp=0.0;
float Hum=0.0;
unsigned long lineSet= 0;
unsigned long timeSetHis=0;
int retry = 0;
float tempSet=0.0;
time_t now;
// Máy chủ NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 cho Việt Nam
const int   daylightOffset_sec = 0;


String chuyenDoi (int time){
  String timeNew;
  if (time<10){
    timeNew="0"+String(time);
  }
  else{
    timeNew=String(time);
  }
  return timeNew;
}

void setTimeHis(){
 if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s2))
    Serial.printf("stream 2 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
    if (fbdo_s2.streamAvailable()){
      if(fbdo_s2.dataType()=="int"){
        timeSetHis=fbdo_s2.intData()*1000;
        Serial.println("Successfull read from"+fbdo_s2.dataPath()+ timeSetHis+"("+fbdo_s2.dataType()+")");
     }
   }
  }
}

void getTime(){
 
 retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 20) {
    Serial.println("⏳ Đang đồng bộ thời gian...");
    delay(300);
    retry++;
  }

  if (retry == 20) {
    Serial.println("❌ Không thể lấy thời gian!");
  } else {
    Serial.println("✅ Lấy thời gian thành công!");

    // Lấy luôn epoch time
    time(&now);
    if (now > 100000) {
      Serial.print("Epoch time: ");
      Serial.println(now);
    } else {
      Serial.println("⚠️ Epoch chưa hợp lệ!");
    }
  }
}

void checkLineHis() {
  if (Firebase.RTDB.getInt(&fbdo, "/Count/CountHis")) {
   countHis=fbdo.intData();
   if(countHis > 5){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/History", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/History/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countHis = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/Count/CountHis", countHis);
   }
 }
}
void checkLineWar(){  
 if (Firebase.RTDB.getInt(&fbdo, "/Count/CountWar")) {
   countWar=fbdo.intData();
   if(countWar > 5){
    QueryFilter query;
    query.orderBy("$key"); 
    query.limitToFirst(1);  // chỉ lấy 1 bản ghi cũ nhất

     if (Firebase.RTDB.getJSON(&fbdo, "/WarHistory", &query)) {
       FirebaseJson json = fbdo.jsonObject();
       size_t count = json.iteratorBegin();   
       String key, value;
       int type;
       json.iteratorGet(0, type, key, value);
       Serial.printf("Key: %s, Value: %s\n", key.c_str(), value.c_str());
       Firebase.RTDB.deleteNode(&fbdo, "/WarHistory/" + key);
       json.iteratorEnd();
       query.clear();
      }
     countWar = fbdo.intData()-1;
     Firebase.RTDB.setInt(&fbdo, "/Count/CountWar", countWar);
   }
 }
}
void hisdatabase(float hum,float temp){
 if (Firebase.ready()&& signupOk &&((millis()-prevMillisHis)>timeSetHis|| prevMillisHis==0)){
  prevMillisHis=millis();  
  getTime();
  //getTimeEpoch();
  String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
              chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
              chuyenDoi(timeinfo.tm_hour) + ":" + 
              chuyenDoi(timeinfo.tm_min) + ":" + 
              chuyenDoi(timeinfo.tm_sec);
  Serial.println(time);
  FirebaseJson json;
  json.set("Time", time);
  json.set("Temp", temp);
  json.set("Hum", hum);

  String path = "History/" + String(now);
   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      
      if (Firebase.RTDB.getInt(&fbdo, "/Count/CountHis")) {
        countHis = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/Count/CountHis", countHis);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    
 }
}

void getTempWar(){
  if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s1))
     Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
    if (fbdo_s1.streamAvailable()){
      if(fbdo_s1.dataType()=="string"){
        sendDataWarning=0;
        tempSet=fbdo_s1.stringData().toFloat();
        Serial.println("Successfull read from"+fbdo_s1.dataPath()+ String(tempSet) +"("+fbdo_s1.dataType()+")");
      }
    }
  }
}

void getLine(){
  if (Firebase.ready()&& signupOk){
    if(!Firebase.RTDB.readStream(&fbdo_s3))
     Serial.printf("stream 3  begin error,%s\n\n", fbdo_s3.errorReason().c_str());
    if (fbdo_s3.streamAvailable()){
      if(fbdo_s3.dataType()=="int"){
        lineSet=fbdo_s3.intData();
        Serial.println("Successfull read from"+fbdo_s3.dataPath()+ String(lineSet) +"("+fbdo_s3.dataType()+")");
        lineSet*=4;
      }
    }
  }
}

void warDatabase(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-prevMillisWar)>5000|| prevMillisWar==0)){
   prevMillisWar=millis();
   if (temp>tempSet &&((millis()-sendDataWarning)>50000|| sendDataWarning==0)) {
    sendDataWarning=millis();
    getTime();
  //  getTimeEpoch();
 String time = chuyenDoi(timeinfo.tm_mday) + "-" + 
              chuyenDoi(timeinfo.tm_mon + 1) + "_" +   // tm_mon bắt đầu từ 0 → cộng 1
              chuyenDoi(timeinfo.tm_hour) + ":" + 
              chuyenDoi(timeinfo.tm_min) + ":" + 
              chuyenDoi(timeinfo.tm_sec);
    FirebaseJson json;
    json.set("Time", time);
    json.set("TempWar", tempSet);
    json.set("Temp", temp);
 
  String path = "WarHistory/" + String(now);
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(),&json)) {
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
      if (Firebase.RTDB.getInt(&fbdo, "/Count/CountWar")) {
        countWar = fbdo.intData() + 1;
        Firebase.RTDB.setInt(&fbdo, "/Count/CountWar", countWar);
      }
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
 }
}
void truyenChinh(float hum,float temp){
  if (Firebase.ready()&& signupOk &&((millis()-sendDataPrevMillis)>5000|| sendDataPrevMillis==0)){
  sendDataPrevMillis=millis();
  if (Firebase.RTDB.setFloat(&fbdo, "Sensor/temp", temp)) {
      Serial.println();
      Serial.print(temp);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/hum", hum)) {
      Serial.println();
      Serial.print(hum);
      Serial.print(" - successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
 }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Connection Wifi
  WiFi.begin(Wifi_SSID,Wifi_PASSWORD);
  Serial.print("conecting to wifi");
  while(WiFi.status()!= WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP:");
  Serial.println(WiFi.localIP());
  Serial.println();
 // connection firebase
  config.api_key=API_KEY;
  config.database_url=DATABASE_URL;
  if (Firebase.signUp(&config,&auth,"","")){
    Serial.println("singup ok");
    signupOk=true;
  }else{
    Serial.printf("%s\n",config.signer.signupError.message.c_str());
  }
  config.token_status_callback= tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.begin(&config, &auth);

  // setup dht22
  DHT22.begin();
  delay(1500);
  // Cấu hình NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
}
  // cau hinh stream
if (!Firebase.RTDB.beginStream(&fbdo_s1, "/Warning/TempWarSet"))
  Serial.printf("stream 1 (Nhiet do dat canh bao) begin error,%s\n\n", fbdo_s1.errorReason().c_str());
  if (!Firebase.RTDB.beginStream(&fbdo_s2, "/TimeSetInHistory/TimeSet/"))
  Serial.printf("stream 2 (Truyen du lieu luu tru len database ) begin error,%s\n\n", fbdo_s2.errorReason().c_str());
if (!Firebase.RTDB.beginStream(&fbdo_s3, "/LineSet/LineSet/"))
  Serial.printf("stream 3 (Nhan du lieu so dong hien thi ) begin error,%s\n\n", fbdo_s3.errorReason().c_str());
}

void loop() {
  if (checktime==0){
   getTime();
   checktime=1;}
 if ((millis()-prevMillisMain)>5000){
  prevMillisMain=millis(); 
  Hum= DHT22.runHum();
  Temp=DHT22.runTemp();
 }
 getLine();
  if ((millis()-deleteHisMillis)>10000){
    deleteHisMillis=millis(); 
  checkLineHis();
  }
  if ((millis()-deleteWarMillis)>10000){
    deleteWarMillis=millis(); 
  checkLineWar();
  }
  truyenChinh(Hum,Temp);
  setTimeHis();
  hisdatabase(Hum, Temp);
  getTempWar();
  warDatabase(Hum, Temp);
 // Serial.println(String(lineSet));
}