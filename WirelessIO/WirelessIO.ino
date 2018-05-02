/*
 * 名称：多功能无线物联实验平台
 * 作者：周米笑
 * 邮件：mixiao.zhou@qq.com
 * Github: https://github.com/mixiao07/wirelessio
 * 版本：2018-05-01 1.0.0
 * 备注：
 *    材料：
 *    ESP8266
 *    LED2812
 *    继电器
 *    Server
 *    人体传感器
 *    其他：面包板、杜邦线、电源等
 *    
 *    开发工具：
 *    Arduino 1.8.1
 */

#include <ESP8266WiFi.h>
#include <Servo.h>
#include <dht11.h>
#include <ColorRecognition.h>
#include <ColorRecognitionTCS230PI.h>
#include "Adafruit_NeoPixel.h"

const char* ssid     = "Xiaomi_1201";
const char* password = "19850322";
const char* host = "192.168.31.248";
const int hostPort = 1180;

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(16, OUTPUT);  // LED pin
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

bool isConnected = false;
int  disCount = 0;
int  cycle = 20;
WiFiClient client;

bool cmd_process(const char *pcmd);

void loop() {
  if (!isConnected) {
    Serial.print("connecting to ");
    Serial.print(host);
    Serial.print(" port ");
    Serial.println(hostPort);
    if (!client.connect(host, hostPort)) {
      Serial.println("connection failed");
      delay(1000);
      return;
    }
    isConnected = true;
    disCount = 0;
    Serial.println("connect to host ok!");
  }

  if (!client.connected()) {
    disCount++;
    if (disCount > 100) {
      isConnected = false;
      Serial.println("not connected, re-connect");
    }
    delay(30);
    return;    
  }  
  
  while(client.available()){
    char *pcmd;
    String cmd = client.readStringUntil(';');
    pcmd = (char*)cmd.c_str();
    if(strncmp(pcmd, "CMD:", 4) != 0) {
      continue;
    }
    Serial.print("Get command from host: ");
    Serial.println(pcmd);
    if (cmd_process(pcmd+4)) {
      client.print("ACK:OK\r\n");
    }
    else {
      client.print("ACK:NG\r\n");
    }
  }
  
  delay(cycle);
}

bool isStrContainX(const char* px, const char *pstr)
{
  if (px == NULL || pstr == NULL)
    return false;
  if (strncmp(px, pstr, strlen(px)) == 0)
    return true;
  return false;
}

#define CMD_BEAT                    ("Beat")
#define CMD_MAC                     ("Mac")
#define CMD_PINMODE                 ("pinMode")
#define CMD_DIGITAL_READ            ("digitalRead")
#define CMD_DIGITAL_WRITE           ("digitalWrite")
#define CMD_ANALOG_READ             ("analogRead")
#define CMD_ANALOG_WRITE            ("analogWrite")

#define CMD_LED2812                 ("LED2812")
#define CMD_SERVER                  ("Server")
#define CMD_COLOR                   ("Color")
#define CMD_DHT11                   ("DHT11")

#define IS_CMD_X(x,cmd)             (isStrContainX(x, cmd))

bool cmd_led2812_process(const char *pcmd);
bool cmd_server_process(const char *pcmd);
bool cmd_color_process(const char* pcmd);
bool cmd_dht11_process(const char *pcmd);

