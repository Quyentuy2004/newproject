#define DHTPIN 4  // GPIO nối với DATA của DHT22
byte RH_high, RH_low, temp_high, temp_low, checksum;
float hum;
float temp;
unsigned long startTime;



byte Readbit(){
  byte dataByte=0;
  for(byte i=0; i<8; i++)
  {
    while(digitalRead(DHTPIN)==LOW);
      
      /*detect data bit (high pulse)*/
    startTime = micros();
    while (digitalRead(DHTPIN) == HIGH) ;
    if (micros() - startTime > 40) {
      dataByte = (dataByte<<1)|(0x01);
    }
    
  
   
    else dataByte = (dataByte<<1);   /*store 1 or 0 in dataByte*/
    //--------------------------------------------------------------------
   // while(digitalRead(DHTPIN)==LOW);        /*wait for DHT22 low pulse*/
  }
  return dataByte;
}



bool readDHT22() {
RH_high=0;
  RH_low=0;
  temp_high=0;
  temp_low=0;
  checksum=0;
  // Gửi start signal
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delayMicroseconds(18000); 
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(35);
  pinMode(DHTPIN, INPUT);

  // Chờ phản hồi từ DHT22
   startTime = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - startTime > 100) 
    return false;
  }

  // DHT kéo LOW khoảng 80us
  startTime = micros();
  while (digitalRead(DHTPIN) == LOW) {
    if (micros() - startTime > 100) 
    return false;
  }

  // Sau đó DHT kéo HIGH khoảng 80us
  startTime = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - startTime > 100) return false;
  }

 
  RH_high=Readbit();
  RH_low=Readbit();
  temp_high=Readbit();
  temp_low=Readbit();
  checksum=Readbit();
 // Serial.println( RH_high);
 // Serial.println( RH_low);
  //Serial.println(temp_high);
  //Serial.println(temp_low);
  // Tính checksum
  uint8_t check =  RH_high + RH_low + temp_high + temp_low;
  //if (uint8_t(checksum) == check){
   // Serial.print("Ok");
 // }
  return uint8_t(checksum) == check;
}

class DHT{
public:
void begin() {
      Serial.begin(115200);
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, HIGH);
  delay(2000); // chờ DHT22 ổn định
  
  
}
float runTemp() {
  
  if (readDHT22()) {
   
   temp = ((temp_high << 8) | temp_low)/10.0;/*get 16-bit value of temp*/

    Serial.print("Nhiet do: "); Serial.print(temp); Serial.println(" *C");
 
  } 
  else {
    Serial.println("Loi doc DHT22!");
  }

  delay(2000);

  return temp;
}
 float runHum() {
  
  if (readDHT22()) {
   hum  = ((RH_high << 8) | RH_low)/10.0;    /*get 16-bit value of humidity*/
  
  
    Serial.print("Do am: "); Serial.print(hum); Serial.println(" %");
  } 
  else {
    Serial.println("Loi doc DHT22!");
  }
  delay(2000);
return hum;
}   

} DHT22;