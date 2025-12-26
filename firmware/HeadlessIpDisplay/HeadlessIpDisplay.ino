/*
 * HeadlessIpDisplay - IP Display for Headless Raspberry Pi Servers
 * 
 * Модифікована версія для роботи БЕЗ графічної сесії.
 * Плата слухає серійний порт і відображає дані, які надсилає Pi.
 * 
 * Формат даних від Pi:
 *   HOST:hostname
 *   WIFI:192.168.1.100/24
 *   ETH:10.0.0.5/24
 *   OTHER:172.16.0.1/24
 * 
 * Для SpotPear/Waveshare RP2350 + 1.47" LCD
 */

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in47.h"

// TinyUSB вже включається автоматично при виборі USB Stack
// Просто використовуємо Serial як CDC

// Налаштування кольорів
#define DISPLAY_BG_COLOR    BLACK
#define DISPLAY_TEXT_COLOR  WHITE
#define DISPLAY_LABEL_COLOR GRAY

// Розміри екрану
#define LCD_WIDTH  172
#define LCD_HEIGHT 320

// Буфер для екрану
uint16_t* framebuffer = nullptr;

// Дані для відображення
String hostname = "Waiting...";
String wifiIp = "";
String ethIp = "";
String otherIp1 = "";
String otherIp2 = "";
unsigned long lastUpdate = 0;
bool dataReceived = false;

// Серійний буфер
String serialBuffer = "";

// Функція для видалення CIDR маски (/24) з IP
String stripCidr(String ip) {
  int slashPos = ip.indexOf('/');
  if (slashPos > 0) {
    return ip.substring(0, slashPos);
  }
  return ip;
}

void setup() {
  // Ініціалізація дисплея ПЕРЕД USB
  DEV_Module_Init();
  LCD_1IN47_Init(HORIZONTAL);
  LCD_1IN47_Clear(DISPLAY_BG_COLOR);
  
  // Виділення пам'яті для framebuffer
  framebuffer = (uint16_t*)malloc(LCD_WIDTH * LCD_HEIGHT * 2);
  if (framebuffer) {
    Paint_NewImage((uint8_t*)framebuffer, LCD_WIDTH, LCD_HEIGHT, 0, DISPLAY_BG_COLOR);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(DISPLAY_BG_COLOR);
  }
  
  // Показати початковий екран
  drawDisplay();
  
  // Ініціалізація USB CDC (Serial)
  Serial.begin(115200);
  
  // Чекаємо до 3 секунд на USB підключення (не блокуємо якщо немає)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) {
    delay(10);
  }
}

void loop() {
  // Читання даних з серійного порту
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processLine(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;
    }
  }
  
  // Оновлення статусу кожну секунду
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate >= 1000) {
    lastStatusUpdate = millis();
    drawDisplay();
  }
}

void processLine(String line) {
  line.trim();
  if (line.length() == 0) return;
  
  int colonPos = line.indexOf(':');
  if (colonPos == -1) return;
  
  String key = line.substring(0, colonPos);
  String value = line.substring(colonPos + 1);
  value.trim();
  
  key.toUpperCase();
  
  if (key == "HOST" || key == "HOSTNAME") {
    hostname = value;
    dataReceived = true;
    lastUpdate = millis();
  }
  else if (key == "WIFI" || key == "WLAN") {
    wifiIp = value;
    dataReceived = true;
    lastUpdate = millis();
  }
  else if (key == "ETH" || key == "ETHERNET") {
    ethIp = value;
    dataReceived = true;
    lastUpdate = millis();
  }
  else if (key == "OTHER1" || key == "OTHER") {
    otherIp1 = value;
    dataReceived = true;
    lastUpdate = millis();
  }
  else if (key == "OTHER2") {
    otherIp2 = value;
    dataReceived = true;
    lastUpdate = millis();
  }
  else if (key == "CLEAR") {
    // Очистити всі дані
    hostname = "Waiting...";
    wifiIp = "";
    ethIp = "";
    otherIp1 = "";
    otherIp2 = "";
    dataReceived = false;
  }
  
  // Оновити дисплей після отримання даних
  drawDisplay();
}

void drawDisplay() {
  if (!framebuffer) return;
  
  Paint_Clear(DISPLAY_BG_COLOR);
  
  int y = 8;
  int lineHeight = 38;
  
  // Hostname
  Paint_DrawString_EN(5, y, "HOST:", &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
  y += 18;
  Paint_DrawString_EN(5, y, hostname.c_str(), &Font20, DISPLAY_BG_COLOR, DISPLAY_TEXT_COLOR);
  y += lineHeight;
  
  // WiFi - мітка та IP на одному рядку
  Paint_DrawString_EN(5, y, "WIFI:", &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
  y += 18;
  if (wifiIp.length() > 0) {
    Paint_DrawString_EN(5, y, stripCidr(wifiIp).c_str(), &Font16, DISPLAY_BG_COLOR, GREEN);
  } else {
    Paint_DrawString_EN(5, y, "---", &Font16, DISPLAY_BG_COLOR, GRAY);
  }
  y += lineHeight;
  
  // Ethernet
  Paint_DrawString_EN(5, y, "ETH:", &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
  y += 18;
  if (ethIp.length() > 0) {
    Paint_DrawString_EN(5, y, stripCidr(ethIp).c_str(), &Font16, DISPLAY_BG_COLOR, CYAN);
  } else {
    Paint_DrawString_EN(5, y, "---", &Font16, DISPLAY_BG_COLOR, GRAY);
  }
  y += lineHeight;
  
  // Other interfaces
  if (otherIp1.length() > 0) {
    Paint_DrawString_EN(5, y, "OTHER:", &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
    y += 18;
    Paint_DrawString_EN(5, y, stripCidr(otherIp1).c_str(), &Font16, DISPLAY_BG_COLOR, YELLOW);
    y += lineHeight;
  }
  
  if (otherIp2.length() > 0) {
    Paint_DrawString_EN(5, y, "OTHER:", &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
    y += 18;
    Paint_DrawString_EN(5, y, stripCidr(otherIp2).c_str(), &Font16, DISPLAY_BG_COLOR, YELLOW);
    y += lineHeight;
  }
  
  // Статус
  y = LCD_HEIGHT - 25;
  if (dataReceived) {
    unsigned long seconds = (millis() - lastUpdate) / 1000;
    char status[32];
    snprintf(status, sizeof(status), "Updated %lus ago", seconds);
    Paint_DrawString_EN(5, y, status, &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
  } else {
    // Анімація очікування
    static int dots = 0;
    dots = (dots + 1) % 4;
    String waiting = "Waiting";
    for (int i = 0; i < dots; i++) waiting += ".";
    Paint_DrawString_EN(5, y, waiting.c_str(), &Font16, DISPLAY_BG_COLOR, DISPLAY_LABEL_COLOR);
  }
  
  // Відправити на дисплей
  LCD_1IN47_Display(framebuffer);
}
