//header defination
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include "Adafruit_GFX.h"
#include "OakOLED.h"
//#include <ESPmDNS.h>
//#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Tone32.h>
#include <Adafruit_BMP280.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
//#include <DabbleESP32.h>
//#include <WiFiClientSecure.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>

ThreeWire myWire(22,24,27); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
const char* mqttServer = "	xxx.xxx.xxx.xxx";
const int mqttPort = xxxx;
const char* mqttUser = "";
const char* mqttPassword = "";
int prevState=0, prevState2=0;
String datarecvd, bpm, spo, msgparam;
char timeChar[8],buffloc[14];
char msgtemp[25];
#define BUZZER_PIN 5
#define BUZZER_CHANNEL 0
#define SWITCH_PIN 18
#define buttonPin  19
String MY_LAT, MY_LON;
int alaram=0;
String ast, ast1, ahr,amn, ady0,ady1,ady2,ady3,ady4,ady5,ady6, med1,med2;
String ast2, ahr2,amn2, ady20,ady21,ady22,ady23,ady24,ady25,ady26, med21,med22;
WiFiClient espClient;
PubSubClient client(espClient);
//UniversalTelegramBot bot(BOT_TOKEN, secured_client);
int local =0;
Adafruit_BMP280 bmp280;
OakOLED oled;
const char esp32[]  = "ESP32";
float temp, altitude, pressure;
uint32_t tsLastReport = 0;
//Defining ssid and password for connectivity
const char* ssid = "xxxxxxxx";  
const char* password = "xxxxxxxx"; 

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

boolean fall = false, atune=false,atune2=false; //stores if a fall has occurred
boolean trigger1=false; //stores if first trigger (lower threshold) has occurred
boolean trigger2=false; //stores if second trigger (upper threshold) has occurred
boolean trigger3=false; //stores if third trigger (orientation change) has occurred
byte trigger1count=0; //stores the counts past since trigger 1 was set true
byte trigger2count=0; //stores the counts past since trigger 2 was set true
byte trigger3count=0; //stores the counts past since trigger 3 was set true
int angleChange=0;
//BMP image
const unsigned char bitmap [] PROGMEM=
{
0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x0f, 0xe0, 0x7f, 0x00, 0x3f, 0xf9, 0xff, 0xc0,
0x7f, 0xf9, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xf0,
0xff, 0xf7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0x7f, 0xdb, 0xff, 0xe0,
0x7f, 0x9b, 0xff, 0xe0, 0x00, 0x3b, 0xc0, 0x00, 0x3f, 0xf9, 0x9f, 0xc0, 0x3f, 0xfd, 0xbf, 0xc0,
0x1f, 0xfd, 0xbf, 0x80, 0x0f, 0xfd, 0x7f, 0x00, 0x07, 0xfe, 0x7e, 0x00, 0x03, 0xfe, 0xfc, 0x00,
0x01, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x3f, 0xc0, 0x00,
0x00, 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t Alarm88[8] PROGMEM = //alram
{ 0xC3,0xBD,0x42,0x52,0x4E,0x42,0x3C,0xC3};
//WebServer server(80);
//setup begins
void setup() {
  Serial.begin(115200);
  bmp280.begin(0x76);
  ss.begin(GPSBaud);
  //Dabble.begin("HasHealthBLE"); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(SWITCH_PIN, INPUT);
  digitalWrite(SWITCH_PIN, LOW);
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  oled.begin();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(0, 0);
  oled.println("Starting device...");
  oled.display();
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }
  WiFi.setSleep(true);

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
 if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }  
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) 
    {
      Serial.println("connected");  
    } else {
      Serial.println("MQTT failed start: ");
      Serial.print(client.state());
      delay(1000);
    }
  }
  /*   while(!MDNS.begin("hashealth")) {
        Serial.println("Error setting up MDNS responder!");
            delay(500);
    } */
  client.subscribe("HasHealthEsp01");
  client.subscribe("alarm");
 // server.begin();
//  server.on("/", handle_OnConnect);
 // server.onNotFound(handle_NotFound);
 //   MDNS.addService("http", "tcp", 80);
//secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(1);
  oled.setCursor(2, 25);
  oled.println("HAS HEALTH");
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(50, 50);
  oled.println(WiFi.localIP());
  oled.display();
  delay(4000);
}