bool cmd_process(const char *pcmd)
{
  if (IS_CMD_X(CMD_BEAT, pcmd)) {
    return true;    
  }
  else if (IS_CMD_X(CMD_MAC, pcmd)) {    
    String msg = "ACK:";
    msg += WiFi.macAddress();
    msg += "\r\n";
    client.print(msg);
    Serial.println(msg);
    return true;
  }
  else if (IS_CMD_X(CMD_PINMODE, pcmd)) {
    // CMD:pinMode,x,output/input
    pcmd+=strlen(CMD_PINMODE)+1;
    int pin=atoi(pcmd);
    if(pin>32)
    {
      Serial.println("Invalid pin > 9");
      return false;
    }
    
    pcmd=strpbrk(pcmd, ",");
    if(pcmd==NULL)
    {
        Serial.println("Invalid pcmd without mode");
        return false;      
    }
    pcmd++;
    int mode;
    if(isStrContainX("input", pcmd))
    {
      mode=INPUT;
    } 
    else if(isStrContainX("output", pcmd))
    {
      mode=OUTPUT;
    }
    else
    {
      Serial.println("Invalid mode");
      return false;
    }
    pinMode(pin,mode);
    
    String log="pinMode(";
    log += pin;
    log +=  ", ";
    log += mode;
    log += ")";
    Serial.println(log);
    return true;
  }
  else if (IS_CMD_X(CMD_DIGITAL_READ, pcmd)) {
    pcmd+=strlen(CMD_DIGITAL_READ)+1;
    int pin;
    pin=atoi(pcmd);
    if(pin>32)
    {
      Serial.println("Invalid digitalRead value");
      return false;
    }
    int value=digitalRead(pin);
    String log="digitalRead(";
    log+=pin;
    log+=") = ";
    log+=value;
    Serial.println(log);
    String msg = "RET:";
    msg += value;
    msg += "\r\n";
    client.print(msg);
    return true;   
  }
  else if(IS_CMD_X(CMD_DIGITAL_WRITE,pcmd)) {
    // CMD:digitalWrite,pin,value (0:low, 1:high)
    pcmd+=strlen(CMD_DIGITAL_WRITE)+1;
    int pin;
    pin=atoi(pcmd);
    if(pin>32)
    {
      Serial.println("Invalid pin value");
      return false;
    }
    
    int value;
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL)
    {
      Serial.println("Invalid CMD_DIGITAL_WRITE value");
      return false;
    }
    pcmd++;
    value=atoi(pcmd);
    if(value==0)
    {
      value=LOW;
    }
    else if(value==1)
    {
      value=HIGH;
    }
    else
    {
      Serial.printf("Invalid value");
      return false;
    }
    digitalWrite(pin, value);
    String log="digitalWrite(";
    log += pin;
    log += ", ";
    log += value;
    log += ")";
    Serial.println(log);
    return true;
  }
  else if(IS_CMD_X(CMD_ANALOG_READ,pcmd)){
    // CMD:analogRead,x
    pcmd+=strlen(CMD_ANALOG_READ)+1;
    int pin;
    pin=atoi(pcmd);
    if(pin!=0)
    {
      Serial.println("Invalid analogRead value");
      return false;
    }
    pinMode(A0, INPUT);
    int value=analogRead(A0);
    String log="analogRead(";
    log += A0;
    log += ") = ";
    log += value;
    Serial.println(log);
    String msg = "RET:";
    msg += value;
    msg += "\r\n";
    client.print(msg);
    return true;
  }
  else if(IS_CMD_X(CMD_ANALOG_WRITE,pcmd)){
    // CMD:pin,value;
    int pin = atoi(pcmd);
    if (pin > 32)
    {
      return false;
    }
    pcmd = strpbrk(pcmd, ",");
    if (pcmd == NULL)
    {
      return false;
    }
    pcmd++;
    int value = atoi(pcmd);
    if (value > 1024)
    {
      return false;
    }
    analogWrite(pin, value);
    String log = "analogWrite(";
    log += pin;
    log += ", ";
    log += value;
    log += ");";
    Serial.println(log);
    return true;
  }
  else if (IS_CMD_X(CMD_LED2812,pcmd)) {
    pcmd += strlen(CMD_LED2812) + 1;
    return cmd_led2812_process(pcmd);
  }
  else if (IS_CMD_X(CMD_SERVER,pcmd)) {
    pcmd += strlen(CMD_SERVER) + 1;
    return cmd_server_process(pcmd);
  }
  else if (IS_CMD_X(CMD_COLOR,pcmd)) {
    pcmd += strlen(CMD_COLOR) + 1;
    return cmd_color_process(pcmd);
  }
  else if (IS_CMD_X(CMD_DHT11,pcmd)) {
    pcmd += strlen(CMD_DHT11) + 1;
    return cmd_dht11_process(pcmd);
  }
  
  return false;
}


Adafruit_NeoPixel *pled2812=NULL;
bool cmd_led2812_process(const char *pcmd)
{
  // CMD:led2812,pin,N,{i,r,g,b}....;
  int pin=atoi(pcmd);
  if(pin>32)
  {
    Serial.println("Invalid pin value");
    return false;
  }
  pcmd=strpbrk(pcmd,",");
  if(pcmd==NULL)
  {
    return false;
  }
  int n;
  pcmd++;
  n=atoi(pcmd);
  if(n<=0)
  {
    Serial.println("Invalid n value");
    return false;
  }
  
  if (pled2812 == NULL) {
    pled2812 = new Adafruit_NeoPixel(n, pin, NEO_GRB + NEO_KHZ800);
    pled2812->begin();
  }
  else if ((pled2812->numPixels() != n) || (pled2812->getPin() != pin)) {
    delete pled2812;
    pled2812 = new Adafruit_NeoPixel(n, pin, NEO_GRB + NEO_KHZ800);
    pled2812->begin();
  }

  while(true) {
    pcmd = strpbrk(pcmd, "{");
    if(pcmd==NULL)
    {
      break;
    }
    int i;
    pcmd++;
    i=atoi(pcmd);
    if(i>=n || i<0)
    {
      continue;
    }
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL) continue;
    pcmd++;
    int r;
    r=atoi(pcmd);
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL) continue;
    pcmd++;
    int g;
    g=atoi(pcmd);
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL) continue;
    pcmd++;
    int b;
    b=atoi(pcmd);
    
    uint32_t color = pled2812->Color(r, g, b);
    pled2812->setPixelColor(i, color);
  }
  pled2812->show();
  
  Serial.println("led2812 show");
  return true;
}

