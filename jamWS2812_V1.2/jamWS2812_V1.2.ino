/*
 * if (A1Dy) {
   // alarm is by day of week, not date.
    A1Day = bcdToDec(temp_buffer & 0b00001111);
  } else {
    // alarm is by date, not day of week.
    A1Day = bcdToDec(temp_buffer & 0b00111111);
  }
 */

#include <FS.h>
#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DS3231.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include "DHT.h"
#include <ESP8266WebServer.h>

#define DHTPIN D5

#define PinLed D6
#define LEDS_PER_SEG 3
#define LEDS_PER_DOT 2
#define LEDS_PER_DIGIT  LEDS_PER_SEG * 7
#define LED   88
#define indikator D0 //D4=lampu internal,D0=lampu eksternal
#define BUZZ D4//D7//
#define button D3//
#define led_state D2//4;//D2
#define N_DIMMERS 1
#define dimmer_led  D4//16; //D0
#define DHTTYPE DHT11

WiFiClient client;
ESP8266WebServer server(80);

const char* host = "JAM-KECIL";

RTClib RTC;
DS3231 Time;
const char *ssid     = "ESP";
const char *password = "00000000";
DHT dht(DHTPIN, DHTTYPE);

int h1;
int h2;
int m1;
int m2;
int JW;
int MW;
int JR;
int MR;
bool wm_nonblocking = false;
DateTime now;
WiFiManager wifi;
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

byte dot1[]={42,43};
byte dot2[]={44,45};
byte flag=0;
unsigned long tmrsave = 0;
unsigned long tmrsaveHue = 0;
//unsigned long tmrWarning = 0;
int delayWarning(200);
int delayHue(2);
int Delay(500);
//int TIMER = 0;
int dotsOn = 0;
bool warningWIFI = false;
static int hue;
int pixelColor;
int peakWIFI = 0;
bool stateWifi,stateMode;
int temp1,temp2;
byte         RunSel    = 1; //
byte         RunFinish = 0 ;
boolean     DoSwap;
byte statusAlarm;
bool stateAlarm = false;
long dataTemp[100];
String jamSet1,jamSet2;
String menitSet1,menitSet2;
String statusAlarm1,statusAlarm2;

const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient Clock(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);

Adafruit_NeoPixel strip(LED, PinLed, NEO_GRB + NEO_KHZ800);

long numberss[] = {
  //  7654321
  0b0111111,  // [0] 0
  0b0100001,  // [1] 1
  0b1110110,  // [2] 2
  0b1110011,  // [3] 3
  0b1101001,  // [4] 4
  0b1011011,  // [5] 5
  0b1011111,  // [6] 6
  0b0110001,  // [7] 7
  0b1111111,  // [8] 8
  0b1111011,  // [9] 9
  0b0000000,  // [10] off
  0b1111000,  // [11] degrees symbol
  0b0011110,  // [12] C(elsius)
  0b1011110,  // [13] E
  0b0111101,  // [14] n(N)
  0b1001110,  // [15] t
  0b1111110,  // [16] e
  0b1000101,  // [17] n 
  0b1000100,  // [18] r
  0b1000111,  // [19] o
  0b1100111,  // [20] d
  0b0000001,  // [21] i
  0b1000110,  // [22] c
  0b1000000,  // [23] -
  0b1111101,  // [24] A
  0b1111100,  // [25] P
  0b1011011,  // [26] S
  0b0011110,  // [27] C
  0b1111000   // [28] '
};

bool alarm1Status = false;
bool alarm2Status = false;
String alarm1Time = "";
String alarm2Time = "";

