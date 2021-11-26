/*
Please install the following dependent libraries before compiling:
M5Atom: https://github.com/m5stack/M5Atom
FastLED: https://github.com/FastLED/FastLED
pubsubclient: https://github.com/knolleary/pubsubclient
*/

#include <M5Atom.h>
#include "PrinterApi.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <Preferences.h>
#include <PubSubClient.h>
#include <DNSServer.h>


const byte DNS_PORT = 53;

//MQTT设置
#define MQTT_BROKER "mqtt.m5stack.com"
#define MQTT_PORT 1883
#define MQTT_ID "atomPrinter"
String MQTT_TOPIC;

//打印机模式
#define UIFLOW 0
#define APMode 1
uint8_t printerMode;
bool setWifi;//判断是否需要设置wifi密码

//wifi设置
const IPAddress apIP(192, 168, 4, 1);
const char *apSSID = "ATOM_PRINTER";
String ssidList;//wifi列表
String wifiSSID;//保存flash中的wifi名称
String wifiPASSWORD;//保存flash中的wifi密码
String MAC;

//保存网页获取的数据
String Pdata, newLine, QRCode, AdjustLevel, printType, BarCode, BarType, Position; 

//判断打印机是否打印完成
bool print_flag;
bool send_flag;

WebServer webServer(80);
DNSServer dnsServer;
WiFiClient client;
PubSubClient  mqttClient(MQTT_BROKER, MQTT_PORT, client);
Preferences preferences;
Printer atomPrinter;
HardwareSerial AtomSerial(1);

void flashing(uint32_t color,uint8_t frequency); //rgb灯的闪烁 color：颜色 frequency：频率(ms)
boolean checkConnection(); //查看wifi连接情况 正常 return TRUE 错误 return FALSE
void mqttConnect(); //连接mqtt
void setWifiMode(); //不同模式下网络的初始化
void setWebServer(); //不同模式下网页的设置

void flashing(uint32_t color,uint8_t frequency){
  static uint32_t prev_ms = millis();
  static bool rgbState = 0;
  if (millis() > prev_ms + frequency) {
      prev_ms = millis();
      rgbState=!rgbState;
  }
  M5.dis.drawpix(0, color*rgbState);
}

void setup(){
    M5.begin(true, false, true);
    preferences.begin("printer-config");  
    if(preferences.getBytesLength("PRINTER_MODE"==0)){
      //如果flash上不存在打印机的模式时，将保存APMode
      preferences.putBool("PRINTER_MODE", APMode);
    }
    M5.dis.drawpix(0,0x00ffff);//初始化状态灯
    atomPrinter.Set_Printer_Uart(AtomSerial,23,33,9600);
    atomPrinter.Printer_Init();
    atomPrinter.NewLine_Setting(0x0A);
    send_flag = 0;
    print_flag = 0;
    setWifi = 0;
    
    //获取当前搜索到的wifi名称列表
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    delay(100);
    for (int i = 0; i < n; ++i)
    {
        ssidList += "<option value=\"";
        ssidList += WiFi.SSID(i);
        ssidList += "\">";
        ssidList += WiFi.SSID(i);
        ssidList += "</option>";
    }

    wifiSSID = preferences.getString("WIFI_SSID");
    wifiPASSWORD = preferences.getString("WIFI_PASSWD");
    printerMode = preferences.getBool("PRINTER_MODE");
    MAC=WiFi.softAPmacAddress();
    
    /*打印机模式选择：
      当flash没有保存wifi信息：1.当前状态为AP -> 保持APMode
                              2.当前状态为UIFLOW -> 切换至APMode，并提示需要输入wifi信息
      当flash保存了wifi信息  ：1.当前状态为AP -> 保持APMode
                              2.当前状态为UIFLOW -> 保持UIFLOW
    */
    if (wifiSSID.length() > 0){
      if(printerMode==APMode){
        printerMode = APMode;
      }
      else{
        printerMode = UIFLOW;
      }    
    }
    else{
      if(printerMode==APMode){
        printerMode = APMode;
      }
      else if(printerMode==UIFLOW){
        printerMode = APMode;
        setWifi=1;
        Serial.println("you need to set the wifi ssid and password");
        AtomSerial.print("you need to set the wifi ssid and password");
        atomPrinter.Print_NewLine(1);
      }
    }
    setWifiMode();
    setWebServer();
}

