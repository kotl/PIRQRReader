#include <Arduino.h>
#include "BluetoothSerial.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include <Base64.h>
#include <elapsedMillis.h>

#define PIR_PIN 13
#define LED_BUILTIN 2 // Should short on pin 4 (bright LED)

BluetoothSerial ESP_BT; 

String RESP_INIT = "PIR_ESP_INIT\n";
String RESP_MOTION_START = "MOTION_START\n";
String RESP_MOTION_STOP = "MOTION_STOP\n";
String RESP_PIC_BASE64 = "PIC_BASE64:";
String RESP_PIC_BIN = "PIC_BIN:";
String RESP_PIC_END = "PIC_END\n";
String RESP_PIC_FAILED = "PIC_FAILED\n";
String RESP_CAMERA_INIT_FAILED = "CAMERA_INIT_FAILED\n";
String RESP_EXECUTE = "EXECUTE_";
String LINEEND = "\n";

String REQ_LED_ON = "LEDON\n";
String REQ_LED_OFF = "LEDOFF\n";
String REQ_TAKE_PIC_BIN = "TAKEPICBIN\n";
String REQ_TAKE_PIC_BASE64 = "TAKEPICBASE64\n";


void writeBT(String msg) {
  ESP_BT.write((unsigned char* )msg.c_str(), msg.length());  
}

void initCamera() {

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;    
  esp_err_t err = esp_camera_init(&config);  
  if (err != ESP_OK) {
    writeBT(RESP_CAMERA_INIT_FAILED);
    return;
  }
}

#define ENCODE_BUF_LENGTH 14000
#define CHUNK_LENGTH 10000

uint8_t encodeBuffer[ENCODE_BUF_LENGTH];

elapsedMillis timeElapsed;

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  ESP_BT.begin("PIR-ESP-CAM");
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(5000);

  initCamera();
  writeBT(RESP_INIT);
  timeElapsed = 0;
}

void takePic(String command) {
 camera_fb_t * fb = NULL;
  
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    writeBT(RESP_PIC_FAILED);
    return;
  }  
  if (command == REQ_TAKE_PIC_BASE64) {
    int chunk = 0;
    while(chunk*CHUNK_LENGTH < fb->len) {
      int start = chunk*CHUNK_LENGTH;
      int end = start + CHUNK_LENGTH;
      if(end>fb->len) {
        end = fb->len;
      }

      int encodedLength = Base64.encodedLength(end-start);
      writeBT(RESP_PIC_BASE64);
      writeBT(String(chunk) + "," + String(encodedLength));
      writeBT(LINEEND);
      Base64.encode((char *) encodeBuffer, (char*) &fb->buf[start], end-start);
      ESP_BT.write(encodeBuffer, encodedLength);
      writeBT(LINEEND);
      chunk++;
    }

    writeBT(RESP_PIC_END);

  }
  if (command == REQ_TAKE_PIC_BIN) {
    writeBT(RESP_PIC_BIN);
    writeBT(String(fb->len));
    writeBT(LINEEND);
    ESP_BT.write(fb->buf, fb->len);
    writeBT(RESP_PIC_END);
  }
}

int hasMotion = false;

void loop()
{
  delay(1);
  if (timeElapsed > 2000) {
    timeElapsed = 0;
    // Every 2 seconds we re-send motion status, just in case.
    writeBT(hasMotion ? RESP_MOTION_START : RESP_MOTION_STOP);

  }
  if(digitalRead(PIR_PIN) == HIGH) {
    if(!hasMotion) {
     hasMotion = true; 
     writeBT(RESP_MOTION_START);
    }
  } else {
    if(hasMotion) {
     hasMotion = false; 
     writeBT(RESP_MOTION_STOP);
    }
  }

  if (ESP_BT.available())
  {
     String command = "";
     while (ESP_BT.available()) {
       char c = ESP_BT.read();
       command += c;
       if (command.length() > 200) break;
     }
     if (command == REQ_LED_ON) {
        writeBT(RESP_EXECUTE);
        writeBT(command);
        digitalWrite(LED_BUILTIN, HIGH);      
     } else {
     if (command == REQ_LED_OFF) {
        writeBT(RESP_EXECUTE);
        writeBT(command);
        digitalWrite(LED_BUILTIN, LOW);
     } else {
     if (command == REQ_TAKE_PIC_BIN || command == REQ_TAKE_PIC_BASE64) {
        writeBT(RESP_EXECUTE);
        writeBT(command);
        takePic(command);
     } else {
       writeBT("ECHO:");
       writeBT(command);
     }}}
  }
}
