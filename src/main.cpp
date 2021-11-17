#include <Arduino.h>
#include <ESP32QRCodeReader.h>
#include "BluetoothSerial.h"

BluetoothSerial ESP_BT; 
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
struct QRCodeData qrCodeData;

#define PIR_PIN 13
#define LED_BUILTIN 2

String INIT = "QR_INIT\n";
String START = "QR_START\n";
String STOP = "QR_STOP\n";
String CODE = "QR_CODE:";
String LINEEND = "\n";
String ECHO = "ECHO:";

void writeBT(String msg) {
  ESP_BT.write((unsigned char* )msg.c_str(), msg.length());  
}

void setup()
{
  ESP_BT.begin("PIR-QR-Reader");
  reader.setup();
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(5000);
  writeBT(INIT);
}

bool started = false;

void loop()
{
  delay(10);
  if (digitalRead(PIR_PIN) == HIGH) {
    if(!started) {
      started = true;
      reader.begin();
      digitalWrite(LED_BUILTIN, HIGH);      
      writeBT(START);
    }
    if (reader.receiveQrCode(&qrCodeData, 100) && qrCodeData.valid)
    {
      writeBT(CODE);
      ESP_BT.write(qrCodeData.payload, qrCodeData.payloadLen);
      writeBT(LINEEND);
    }
  } else {
    if(started) {
      started = false;
      reader.end();
      digitalWrite(LED_BUILTIN, LOW);
      writeBT(STOP);
    }
  }  
  if (ESP_BT.available())
  {
     writeBT(ECHO);
     while (ESP_BT.available()) {
       ESP_BT.write(ESP_BT.read());
     }
     writeBT(LINEEND);
  }
}
