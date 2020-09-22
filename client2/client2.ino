
//=======================================
#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include <SPI.h>
String humid, temp, heat, soi1, soi2, soi3, soi4, soi5, soi6;
String dataIn;
int b = 1;
int Time = 0;

TFT_eSPI tft = TFT_eSPI();

unsigned long last_time = 0; 
unsigned long currentTime = millis();

void splitData(String input){
  
  for (int i = 0; i < input.length(); i++) {
  
  if (input.substring(i, i+1) == "," && b == 1) {
    humid = input.substring(0, i);
    temp = input.substring(i+1, i+6);
    b = 2;
  }
  else if(input.substring(i, i+1) == "," && b == 2){
    heat = input.substring(i+1, i+6);
    b = 3;
  }
  else if(input.substring(i, i+1) == "," && b == 3){
    soi1 = input.substring(i+1, i+6);
    b = 4;
  }else if(input.substring(i, i+1) == "," && b == 4){
    soi2 = input.substring(i+1, i+6);
    b = 5;
  }else if(input.substring(i, i+1) == "," && b == 5){
   soi3 = input.substring(i+1, i+6);
   b = 6;   
  }else if(input.substring(i, i+1) == "," && b == 6){
   soi4 = input.substring(i+1,i+6);
      b = 7; 
  }else if(input.substring(i, i+1) == "," && b == 7){
   soi5 = input.substring(i+1,i+6);
      b = 8; 
  }else if(input.substring(i, i+1) == "," && b == 8){
   soi6 = input.substring(i+1,i+6);
      b = 1; 
  }
  
  }
  
}

void showScrn1() {


 // tft.setTextColor(TFT_GREEN, TFT_BLACK);

// tft.setFreeFont(&FreeMonoBold9pt7b);//find fonts from TFT_eSPI.h library...
   
//  tft.drawLine(110, 37,110, 132, TFT_BLUE); // center 

  //DHT22
  tft.setCursor(5, 10);
  tft.print("Temp:");
  tft.setCursor(75, 10);
  tft.print(temp);
  tft.print(F("C"));
    
  tft.setCursor(5, 30);
  tft.print("Humid:");
  tft.setCursor(75, 30);
  tft.print(humid);
  tft.print(F("%"));

  tft.setCursor(5, 50);
  tft.println("Heat:");
  tft.setCursor(75, 50);
  tft.print(heat);
  tft.print(F("C"));
  
   //ds18b20
  tft.setCursor(5, 70);
  tft.print("T1:");
  tft.setCursor(40, 70);
  tft.print(soi1);
  tft.print(F("C"));

  tft.setCursor(125, 70);
  tft.print("T2:");
  tft.setCursor(160, 70);
  tft.print(soi2);
  tft.print(F("C"));

  tft.setCursor(5, 90);
  tft.println("T3:");
  tft.setCursor(40, 90);
  tft.print(soi3);
  tft.print(F("C"));

  tft.setCursor(125, 90);
  tft.println("T4:");
  tft.setCursor(160, 90);
  tft.print(soi4);
  tft.print(F("C"));

  tft.setCursor(5, 110);
  tft.println("T5:");
  tft.setCursor(40, 110);
  tft.print(soi5);
  tft.print(F("C"));

  tft.setCursor(125, 110);
  tft.println("T6:");
  tft.setCursor(160, 110);
  tft.print(soi6);
  tft.print(F("C"));
   
}

void showTime() {
  if( millis() - last_time > 1000) {
        last_time = millis();
        tft.setCursor(190, 30);
        tft.println("   ");//clear
        tft.setCursor(180, 10);
        tft.print("Time");
        tft.setCursor(190, 30);
        Time++;
        tft.print(Time);
        tft.print(F("s"));
        Serial.println(Time);
        showScrn1();  
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  dataIn = "";
  for (int i=0;i<len;i++) {
   dataIn += (char)incomingData[i]; 
  }
  splitData(dataIn); 
  Time = 0;
}

void setup() {
  Serial.begin(115200);
 
  tft.begin();
  tft.setRotation(3); //Landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
 //tft.setFreeFont(&FreeMonoBold9pt7b);
  
  WiFi.mode(WIFI_STA);
  
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  static esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, new uint8_t[6]{0x24, 0x62, 0xAB, 0xF1, 0xB5, 0x2C}, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;  
     
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  
 showTime();
    
  // put your main code here, to run repeatedly:

}