// HTML content
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Alarm Settings</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            background-color: #f0f0f0;
        }

        .container {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        .alarm {
            margin-bottom: 20px;
        }

        h1, h2 {
            text-align: center;
        }

        button {
            display: block;
            margin: 10px auto;
        }

        p {
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Alarm Settings</h1>
        <div class="alarm">
            <h2>Alarm 1</h2>
            <input type="time" id="alarm1Time" onchange="setAlarmTime('alarm1')">
            <button onclick="toggleAlarm('alarm1')">Turn On/Off</button>
            <p id="alarm1Status">Status: Off</p>
        </div>
        <div class="alarm">
            <h2>Alarm 2</h2>
            <input type="time" id="alarm2Time" onchange="setAlarmTime('alarm2')">
            <button onclick="toggleAlarm('alarm2')">Turn On/Off</button>
            <p id="alarm2Status">Status: Off</p>
        </div>
    </div>
    <script>
        function toggleAlarm(alarm) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var response = JSON.parse(this.responseText);
                    document.getElementById('alarm1Status').innerText = 'Status: ' + (response.alarm1 ? 'On' : 'Off');
                    document.getElementById('alarm2Status').innerText = 'Status: ' + (response.alarm2 ? 'On' : 'Off');
                }
            };
            if (alarm === 'alarm1') {
                xhttp.open("GET", "/toggleAlarm1", true);
            } else if (alarm === 'alarm2') {
                xhttp.open("GET", "/toggleAlarm2", true);
            }
            xhttp.send();
        }

        function setAlarmTime(alarm) {
            var xhttp = new XMLHttpRequest();
            var time = document.getElementById(alarm + 'Time').value;
            xhttp.open("POST", "/set" + alarm.charAt(0).toUpperCase() + alarm.slice(1) + "Time", true);
            xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            xhttp.send("time=" + time);
        }
    </script>
</body>
</html>
)rawliteral";


void handleCustomConfig() {
  server.send(200, "text/html", HTML_PAGE);
  Serial.println("handleCustomConfig jalan");
  for(int i=0;i<2;i++){buzzer(1);delay(60);buzzer(0);delay(60);}
}

void handleSetAlarm() {
  Serial.println("set alarm jalan");
  buzzer(1);
  
  String inputValue = server.arg("value");
  jamSet1   = inputValue.substring(0,2);
  menitSet1 = inputValue.substring(2,4);
  jamSet2   = inputValue.substring(4,6);
  menitSet2 = inputValue.substring(6,8);
   
  String inputStatus = server.arg("status");
  statusAlarm1 = inputStatus.substring(0,1);
  statusAlarm2 = inputStatus.substring(1,2);
  
  EEPROM.write(1, statusAlarm1.toInt());
  EEPROM.write(2, statusAlarm2.toInt());
  EEPROM.write(3, jamSet1.toInt());
  EEPROM.write(4, menitSet1.toInt());
  EEPROM.write(5, jamSet2.toInt());
  EEPROM.write(6, menitSet2.toInt());
  EEPROM.commit();
  
  Serial.println(String()+ "jamSet1:menitSet1=" + jamSet1 + ":" + menitSet1);
  Serial.println(String()+ "jamSet2:menitSet2=" + jamSet2 + ":" + menitSet2);
  Serial.println(String()+ "status 1 :" + statusAlarm1);
  Serial.println(String()+ "status 2 :" + statusAlarm2);

  server.send(200, "text/plain", "OK");
  delay(60);
  buzzer(0);
}
////////////////
void toggleAlarm1() {
    alarm1Status = !alarm1Status;
    Serial.println(String() + "alarm1Status: " + String(alarm1Status));
    sendStatus();
}

void toggleAlarm2() {
    alarm2Status = !alarm2Status;
    Serial.println(String() + "alarm2Status: " + String(alarm2Status));
    sendStatus();
}

void setAlarm1Time() {
    if (server.hasArg("time")) {
        alarm1Time = server.arg("time");
        Serial.println("Alarm 1 time set to: " + alarm1Time);
    }
    sendStatus();
}

void setAlarm2Time() {
    if (server.hasArg("time")) {
        alarm2Time = server.arg("time");
        Serial.println("Alarm 2 time set to: " + alarm2Time);
    }
    sendStatus();
}

void sendStatus() {
    String json = "{ \"alarm1\": " + String(alarm1Status) + ", \"alarm2\": " + String(alarm2Status) + " }";
    server.send(200, "application/json", json);
}