int status = WL_IDLE_STATUS; 
bool buttonSwich=0;
uint8_t Data[1000], Length;
long startTime=0,endTime=0;
void loop() 
{
  if (printerMode == APMode)
  {
    if(setWifi){
      if(WiFi.softAPgetStationNum()){
        M5.dis.drawpix(0, 0xffff00);
      }
      else{
        flashing(0xffff00,300);
      }
    }
    else{
      if(WiFi.softAPgetStationNum()){
        M5.dis.drawpix(0, 0x00ff00);
      }
      else{
        flashing(0x00ff00,300);
      }
    }
    if (print_flag)
    {
      if (printType == "ASCII")
      {
        atomPrinter.Printer_Init();
        atomPrinter.Print_ASCII(Pdata);
      }
      else if (printType == "QRCode")
      {
        atomPrinter.Printer_Init();
        atomPrinter.Set_adjlevel(AdjustLevel);
        atomPrinter.Set_QRCode(QRCode);
        atomPrinter.Print_QRCode();
        
      }
      else if (printType == "BarCode")
      {
        atomPrinter.Printer_Init();
        atomPrinter.BarCode_switch(1);
        atomPrinter.Set_BarCode_HRI(Position);
        atomPrinter.Print_BarCode("CODE128",BarCode);
        atomPrinter.BarCode_switch(0);
      }
      if (newLine == "on")
      {
        atomPrinter.Print_NewLine(1);
      }
      print_flag = 0;
    }
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
  else if (printerMode == UIFLOW){
    if(wifiSSID.length() > 0){
      while(mqttClient.state() != 0) {
          while (WiFi.status() != WL_CONNECTED) {
            flashing(0x0000ff,500);
            WiFi.begin(wifiSSID.c_str(), wifiPASSWORD.c_str());
            delay(300);
          }
          Serial.println(F("=== MQTT IS CONNECTING ==="));
          mqttConnect();
      }
      M5.dis.drawpix(0, 0x0000ff);
      mqttClient.loop();
      webServer.handleClient();
    }
    else{
      printerMode = APMode;
      dnsServer.processNextRequest();
      webServer.handleClient();
    }
  }
  Length = 0;
  while (Serial.available())
  {
    Data[Length] = Serial.read();
    Length++;
    send_flag = 1;
  }
  if (send_flag)
  {
    for (uint8_t j = 0; j < Length; j++)
    {
      AtomSerial.write(Data[j]);
    }
    send_flag = 0;
  }

  //计算按键按下时间
  if(M5.Btn.wasPressed()){
    startTime=millis();
  }
  if(M5.Btn.wasReleased()){
    endTime=millis();
    buttonSwich=1;
  }

  if(buttonSwich==1){
    if((endTime-startTime)>2000){
      //长按：打印示例
      atomPrinter.Printer_Init();
      atomPrinter.Print_ASCII("Hello M5stack");
      atomPrinter.Print_NewLine(2);
      atomPrinter.Printer_Init();
      atomPrinter.Set_adjlevel("L");
      atomPrinter.Set_QRCode("https://www.M5Stack.com");
      atomPrinter.Print_QRCode();
      atomPrinter.Printer_Init();
      atomPrinter.BarCode_switch(1);
      atomPrinter.Set_BarCode_HRI("Below");
      atomPrinter.Print_BarCode("CODE128","123456789ABC");
      atomPrinter.BarCode_switch(0);
      atomPrinter.Print_NewLine(3);
      atomPrinter.Printer_Init();
      delay(2000);
    }
    else{
      //短按：切换打印机当前状态
      if(setWifi==1){
        printerMode=UIFLOW;
      }
      printerMode = !printerMode;
      preferences.remove("PRINTER_MODE");
      preferences.putBool("PRINTER_MODE",printerMode);
      ESP.restart(); 
    }
    buttonSwich=0;
  }
  M5.update();
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{ 
  //mqtt回调函数：将从订阅主题获得的信息通过串口打印
  char PayloadData[len+1];
  strncpy(PayloadData,(char*)payload,len);
  PayloadData[len] = '\0';
  Serial.print(MQTT_TOPIC+":");
  Serial.println(String(PayloadData));
  AtomSerial.print(String(PayloadData));
}

boolean checkConnection() 
{
    uint8_t count=0;
    Serial.print("Waiting for Wi-Fi connection");
    while (count<10)
    {
        if(WiFi.status() == WL_CONNECTED){
        Serial.println("Connected!");
        MQTT_TOPIC=MAC;
        AtomSerial.print("Topic: ");
        AtomSerial.print(MAC);
        atomPrinter.Print_NewLine(1);
        return (true);
        }
        Serial.print(".");
        delay(1000);
        count++;
    }
    Serial.println("Timed out.");
    return false;
}

void mqttConnect(void)
{
    static bool State=0;
    Serial.print(F("Connecting"));
    String mqttid = ("MQTTID_" + String(random(65536)));
    while(!mqttClient.connect(mqttid.c_str())){
        Serial.print(F("."));
        State=!State;
        M5.dis.drawpix(0, 0xff0000);
    }
    Serial.println(F(" success"));

    AtomSerial.print(F("successfully connect to UIFLOW"));
    atomPrinter.Print_NewLine(3);

    AtomSerial.println("subscribe: " + MQTT_TOPIC);
    mqttClient.subscribe(MQTT_TOPIC.c_str());
}


String urlDecode(String input)
{
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

void handleRoot() {
  String s = "<html><head><meta  charset=\"gbk\" name=\"viewport\" content=\"width=device-width,user-scalable=0\"><div align=\"center\"><title>ATOM_Printer</title></head>";
  s += "<body><h1>ATOM Printer</h1><p></p><iframe id=\"id_frame\" name = \"id_frame\" style=\"display:none;\"></iframe><form id=\"print\" method=\"get\" action=\"\" target=\"id_frame\"><p><input type=\"radio\" name=\"printType\" value=\"ASCII\"><font size=\"6\"><strong>ASCII</strong></font><br><input id=\"Pdata\" length=\"256\" type=\"text\" name=\"Pdata\" onkeyup=\"value=value.replace(/[^\w\u4E00-\u9FA5]/g,'')\"></p>";
  s += "<p><input type=\"radio\" name=\"printType\" value=\"QRCode\"><font size=\"6\"><strong>QRCode</strong></font><br><strong>Adjust Level:</strong><select name=\"AdjustLevel\"><option>L</option><option>M</option><option>Q</option><option>H</option></select><br><input id=\"QRCode\" length=\"256\" type=\"text\" name=\"QRCode\" onkeyup=\"value=value.replace(/[^\w\u4E00-\u9FA5]/g,'')\"></p>";
  s += "<p><input type=\"radio\" name=\"printType\" value=\"BarCode\"><font size=\"6\"><strong>BarCode</strong></font><br><strong>Set HRI:</strong><select name=\"Position\"><option>Hide</option><option>Above</option><option>Below</option><option>Both</option></select><br>";
  s += "<input id=\"BarCode\" maxlength=\"18\" type=\"text\" name=\"BarCode\" onkeyup=\"value=value.replace((/[\W]/g,'')\"</p>";
  s += "<p><input type=\"checkbox\" name=\"newLine\" value=\"on\">newLine</p>";
  s += "<p><input type=\"submit\" value=\"print\"></p>";
  s += "</form>";
  s += "<h1>Wi-Fi Settings For UIFLOW</h1><p>Please enter your password by selecting the SSID.</p>";
  s += "<form id=\"wifi\" method=\"get\" action=\"/Connect\"><label>SSID: </label><select name=\"ssid\">";
  s += ssidList;
  s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
  s += "</div></body></html>";
  webServer.send(200, "text/html", s);
  Pdata = urlDecode(webServer.arg("Pdata"));
  newLine = urlDecode(webServer.arg("newLine"));
  QRCode = urlDecode(webServer.arg("QRCode"));
  printType = urlDecode(webServer.arg("printType"));
  AdjustLevel = urlDecode(webServer.arg("AdjustLevel"));
  BarType = urlDecode(webServer.arg("BarType"));
  BarCode = urlDecode(webServer.arg("BarCode"));  
  Position = urlDecode(webServer.arg("Position"));
  print_flag = 1;
}

String makePage(String title, String contents)
{
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><div align=\"center\"><body>";
  s += contents;
  s += "</body></div></html>";
  return s;
}

void setWebServer() 
{
  if (printerMode == APMode) {
    dnsServer.start(DNS_PORT,"*", apIP);//强制门户认证（需要设置notfound时候的网页，否则不会弹出）
    webServer.on("/", handleRoot);
    webServer.on("/Connect", [](){
        String ssid = urlDecode(webServer.arg("ssid"));
        Serial.printf("SSID: %s\n", ssid);
        String pass = urlDecode(webServer.arg("pass"));
        Serial.printf("Password: %s\n\nWriting SSID to EEPROM...\n", pass);
        
        Serial.println("Writing Password to nvr...");
        preferences.putString("WIFI_SSID", ssid);
        preferences.putString("WIFI_PASSWD", pass);
        preferences.remove("PRINTER_MODE");
        preferences.putBool("PRINTER_MODE",UIFLOW);
        Serial.println("Write nvr done!");

        String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
        s += ssid;
        s += "\" after the restart.";
        webServer.send(200, "text/html", makePage("Wi-Fi connect", s));
       
        delay(2000);
        ESP.restart();
    });
    webServer.onNotFound(handleRoot);
  }
  else if(printerMode ==  UIFLOW){
    webServer.on("/", []() { 
      String s = "<h1>MQTT mode</h1><br><br>";
      s += "<p><strong>DATA TOPIC: </strong><font color=\"#0000ff\">";
      s += MQTT_TOPIC;
      s += "</font></p>";
      s += "<h1>Change WiFi Settings/Mode</h1>";
      s += "<form method=\"get\" action=\"/Connect\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      s += "<p><a href=\"/switch\">Change to APMode</a></p>";
      s += "<p><a href=\"/clean\">Clean the wifi config</p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });

    webServer.on("/Connect", [](){
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.printf("SSID: %s\n", ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.printf("Password: %s\n\nWriting SSID to EEPROM...\n", pass);

      Serial.println("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);

      Serial.println("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi connect", s));
      delay(2000);
      ESP.restart(); 
    });    
    
    webServer.on("/clean", [](){
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");

      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(2000);
      ESP.restart();
    });

      webServer.on("/switch", [](){
      preferences.remove("PRINTER_MODE");
      preferences.putBool("PRINTER_MODE",APMode);
      String s = "<h1>Printer has change to APMode</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(2000);
      ESP.restart();
    });
  }
  webServer.begin();
  Serial.println("HTTP server started");  
}

void setWifiMode()
{
    if(printerMode==APMode){
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        String No = MAC.substring(MAC.length()-5,MAC.length()-3)+MAC.substring(MAC.length()-2,MAC.length());

        String ssid=("ATOM-PRINTER_"+No);
        const char *softAPName = ssid.c_str();
        WiFi.softAP(softAPName); 
        Serial.println("Mode: AP");
        Serial.print("AP SSID: ");
        Serial.println(softAPName);
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());  //IP address assigned to your ESP  获取ip地址

        AtomSerial.print("Mode: AP");
        atomPrinter.Print_NewLine(1);
        AtomSerial.print("AP SSID: ");
        AtomSerial.print(ssid);
        atomPrinter.Print_NewLine(1);
        AtomSerial.print("IP address: ");
        AtomSerial.print(WiFi.softAPIP());  //IP address assigned to your ESP  获取ip地址
        atomPrinter.Print_NewLine(3);
    }
    else if(printerMode == UIFLOW){
        WiFi.mode(WIFI_MODE_STA);
        Serial.print("WiFi SSID: ");
        Serial.println(wifiSSID);
        WiFi.begin(wifiSSID.c_str(), wifiPASSWORD.c_str());

        Serial.println("Mode: UIFLOW");
        AtomSerial.print("Mode: UIFLOW");
        atomPrinter.Print_NewLine(1);
        if(!checkConnection()){
            // preferences.remove("WIFI_SSID");
            // preferences.remove("WIFI_PASSWD");
            Serial.println("Password Erro!");
            AtomSerial.print("WiFi Password Erro!");
            atomPrinter.Print_NewLine(1);
            delay(1000);
            ESP.restart();
        }
        Serial.print("Settings URL: ");
        Serial.println(WiFi.localIP());  //IP address assigned to your ESP  获取ip地址

        AtomSerial.print("WiFi SSID: ");
        AtomSerial.print(wifiSSID.c_str());
        atomPrinter.Print_NewLine(1);

        AtomSerial.print("Settings URL: ");
        AtomSerial.print(WiFi.localIP());  //IP address assigned to your ESP  获取ip地址
        atomPrinter.Print_NewLine(1);

        const int httpPort = 80;
        const char* host = "ezdata.m5stack.com";
        WiFiClient client;

        if (!client.connect(host, httpPort)) {
            Serial.println("connection failed");
        }

        client.print(String("GET ") + "/api/store/common/now HTTP/1.1\r\n" +
                    "Host: "+ host + "\r\n" +
                    "Connection: close\r\n\r\n");
        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 5000) {
                Serial.println(">>> Client Timeout !");
                client.stop();
                break;
            }
        }
        String line;
        while(client.available()) {
            line = client.readStringUntil('\r');
            Serial.print(line);
        }
        AtomSerial.print("Time: ");
        AtomSerial.print(line);
        atomPrinter.Print_NewLine(1);
        mqttClient.setBufferSize(4096);
        mqttClient.setCallback(mqttCallback);
        mqttClient.setKeepAlive(10);
    }
}
