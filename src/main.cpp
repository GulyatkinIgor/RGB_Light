#include <GyverPortal.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

struct LoginPass
{
  char ssid[20];
  char pass[20];
  char tokk[40];
  char serv[20];
};

struct CurrentLight
{
  int r;
  int g;
  int b;
  int st;
  float br;
};

#define VPIN_StateLight V1
#define VPIN_BrightLight V10
#define VPIN_RedLight V8
#define VPIN_GreenLight V5
#define VPIN_BlueLight V6

#define RedLight D8
#define GreenLight D5
#define BlueLight D6
#define Indicate D4

int Red;
int Green;
int Blue;
float BrightLight;
int StateLight;

int Period = 1000;
uint32_t myTimer;

bool RedFlag;
bool GreenFlag;
bool BlueFlag;
bool StateFlag;
bool BrightFlag;

LoginPass lp;
CurrentLight RGB;

void build()
{
  GP.BUILD_BEGIN();
  GP.PAGE_TITLE("RGB light config control");
  GP.THEME(GP_DARK);
  GP.TITLE("Настройка параметров подключения контроллера RGB");
  GP.FORM_BEGIN("/login");
  GP.TEXT("lg", "Login", lp.ssid);
  GP.BREAK();
  GP.TEXT("ps", "Password", lp.pass);
  GP.BREAK();
  GP.TEXT("tk", "Tokken", lp.tokk);
  GP.BREAK();
  GP.TEXT("sv", "Server", lp.serv);
  GP.SUBMIT("Submit");
  GP.FORM_END();
  GP.BUILD_END();
}

void action(GyverPortal &p)
{
  if (p.form("/login"))
  {                           // кнопка нажата
    p.copyStr("lg", lp.ssid); // копируем себе
    p.copyStr("ps", lp.pass);
    p.copyStr("tk", lp.tokk);
    p.copyStr("sv", lp.serv);
    EEPROM.put(0, lp);       // сохраняем
    EEPROM.commit();         // записываем
    WiFi.softAPdisconnect(); // отключаем AP
    delay(3000);
    Serial.println();
    Serial.print("Restart ESP...");
    ESP.restart();
  }
}

void loginPortal()
{
  Serial.println("Portal start");
  // запускаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WiFi Config");
  delay(2000);
  // запускаем портал
  GyverPortal ui;
  ui.attachBuild(build);
  ui.start(WIFI_AP);
  ui.attach(action);
  while (ui.tick())
    ;
}

void connectWiFi()
{
  Serial.print("Connect to: ");
  Serial.println(lp.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(lp.ssid, lp.pass);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    n++;
    if (n >= 60)
    {
      n = 0;
      Serial.println();
      Serial.print("Not connected! Start AP ");
      loginPortal();
    }
  }
  Serial.println();
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.print("AP: ");
  Serial.println(lp.ssid);
}

void connectBlynk()
{
  int8_t count = 0;
  while (!Blynk.connected())
  {
    Serial.println();
    Serial.print("Connect to server blynk...");
    IPAddress serverIp;
    if (serverIp.fromString(lp.serv))
    {
      Serial.println(serverIp);
    }
    Blynk.config(lp.tokk, serverIp, 8080);
    Blynk.connect();
    Serial.print(".");
    delay(1000);
    if (count >= 30)
    {
      Serial.println();
      Serial.print("No respone...");
      Serial.println();
      Serial.print("restart ESP");
      count = 0;
      delay(2000);
      ESP.restart();
    }
    count++;
  }
  Serial.println();
  Serial.print("Blynk CONECTED!");
  Blynk.virtualWrite(VPIN_RedLight, RGB.r);
  Blynk.virtualWrite(VPIN_GreenLight, RGB.g);
  Blynk.virtualWrite(VPIN_BlueLight, RGB.b);
  Blynk.virtualWrite(VPIN_StateLight, RGB.st);
  Blynk.virtualWrite(VPIN_BrightLight, RGB.br);
  Serial.println();
  Serial.print("Data update");
}

