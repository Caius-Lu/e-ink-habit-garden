#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "config.h"

#ifdef ESP8266
// ESP8266 上 LED_PIN=GPIO2 是墨水屏的 RST 控制线（见 display.h），
// 不能当 LED 用，全部置为空操作
void initLED() {}
void turnOnLed() {}
void turnOffLed() {}
void blinkLed(int blinkInterval) {}
#else

unsigned long lastBlinkTime = 0;

void initLED()
{
  pinMode(LED_PIN, OUTPUT);
}

void turnOnLed()
{
  digitalWrite(LED_PIN, HIGH);
}

void turnOffLed()
{
  digitalWrite(LED_PIN, LOW);
}

void blinkLed(int blinkInterval)
{
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval)
  {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // 切换 LED 状态
    lastBlinkTime = currentTime;
  }
}

#endif
#endif