////////////////////

 void setup() 
 {
   Serial.begin(115200);
   digitalWrite(indikator, LOW);
   pinMode(indikator, OUTPUT);
   digitalWrite(BUZZ,LOW);
   pinMode(BUZZ,OUTPUT);
   pinMode(button,INPUT);
   pinMode(led_state, OUTPUT);
   pinMode(dimmer_led, OUTPUT);
   EEPROM.begin(512);
   Wire.begin();
   strip.begin();
   dht.begin();
   //wifi.resetSettings(); 
   strip.setBrightness(50);
   stateWifi = EEPROM.read(0);
   stateMode = EEPROM.read(0);
   Serial.println(String()+"stateWifisetup=" + stateWifi + "stateModesetup=" + stateMode);
   
   
   if(stateWifi==1)
   { 
     if(wm_nonblocking) wifi.setConfigPortalBlocking(false);
  
  WiFiManagerParameter custom_html("alarm", HTML_PAGE, "", 5000, "type=\"hidden\"");
  wifi.addParameter(&custom_html);
  
  // Set up the custom web server
  server.on("/alarm", handleCustomConfig);
  //server.on("/set", handleSetAlarm);
  server.on("/toggleAlarm1", toggleAlarm1);
  server.on("/toggleAlarm2", toggleAlarm2);
  server.on("/setAlarm1Time", setAlarm1Time);
  server.on("/setAlarm2Time", setAlarm2Time);

  
  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wifi.setMenu(menu);

  // set dark theme
  wifi.setClass("invert");
   
   showAP();
   //wifi.setConfigPortalTimeout(60);
   bool connectWIFI = wifi.autoConnect("JAM DIGITAL KECIL", "00000000");
   //keluarkan tulisan RTC
   if(!connectWIFI) 
   {
     stateWifi=0;
     Serial.println("NOT CONNECTED TO AP");
     Serial.println("Pindah ke mode RTC");
     //showRTC();
     delay(1000);
     EEPROM.write(0,stateWifi);
     EEPROM.commit();
     for(int i =0; i < 5;i++)
     {
       digitalWrite(BUZZ,HIGH);
       delay(50);
       digitalWrite(BUZZ,LOW);
       delay(50);
     }
     ESP.restart();
    }
   else
    {
      Serial.println("CONNECTED");
      showNTP();
      
      
     
      for(int i =0; i < 2;i++)
      {
        digitalWrite(BUZZ,HIGH);
        delay(50);
        digitalWrite(BUZZ,LOW);
        delay(50);
      }
       Clock.begin();//NTP
       Clock.update();
       Time.setHour(Clock.getHours());
       delay(500);
       Time.setMinute(Clock.getMinutes());
       delay(500);
       Time.setSecond(Clock.getSeconds());
       showConnect();
       delay(1000);
       strip.clear();
       Serial.println(String()+"NTP in the setup:"+ Clock.getHours()+":"+ Clock.getMinutes()+":"+Clock.getSeconds());
       //digitalWrite(led_state, HIGH);
       for(int i=0;i<2;i++){ strip.setPixelColor(dot1[i],strip.Color(255,0,0)); strip.setPixelColor(dot2[i],strip.Color(0,0,0)); strip.show();}
      
       Serial.println("Booting");
       //flag=0;
     
       /* switch off led */
       //digitalWrite(led_state, LOW);
       for(int i=0;i<2;i++){ strip.setPixelColor(dot1[i],strip.Color(0,0,0)); strip.setPixelColor(dot2[i],strip.Color(0,0,0)); strip.show();}
       
       /* configure dimmers, and OTA server events */
       analogWriteRange(1000);
       //analogWrite(led_pin, 2);
     
       
       //analogWrite(dimmer_led, 50);
       for(int i=0;i<2;i++){ strip.setPixelColor(dot1[i],strip.Color(0,0,0)); strip.setPixelColor(dot2[i],strip.Color(50,0,0)); strip.show();}
       
     
       ArduinoOTA.setHostname(host);
       ArduinoOTA.onStart([]() 
        {
         buzzer(1);
         delay(50);
         buzzer(0);
         strip.clear();
        });
     
       ArduinoOTA.onEnd([]() 
       { // do a fancy thing with our board led at end
        strip.clear();
        showEnd();
       });

       ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
       //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
       int ledsToLight = (progress * LED) / total;
       //strip.clear();
         strip.setPixelColor(ledsToLight, strip.Color(0, 0, 255)); // Biru menandakan progres
         
          strip.show();
        Serial.println(String()+"counter:"+ledsToLight);
        });
     
       ArduinoOTA.onError([](ota_error_t error) 
       {
         strip.clear();
         Serial.println(String()+"error");
         showError();
         (void)error;
         ESP.restart();
       });
     
       /* setup the OTA server */
       ArduinoOTA.begin();
       Serial.println("Ready");
       server.begin();
       delay(1000);
       //Serial.println(String()+"NTP in the setup:"+ Clock.getHours()+":"+ Clock.getMinutes()+":"+Clock.getSeconds());
       flag=1;
     }
   }  
   else
   {  
      showRTC();
      delay(1000);
   } 
    strip.clear();
    Serial.println("RUN");
    
     int inputStatusAlarm1 = EEPROM.read(1);
     int inputStatusAlarm2 = EEPROM.read(2);
     int inputJamSet1 = EEPROM.read(3);
     int inputMenitSet1 = EEPROM.read(4);
     int inputJamSet2 = EEPROM.read(5);
     int inputMenitSet2 = EEPROM.read(6);

     statusAlarm1 = String(inputStatusAlarm1);
     statusAlarm2 = String(inputStatusAlarm2);
     jamSet1      = String(inputJamSet1);
     menitSet1    = String(inputMenitSet1);
     jamSet2      = String(inputJamSet2);
     menitSet2    = String(inputMenitSet2);
  
    Serial.println(String()+ "jamSet1:menitSet1=" + inputJamSet1 + ":" + inputMenitSet1);
    Serial.println(String()+ "jamSet2:menitSet2=" + inputJamSet2 + ":" + inputMenitSet2);
    Serial.println(String()+ "status 1 :" + inputStatusAlarm1);
    Serial.println(String()+ "status 2 :" + inputStatusAlarm2);
}