void loop() {
 // server.handleClient();
  client.loop();
  mpu_read();
  //Dabble.processInput();
  RtcDateTime now = Rtc.GetDateTime();
    
if ((ahr.toInt() == now.Hour()) && (amn.toInt()== now.Minute()) && (prevState==0) && ((ady0.toInt()==now.DayOfWeek())|| (ady1.toInt()==now.DayOfWeek())|| (ady2.toInt()==now.DayOfWeek())||
(ady3.toInt()==now.DayOfWeek())|| (ady4.toInt()==now.DayOfWeek())||(ady5.toInt()==now.DayOfWeek())||(ady6.toInt()==now.DayOfWeek())) )
{
  atune=true;
  Serial.println("If case of Rem1");
  tone(BUZZER_PIN, 1800, 100, BUZZER_CHANNEL);
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(10,5);
  oled.println("Medicine");
  oled.setCursor(10,25);
  oled.println(med1);
  oled.setCursor(10,45);
  oled.println(med2);
  oled.display();
}
else if ((prevState==1) && (amn.toInt()< now.Minute()))
{
  prevState=0;
  Serial.println("Now else case of Rem1");
  Serial.println(prevState);
}
//State for display
if ((ahr2.toInt()== now.Hour()) && (amn2.toInt()== now.Minute()) && (prevState2==0) && ((ady20.toInt()==now.DayOfWeek()))
{
        Serial.println("If case of Rem2");
        atune2=true;
        tone(BUZZER_PIN, 1800, 100, BUZZER_CHANNEL);
        oled.clearDisplay();
        oled.setTextSize(2);
        oled.setCursor(10,5);
        oled.println("Medicine");
        oled.setCursor(10,25);
        oled.println(med21);
        oled.setCursor(10,45);
        oled.println(med22);
        oled.display();
}
else if ((prevState2==1) && (amn2.toInt()< now.Minute()))
{
  prevState2=0;
  Serial.println("Now else case of Rem2");
  Serial.println(prevState2);
}
 //Display in Oled
  sprintf(timeChar, "%02d:%02d:%02d",now.Hour(), now.Minute(), now.Second());
   client.publish("ESPRTC", timeChar);
  if ((digitalRead(SWITCH_PIN) == HIGH) && (atune==false)&& (atune2==false))
  {
    oled.clearDisplay();
    if (ast1=="S1" || ast2=="S2")
    {oled.drawBitmap(120,0,Alarm88, 8, 8, 1);}
    oled.setTextSize(2);
    oled.setTextColor(1);
    oled.setCursor(15, 5);
    oled.println(timeChar);
    oled.setCursor(0,26);
    switch (now.DayOfWeek()) {
    case 0: oled.println("Sun"); break;
    case 1: oled.println("Mon"); break;
    case 2: oled.println("Tue");break;
    case 3: oled.println("Wed");break;
    case 4: oled.println("Thu");break;
    case 5: oled.println("Fri");break;
    case 6: oled.println("Sat");break;}
    oled.setCursor(35,26);
    oled.println(",");
    oled.setCursor(50,26);
    oled.println(now.Day());
    oled.setCursor(85,26);
    switch (now.Month()) {
    case 1: oled.println("Jan"); break;
    case 2: oled.println("Feb"); break;
    case 3: oled.println("Mar");break;
    case 4: oled.println("Apr");break;
    case 5: oled.println("May");break;
    case 6: oled.println("Jun");break;
    case 7: oled.println("Jul");break;
    case 8: oled.println("Aug");break;
    case 9: oled.println("Sep");break;
    case 10: oled.println("Oct");break;
    case 11: oled.println("Nov");break;
    case 12: oled.println("Dec");break;
    }
    oled.setCursor(35, 48);
    oled.println(now.Year());
    oled.display();
  }
 else if ((digitalRead(SWITCH_PIN) == LOW)  && (atune==false)&& (atune2==false))
   {
        oled.clearDisplay();
        if (ast1=="S1" || ast2=="S2")
        {oled.drawBitmap(120,0,Alarm88, 8, 8, 1);}
        oled.drawBitmap(5, 0, bitmap, 28, 28, 1);
        oled.setTextSize(2);
        oled.setTextColor(1);
        oled.setCursor(62,5);
        oled.println(bpm); 
        oled.setCursor(0, 45);
        oled.println("SpO2");
        oled.setCursor(62,45);
        oled.println(spo);
        oled.setCursor(80,45);
        oled.println(" %");
        oled.display();
     }
      
 // }
 //Calibration of MPU6050
 ax = (AcX-2050)/14444.00;
 ay = (AcY-77)/16444.00;
 az = (AcZ-1947)/15344.00;
 gx = (GyX+270)/122.07;
 gy = (GyY-351)/122.07;
 gz = (GyZ+136)/122.07;
 // calculating Amplitute vactor for 3 axis
 float Raw_Amp = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
 int Amp = Raw_Amp * 10;  // Mulitiplied by 10 bcz values are between 0 to 1
 int buttonstate=digitalRead(buttonPin);

 if (buttonstate == 0) {
   fall=false;
        if (prevState==0 && (atune==true)){
             Serial.println("PrevState");
             Serial.println(prevState);
             prevState=1;
             atune=false;
             Serial.println("PrevState");
             Serial.println(prevState);
        }
         else if (prevState2==0 && (atune2==true)){
             Serial.println("PrevState2");
             Serial.println(prevState2);
             prevState2=1;
             atune2=false;
             Serial.println("PrevState2");
             Serial.println(prevState2);
        }
  noTone(BUZZER_PIN, BUZZER_CHANNEL);
  }


 if (Amp<=4 && trigger2==false){ //if AM breaks lower threshold (0.4g)
   trigger1=true;
   }
 if (trigger1==true){
   trigger1count++;
   if (Amp>=9){ //if AM breaks upper threshold (3g)
     trigger2=true;
     trigger1=false; trigger1count=0;
     }
 }
 if (trigger2==true){
   trigger2count++;
   angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
   if (angleChange>=10 && angleChange<=400){ //if orientation changes by between 80-100 degrees
     trigger3=true; trigger2=false; trigger2count=0;
       }
   }
 if (trigger3==true){
    trigger3count++;
    if (trigger3count>=6){ 
       angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
       if ((angleChange>=0) && (angleChange<=100)){ //if orientation changes remains between 0-10 degrees
           fall=true; trigger3=false; trigger3count=0, local=1;
              }
       else{ //user regained normal orientation
          trigger3=false; trigger3count=0;
       }
     }
  }
 if (fall==true){ //in event of a fall detection
 Serial.println("FALL DETECTED");
 tone(BUZZER_PIN, 2000, 500, BUZZER_CHANNEL);
 } 
 if (trigger2count>=6){ //allow 0.5s for orientation change
   trigger2=false; trigger2count=0;
   }
 if (trigger1count>=6){ //allow 0.5s for AM to break upper threshold
   trigger1=false; trigger1count=0;
    }
 temp = bmp280.readTemperature();
 altitude = bmp280.readAltitude(1011.18);
 pressure = (bmp280.readPressure()/100);
 
  msgparam="";
  if (fall == true)
   {
     msgparam+= "FALL,";
   }
   else
   {
      msgparam+= "OK,";
   }
   msgtemp[0]='0', msgtemp[1]='0', msgtemp[2]='0', msgtemp[3]='0', msgtemp[4]='0', msgtemp[5]='0';
  dtostrf(temp, 4, 2, msgtemp);
  msgparam+=msgtemp;
  msgtemp[0]='0', msgtemp[1]='0', msgtemp[2]='0', msgtemp[3]='0', msgtemp[4]='0', msgtemp[5]='0';
  dtostrf(altitude, 4, 2, msgtemp);
   msgparam+=",";
  msgparam+=msgtemp;
   msgtemp[0]='0', msgtemp[1]='0', msgtemp[2]='0', msgtemp[3]='0', msgtemp[4]='0', msgtemp[5]='0';
  dtostrf(pressure, 6, 2, msgtemp);
   msgparam+=",";
  msgparam+=msgtemp;
  msgparam.toCharArray(msgtemp, msgparam.length(),0);
  client.publish(esp32, msgtemp);  
}

/*void handle_OnConnect() {
  server.send(200, "text/html", HTML(fall));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}*/
void mpu_read(){
 Wire.beginTransmission(MPU_addr);
 Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
 Wire.endTransmission(false);
 Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
 AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
 AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
 AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
 Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
 GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
 GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
 GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
 }
//Ajax code
String HTML(bool falling) {
 String str = "<!DOCTYPE html>";
  str += "<html>";
  str += "<head>";
  str += "<title>HAS_Health Web Server</title>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  str += "<link rel='stylesheet' href='http://10.42.0.1/web/css/all.min.css'>";
  str += "<link rel='stylesheet' type='text/css' href='styles.css'>";
  str += "<style>";
  str += "body { background-color: #fff; font-family: sans-serif; color: #0f0d0d; font: 14px Helvetica, sans-serif box-sizing: border-box;}";
  str += "#page { margin: 20px; background-color: #fff;}";
  str += ".container { height: inherit; padding-bottom: 20px;}";
  str += ".header { padding: 20px;}";
  str += ".header h1 { padding-bottom: 0.3em; color: #0f0d0d; font-size: 40px; font-weight: bold; font-family: Garmond, 'sans-serif'; text-align: center;}";
  str += ".box-full { padding: 20px; border 1px solid #ddd; border-radius: 1em 1em 1em 1em; box-shadow: 1px 7px 7px 1px rgba(0,0,0,0.4); background: #fff; margin: 20px; width: 300px;}";
  str += "@media (max-width: 494px) { #page { width: inherit; margin: 5px auto; } #content { padding: 1px;} .box-full { margin: 8px 8px 12px 8px; padding: 10px; width: inherit;; float: none; } }";
  str += "@media (min-width: 494px) and (max-width: 980px) { #page { width: 465px; margin 0 auto; } .box-full { width: 380px; } }";
  str += "@media (min-width: 980px) { #page { width: 930px; margin: auto; } }";
  str += ".sensor { margin: 12px 0px; font-size: 2.5rem;}";
  str += ".sensor-labels { font-size: 1rem; vertical-align: middle; padding-bottom: 15px;}";
  str += ".units { font-size: 1.2rem;}";
  str += "hr { height: 1px; color: #eee; background-color: #eee; border: none;}";
  str += "</style>";

  //Ajax Code Start
  str += "<script>\n";
  str += "setInterval(loadDoc,1000);\n";
  str += "function loadDoc() {\n";
  str += "var xhttp = new XMLHttpRequest();\n";
  str += "xhttp.onreadystatechange = function() {\n";
  str += "if (this.readyState == 4 && this.status == 200) {\n";
  str += "document.body.innerHTML =this.responseText}\n";
  str += "};\n";
  str += "xhttp.open(\"GET\", \"/\", true);\n";
  str += "xhttp.send();\n";
  str += "}\n";
  str += "</script>\n";
  //Ajax Code END

  str += "</head>";
  str += "<body>";
  str += "<div id='page'>";
  str += "<div class='header'>";
  str += "<h1>HAS Health</h1>";
  str += "</div>";
  str += "<div id='content' align='center'>";
  str += "<div class='box-full' align='left'>";
  str += "<div class='sensors-container'>";

  //For Heart Rate
  str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/heart.png'></img>";
  str += "<span class='sensor-labels'> Heart Rate </span>";
  str += bpm;
  str += " BPM";
  str += "</p>";
  str += "<hr>";

  //For Sp02
  str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/oxygen.png'></img>";
  str += "<span class='sensor-labels'> SpO2 </span>";
  str += spo;
  str += " %";
  str += "</p>";
  str += "<hr>";
if (falling ==true)
{ 
 if (gps.location.isValid())
  {
  Serial.println("Neo");
  MY_LAT=String(gps.location.lat(),6);
  MY_LON=String(gps.location.lng(),6);
  }
  else 
  {
    Serial.println("BLE");
  MY_LON= String(Sensor.getGPSlongitude(),6);
  MY_LAT=String(Sensor.getGPSLatitude(),6);
  }
  String comma =",";
  String stringThree = MY_LAT + comma + MY_LON;
  stringThree.toCharArray(buffloc,stringThree.length());
  client.publish("esploc", buffloc);
  str += "<p class='sensor'>";
   str += "<img src='http://10.42.0.1/web/ambul.png'></img>";
   str += "  FALL";
   str += "</p>";
   str += "<hr>";
   str += "<span class='sensor-labels'> Location </span>";
   str +=  "<a href=\"http://maps.google.com/maps?&z=18&mrt=yp&t=k&q=";
   str+=MY_LAT;
   str+='+';             
   str+=MY_LON;
   str+="\">Patient Live Location!</a>";

   str += "<hr>";
}
else
{
    str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/healthy.png'></img>";
  str += "  OK";
  str += "</p>";
   str += "<hr>";
}
  str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/temp.png'></img>";
  str += "<span class='sensor-labels'> Room Temp </span>";
  str += temp;
  str += "°C";
  str += "</p>";
  str += "<hr>";
  str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/sea.png'></img>";
  str += "<span class='sensor-labels'> Altitude from sea level </span>";
  str += altitude;
  str += "m";
  str += "</p>";
  str += "<hr>";
  str += "<p class='sensor'>";
  str += "<img src='http://10.42.0.1/web/press.png'></img>";
  str += "<span class='sensor-labels'> Atmospheric Pressure </span>";
  str += temp;
  str += "hPa";
  str += "</p>";
  str += "<hr>";
  
  str += "</p>";
  str += "</div>";
  str += "</div>";
  str += "</div>";
  str += "</div>";
  str += "</div>";
  str += "</body>";
  str += "</html>";
  return str;
}

void callback(char* topic, byte* payload, unsigned int length) {
 datarecvd="";
for (int i = 0; i < length; i++) {
    datarecvd+= ((char)payload[i]);
  }
if (String(topic) == "HasHealthEsp01"){
  bpm="", spo="";
   int ind1 = datarecvd.indexOf(',');  //finds location of first ,
   bpm = datarecvd.substring(0, ind1);   //captures first data String
   int ind2 = datarecvd.indexOf(',', ind1+1);   //finds location of second ,
   spo = datarecvd.substring(ind1+1, ind2);
 }
if (String(topic) == "alarm")
  {
  int ind1 = datarecvd.indexOf('?');  //finds location of first ,
  ast = datarecvd.substring(0,ind1);
   //First Medicine Reminder code
 if (ast=="S1")
  {
  ast1="S1",ahr="",amn="",ady0="",ady1="",ady2="",ady3="",ady4="",ady5="",ady6="",med1="",med2="";  
  int ind2 = datarecvd.indexOf(';', ind1+1);  //finds location of first ,
 }

  int ind3 = datarecvd.indexOf(':', ind2);  //finds location of first ,
  ahr = datarecvd.substring(ind2+1,ind3);   //captures first data String
  int ind4 = datarecvd.indexOf(',', ind3+1);   //finds location of second ,
  amn = datarecvd.substring(ind3+1, ind4);
  int count =(ind4+1);
  while (count<length){
   switch (char(payload[count])){
    case '0': ady0="0"; break;
    case '1': ady1="1"; break;
    case '2': ady2="2";break;
    case '3': ady3="3";break;
    case '4': ady4="4";break;
    case '5': ady5="5";break;
    case '6': ady6= "6";break;
    default: ady0="0",ady1="1",ady2="2",ady3="3",ady4="4",ady5="5",ady6= "6";break;
            }
    count=count+2;
       }
    }
    else if (ast =="D1")
    {
      ast1="D1", ahr="",amn="",ady0="",ady1="",ady2="",ady3="",ady4="",ady5="",ady6="";
    }
  //Second Medicine Reminder code
   if (ast=="S2")
  {
   Serial.println("Second Rem set");
   ast2="S2", ahr2="",amn2="",ady20="",ady21="",ady22="",ady23="",ady24="",ady25="",ady26="",med21="",med22="";  
     }

  int ind3 = datarecvd.indexOf(':', ind2);  //finds location of first ,
  ahr2 = datarecvd.substring(ind2+1,ind3);   //captures first data String
  int count =(ind4+1);
  while (count<length){
   switch (char(payload[count])){
    case '0': ady20="0"; break;
    case '1': ady21="1"; break;
    case '2': ady22="2";break;
    case '3': ady23="3";break;
    case '4': ady24="4";break;
    case '5': ady25="5";break;
    case '6': ady26= "6";break;
    default: ady20="0",ady21="1",ady22="2",ady23="3",ady24="4",ady25="5",ady26= "6";break;
            }
    count=count+2;
       }
    }

}