void setup()
{
  pinMode(RedLight, OUTPUT);
  pinMode(GreenLight, OUTPUT);
  pinMode(BlueLight, OUTPUT);
  pinMode(Indicate, OUTPUT);
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  // читаем логин пароль из памяти
  EEPROM.begin(300);
  EEPROM.get(0, lp);
  EEPROM.get(200, RGB);
  connectWiFi();
  connectBlynk();
}

BLYNK_WRITE(VPIN_RedLight)
{
  Red = param.asInt();
  RGB.r = Red;
  EEPROM.put(200, RGB);
  EEPROM.commit();
  RedFlag = HIGH;
}
BLYNK_WRITE(VPIN_GreenLight)
{
  Green = param.asInt();
  RGB.g = Green;
  EEPROM.put(200, RGB);
  EEPROM.commit();
  GreenFlag = HIGH;
}
BLYNK_WRITE(VPIN_BlueLight)
{
  Blue = param.asInt();
  RGB.b = Blue;
  EEPROM.put(200, RGB);
  EEPROM.commit();
  BlueFlag = HIGH;
}
BLYNK_WRITE(VPIN_StateLight)
{
  StateLight = param.asInt();
  RGB.st = StateLight;
  StateFlag = HIGH;
  EEPROM.put(200, RGB);
  EEPROM.commit();
}
BLYNK_WRITE(VPIN_BrightLight)
{
  BrightLight = param.asInt();
  RGB.br = BrightLight * 0.04;
  BrightFlag = HIGH;
  EEPROM.put(200, RGB);
  EEPROM.commit();
}

void checkButtonLight()
{
  if (millis() - myTimer >= Period)
  {
    myTimer += Period;
    if (RedFlag and RGB.st)
    {
      if (RGB.st)
      {
        analogWrite(RedLight, RGB.r * RGB.br);
      }
      RedFlag = LOW;
    }
    if (GreenFlag)
    {
      if (RGB.st)
      {
        analogWrite(GreenLight, RGB.g * RGB.br);
      }
      GreenFlag = LOW;
    }
    if (BlueFlag)
    {
      if (RGB.st)
      {
        analogWrite(BlueLight, RGB.b * RGB.br);
      }
      BlueFlag = LOW;
    }
    if (StateFlag)
    {
      if (RGB.st == 1)
      {
        int r = 0;
        int g = 0;
        int b = 0;
        for (int bright = 0; bright < 254; bright++)
        {
          if (r < RGB.r)
          {
            r++;
            analogWrite(RedLight, r * RGB.br);
            delay(2);
          }
          if (g < RGB.g)
          {
            g++;
            analogWrite(GreenLight, g * RGB.br);
            delay(2);
          }
          if (b < RGB.b)
          {
            b++;
            analogWrite(BlueLight, b * RGB.br);
            delay(2);
          }
          delay(20);
        }
        StateFlag = LOW;
      }
      if (RGB.st == 0)
      {
        int r = RGB.r;
        int g = RGB.g;
        int b = RGB.b;
        for (int bright = 255; bright > 0; bright--)
        {
          if (r > 0)
          {
            r--;
            analogWrite(RedLight, r * RGB.br);
            delay(2);
          }
          if (g > 0)
          {
            g--;
            analogWrite(GreenLight, g * RGB.br);
            delay(2);
          }
          if (b > 0)
          {
            b--;
            analogWrite(BlueLight, b * RGB.br);
            delay(2);
          }
          delay(20);
        }
        StateFlag = LOW;
      }
    }
    if (BrightFlag)
    {
      if (RGB.st)
      {
        analogWrite(RedLight, RGB.r * RGB.br);
        analogWrite(GreenLight, RGB.g * RGB.br);
        analogWrite(BlueLight, RGB.b * RGB.br);
      }
      BrightFlag = LOW;
    }
  }
}

void loop()
{
  checkButtonLight();
  Blynk.run();
}