void loop() {
  checkButton();
  //stateWIFI();
  timerHue();
  //autoBright();
  //autoConnectt();
  //timerRestart();
  //printDebug();
  alarmRun(stateAlarm);
  
  //Serial.println(String()+"stateAlarm :"+ stateAlarm);
  
 if(stateWifi == 1)
  {
   ArduinoOTA.handle();
   server.handleClient();
   if(wm_nonblocking) wifi.process();
 
   warningWIFI = 0;
    if(flag){
      showCode();
      strip.show();
      buzzer(1);
      Clock.update();
  Time.setHour(Clock.getHours());
  delay(500);
  Time.setMinute(Clock.getMinutes());
  delay(500);
  Time.setSecond(Clock.getSeconds());
  Serial.println(String()+"flag aktive");
  Serial.println(String()+"NTP in the setup:"+ Clock.getHours()+":"+ Clock.getMinutes()+":"+Clock.getSeconds());
  flag=0;
  buzzer(0);
//   Time.setYear(24);
//   Time.setMonth(07);
//   Time.setDate(26);
//   Time.setDoW(5);
   }
    
    monitorNtp(1);
    monitorTemp(2);
   
   
   }
  else if(stateWifi == 0)
   {
    monitorRtc(1);
    monitorTemp(2);

   }
    if(checkAlarm(jamSet1.toInt(),menitSet1.toInt(),statusAlarm1.toInt())){
    stateAlarm=true;
    Serial.println(String()+"alarm jalan 1");
   }
    else if(checkAlarm(jamSet2.toInt(),menitSet2.toInt(),statusAlarm2.toInt())){
    stateAlarm=true;
    Serial.println(String()+"alarm jalan 2");
   }
    //Serial.println(String()+"RunSel:"+RunSel);
    if(RunFinish==1) {RunSel = 2; RunFinish =0;}                      //after anim 1 set anim 2
    if(RunFinish==2) {RunSel = 1; RunFinish =0;}
    
    digitalWrite(indikator, warningWIFI);
  
}

bool checkAlarm(int jam,int menit,int state){
  now = RTC.now();
  if(now.hour() == jam && now.minute() == menit && now.second() == 00 && state == 1){
    return true;
  }
  else{
    return false;
  }
}