Servo server_x;
bool cmd_server_process(const char *pcmd)
{
  // CMD:Server,pin,degree
  int pin=atoi(pcmd);
  if(pin>32)
  {
    Serial.println("Invalid pin value");
    return false;
  }
  int degree;
  pcmd=strpbrk(pcmd,",");
  if(pcmd==NULL)
  {
    return false;
  }
  pcmd++;
  degree=atoi(pcmd);
  if(degree<0 || degree>180)
  {
    Serial.println("Invalid degree value");
    return false;
  }
  server_x.attach(pin);
  server_x.write(degree);
  delay(100);
  Serial.println("server run");
  return true;
}

ColorRecognitionTCS230PI *ptcs230 = NULL;
bool cmd_color_process(const char* pcmd)
{
  if (isStrContainX("pin", pcmd)) {
    // CMD:Color,pin,out,s2,s3
    pcmd += strlen("pin") + 1;

    int out;
    out=atoi(pcmd);
    if(out>32)
    {
      Serial.println("Invalitd out value");
      return false;
    }
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL)
    {
      return false;
    }
    pcmd++;
    int s2;
    s2=atoi(pcmd);
    if(s2>32)
    {
      Serial.println("Invalitd s2 value");
      return false;
    }
    pcmd=strpbrk(pcmd,",");
    if(pcmd==NULL)
    {
      return false;
    }
    pcmd++;
    int s3;
    s3=atoi(pcmd);
    if(s3>32)
    {
      Serial.println("Invalitd s3 value");
      return false;
    }

    if (ptcs230 != NULL) {
      delete ptcs230;
      ptcs230 = NULL;
    }
    ptcs230 = new ColorRecognitionTCS230PI(out, s2, s3);
    if (ptcs230 != NULL) {
      Serial.println("New Color Recognition OK");
    }
    else {
      Serial.println("New Color Recognition Fail");
      return false;
    }
  }
  else if (isStrContainX("whiteBalance", pcmd)) {
    //CMD:Color,whiteBalance;
    if (ptcs230 != NULL) {
      Serial.println("Start WhiteBalance");
      ptcs230->adjustWhiteBalance();
    } 
    else {
      Serial.println("Invalid ptcs230");
      return false;
    }
  }
  else if (isStrContainX("balckBalance", pcmd)) {
    //CMD:Color,balckBalance;
    if (ptcs230 != NULL) {
      Serial.println("Start BlackBalance");
      ptcs230->adjustBlackBalance();
    } 
    else {
      Serial.println("Invalid ptcs230");
      return false;
    }
  }
  else if (isStrContainX("colorRead", pcmd)) {
    //CMD:colorRead;
    if (ptcs230 == NULL) {
      Serial.println("Invalid ptcs230");
      return false;
    }
    
    int r, g, b;
    r = ptcs230->getRed();
    g = ptcs230->getGreen();
    b = ptcs230->getBlue();

    String msg = "RET:";
    msg += r;
    msg += ",";
    msg += g;
    msg += ",";
    msg += b;
    msg += ";\r\n";
    Serial.println(msg);
    client.print(msg);
  }
  else {
    return false;
  }
  
  return true;  
}

bool cmd_dht11_process(const char *pcmd)
{
  // CMD:DHT11,pin;
  int pin = atoi(pcmd);
  if (pin > 32)
  {
    Serial.println("Invalid pin value");
    return false;
  }

  dht11 DHT11;
  int ret = DHT11.read(pin);
  if (DHTLIB_OK != ret)
  {
    Serial.print("DHT11.read fail: ");
    Serial.println(ret);
    return false;
  }
  String msg = "RET:";
  msg += DHT11.temperature;
  msg += ",";
  msg += DHT11.humidity;
  msg += "\r\n";
  client.print(msg);
  Serial.println(msg);
  return true;
}