void alarmRun(int state){
  if(!state) return;
  static byte counter;
  byte         limit = 60;
  static unsigned long save;
  unsigned long        tmr = millis();
 // buzzer(1);
  if((tmr - save) > 1000 and counter <= limit){
    save = tmr;
    counter++;
    if(counter % 2){
      for(int i = 0; i <= 2; i++){ buzzer(1); delay(60); buzzer(0); delay(60); }
    }
    else{
      buzzer(0);
    }
    Serial.println(String()+"counterAlarm:"+ counter);
  }
  if((tmr - save) > 2000 and (counter > limit)){
    counter = 0;
    save = 0;
    buzzer(0);
    stateAlarm = false;
  }
}

void monitorNtp(int drawAdd){
  if(!dwDo(drawAdd)) return;
  static byte counter;
  byte         limit = 10;
  static unsigned long save;
  unsigned long        tmr = millis();
  getClockNTP();
  showClock(Wheel((hue + pixelColor) & 255));
  showDots(strip.Color(255, 0, 0));
  //Serial.println(String()+"test ntp");
  strip.show();
  
  if((tmr - save) > 1000 and counter <= limit){
    save = tmr;
    counter++;
    //Serial.println(String()+"counterNTP:"+ counter);
  }
  if((tmr - save) > 2000 and (counter > limit)){
    counter = 0;
    save = 0;
    dwDone(drawAdd);
  }
  
}

void monitorRtc(int drawAdd){
  if(!dwDo(drawAdd)) return;
  static byte counter;
  byte         limit = 10;
  static unsigned long save;
  unsigned long        tmr = millis();

  getClockRTC();
  warningWIFI = 1;
  showClock(Wheel((hue + pixelColor) & 255));
  showDots(strip.Color(255, 0, 0));
  strip.show();
  
  if((tmr - save) > 1000 and counter <= limit){
    save = tmr;
    counter++;
  }
  if(counter > limit){
    counter = 0;
    dwDone(drawAdd);
  }
}

void monitorTemp(int drawAdd){
  if(!dwDo(drawAdd)) return;
  static byte counter;
  byte         limit = 5;
  static unsigned long save;
  unsigned long        tmr = millis();

  getTemp();
  showTemp();
    for (int i = 42; i <= 45; i++) {
        strip.setPixelColor(i , strip.Color(0, 0, 0));
    }
    //Serial.println(String()+"test temperatur");
   
   
  if((tmr - save) > 1000 and counter <= limit){
    save = tmr;
    counter++;
    //Serial.println(String()+"counterTEMP:"+ counter);
  }
  if((tmr - save) > 2000 and (counter > limit)){
    counter = 0;
    save = 0;
    dwDone(drawAdd);
  }
}

//void printDebug()
//{
////  if(stateWifi==0)
////  {  
////    now=RTC.now(); 
////    Serial.println(String()+"RTC:" + JW+":"+ MW+":"+now.second());
////  }
////  else
////  {
////    Clock.update(); 
////    Serial.println(String()+"NTP:"+ JR+":"+ MR+":"+Clock.getSeconds());
////  }
//  Serial.println(String() + "stateWifi=" + stateWifi + "stateMode=" + stateMode);
//}



void DisplayNumber(byte number, byte segment, uint32_t color) {
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = LEDS_PER_DIGIT;
      break;
    case 2:
      startindex = LEDS_PER_DIGIT * 2 + LEDS_PER_DOT * 2;
      break;
    case 3:
      startindex = LEDS_PER_DIGIT * 3 + LEDS_PER_DOT * 2;
      break;
  }

  for (byte i = 0; i < 7; i++) {           // 7 segments
    for (byte j = 0; j < LEDS_PER_SEG; j++) {         // LEDs per segment
      strip.setPixelColor(i * LEDS_PER_SEG + j + startindex , (numberss[number] & 1 << i) == 1 << i ? color : strip.Color(0, 0, 0));
      //strip.setPixelColor(i * LEDS_PER_SEG + j + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? color : color(0,0,0);
      strip.show();
    }
  }

  //yield();
}

void getClockRTC() 
{
  now = RTC.now();
//  Time.update();
  h1 = now.hour() / 10;
  h2 = now.hour() % 10;
  m1 = now.minute() / 10;
  m2 = now.minute() % 10;
  JR = now.hour();
  MR = now.minute();
 // Serial.println(String()+"RTC:"+ JR+":"+ MR+":"+now.second());
 
}

void getClockNTP()
{
  Clock.update();
  h1 = Clock.getHours() / 10;
  h2 = Clock.getHours() % 10;
  m1 = Clock.getMinutes() / 10;
  m2 = Clock.getMinutes() % 10;
  JW = Clock.getHours();
  MW = Clock.getMinutes();
  //Serial.println(String()+"NTP:"+ JW+":"+ MW+":"+Clock.getSeconds());
}

void getTemp(){
   float data = dht.readTemperature();
   temp1 = int(data) / 10;
   temp2 = int(data) % 10;
   
}

void showClock(uint32_t color) {
  DisplayNumber(h1, 3, color);
  DisplayNumber(h2, 2, color);
  DisplayNumber(m1, 1, color);
  DisplayNumber(m2, 0, color);
}

void showConnect() {
  DisplayNumber( 12, 3, strip.Color(255, 0, 0));
  DisplayNumber( 17, 2, strip.Color(0, 255, 0));
  DisplayNumber( 16, 1, strip.Color(0, 255, 0));
  DisplayNumber( 15, 0, strip.Color(0, 255, 0));
}

void showDisconnect() {
  DisplayNumber( 20, 3, strip.Color(255, 0, 0));
  strip.setPixelColor(63 , strip.Color(255, 0, 0));
  DisplayNumber( 21, 2, strip.Color(255, 0, 0));
  DisplayNumber( 5, 1, strip.Color(255, 0, 0));
  DisplayNumber( 22, 0, strip.Color(255, 0, 0));
}

void showError() {
  DisplayNumber( 13, 3, strip.Color(255, 0, 0));
  DisplayNumber( 18, 2, strip.Color(255, 0, 0));
  DisplayNumber( 19, 1, strip.Color(255, 0, 0));
  DisplayNumber( 18, 0, strip.Color(255, 0, 0));
}

void showEnd() {
  DisplayNumber( 23 , 3, strip.Color(255, 0, 0));
  DisplayNumber( 13, 2, strip.Color(0, 255, 0));
  DisplayNumber( 17, 1, strip.Color(0, 255, 0));
  DisplayNumber( 20, 0, strip.Color(0, 255, 0));
}

void showRTC() {
  DisplayNumber( 23, 3, strip.Color(255, 0, 0));
  DisplayNumber( 18, 2, strip.Color(0, 255, 0));
  DisplayNumber( 15, 1, strip.Color(0, 255, 0));
  DisplayNumber( 22, 0, strip.Color(0, 255, 0));
}

  void showNTP() {
  DisplayNumber( 23, 3, strip.Color(255, 0, 0));
  DisplayNumber( 17, 2, strip.Color(0, 255, 0));
  DisplayNumber( 15, 1, strip.Color(0, 255, 0));
  DisplayNumber( 25, 0, strip.Color(0, 255, 0));
  }

  void showAP() {
  DisplayNumber( 23, 3, strip.Color(255, 0, 0));
  DisplayNumber( 23, 2, strip.Color(255, 0, 0));
  DisplayNumber( 24, 1, strip.Color(0, 255, 0));
  DisplayNumber( 25, 0, strip.Color(0, 255, 0));
  }

//  void showRST(){
//  DisplayNumber( 23, 3, strip.Color(255, 0, 0));
//  DisplayNumber( 18, 2, strip.Color(0, 255, 0));
//  DisplayNumber( 26, 1, strip.Color(0, 255, 0));
//  DisplayNumber( 15, 0, strip.Color(0, 255, 0));
//  }

  void showCode() {
  DisplayNumber( 23, 3, strip.Color(255, 0, 0));
  DisplayNumber( 23, 2, strip.Color(255, 0, 0));
  DisplayNumber( 2, 1, strip.Color(0, 255, 0));
  DisplayNumber( 4, 0, strip.Color(0, 255, 0));
  }

  void showTemp(){
    DisplayNumber( temp1, 3, strip.Color(255, 0, 0));
    DisplayNumber( temp2, 2, strip.Color(255, 0, 0));
    DisplayNumber( 28, 1, strip.Color(255, 0, 0));
    DisplayNumber( 27, 0, strip.Color(255, 0, 0));
  }


void stateWIFI() {
  if(!stateWifi) return;
  static int TIMER;
  static unsigned long tmrWarning;
  //unsigned long tmr = millis();

  if (WiFi.status() != WL_CONNECTED) {
    
    if (millis() - tmrWarning > delayWarning) {
      tmrWarning = millis();
      TIMER++;
      if(TIMER <= 10)
      {
        if(TIMER % 2){buzzer(1);}//digitalWrite(BUZZ,HIGH);}
        else{buzzer(0);}//digitalWrite(BUZZ,LOW);}
      }
      if(TIMER >= 30){//rubah ini jika dirasa waktu tunda pergantian mode wifi saat disconnect kurang/lebih
        stateWifi = 0;
        EEPROM.write(0,stateWifi);
        EEPROM.commit();
        buzzer(1);
        showDisconnect();
        delay(1500);
        ESP.restart();
      }
      Serial.println(String() + "Timer:" + TIMER);
    }
  }
    
}
  

void showDots(uint32_t color) {

   now = RTC.now();
   dotsOn = now.second();
    if (dotsOn % 2) {
      for (int i = 42; i <= 45; i++) {
        strip.setPixelColor(i , color);
      }

    } else {
      for (int i = 42; i <= 45; i++) {
        strip.setPixelColor(i , strip.Color(0, 0, 0));
      }
    }
  strip.show();
}


void timerHue() 
{
  unsigned long tmr = millis();
  if (tmr - tmrsaveHue > delayHue) {
    tmrsaveHue = tmr;
    if (pixelColor < 256) {
      pixelColor++;
      if (pixelColor == 255) {
        pixelColor = 0;
      }
    }
  }

  for (int hue = 0; hue < strip.numPixels(); hue++) 
  {
    hue++;
   
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void checkButton()
{
  if(digitalRead(button) == LOW)
  {
    stateWifi = !stateWifi;
    EEPROM.write(0,stateWifi);
    EEPROM.commit();
    Serial.println(String() + "button ditekan,stateWifi:" + stateWifi + " stateMode:" + stateMode);
  }
     
   if(stateMode != stateWifi)
  {  
    Serial.println(String() + "mode berubah");
    for (int i = 0; i < 150; i++) 
    {
      for(int a=0;a<4;a++){ strip.setPixelColor(dot2[a],strip.Color((i * 100) % 1001,0,0)); strip.setPixelColor(dot1[a],strip.Color((i * 100) % 1001,0,0)); strip.show();}
      if(i % 2){buzzer(1);}
      else{buzzer(0);}
      delay(50);
    }
    delay(1000);
    
    strip.clear();
   // wifi.resetSettings(); 
    ESP.restart();
  }
 
   
}

/*
void autoConnectt()
{
  if(stateWifi==0)
  {
  int dataJam[]={0,14};
  now=RTC.now();
  int jam   = now.hour(); //perlu dirubah menjadi jam,menit,dan detik
  int menit = now.minute();
  int detik = now.second();
  if(jam == dataJam[0] || jam == dataJam[1]){
    Serial.println("masuk jam autoConnect");
  if(menit == 0 && detik == 0){
    Serial.println("system autoConnect running");
 // if(detik == 0){
  //WiFiManager wifi;
  wifi.setConfigPortalTimeout(5);
  if(!wifi.autoConnect("JAM DIGITAL", "00000000"))
  {
    Serial.println("auto connect failed");
    for(int i =0; i < 5;i++)
    {
      digitalWrite(BUZZ,HIGH);
      delay(50);
      digitalWrite(BUZZ,LOW);
      delay(50);
    }
    delay(1000);
//    ESP.restart();
  }
  else
  {
    stateWifi=1;
    EEPROM.write(0,stateWifi);
    EEPROM.commit();
    for(int i =0; i < 2;i++)
    {
      digitalWrite(BUZZ,HIGH);
      delay(50);
      digitalWrite(BUZZ,LOW);
      delay(50);
    }
    delay(500);
   // ESP.restart();
  }
  }
  }//
  }
}
*/

void buzzer(int state)
{
 if(state){digitalWrite(BUZZ,HIGH);}
 else{digitalWrite(BUZZ,LOW); }
}

boolean dwDo(int DrawAdd)
  { if (RunSel== DrawAdd) {return true;}
    else return false;}
  
void dwDone(int DrawAdd)
  { RunFinish = DrawAdd;
    RunSel = 0;}

    
