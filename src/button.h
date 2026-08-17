#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <OneButton.h>
#include "network.h"
#ifdef ESP8266
#include "gpio16.h" // GPIO16 属 RTC 域，需特殊配置（官方固件同款方案）
#endif

OneButton button(BUTTON_PIN, true);

#ifdef DASHBOARD
volatile bool refreshRequested = false;
#endif

// 配网请求：按键/网页/串口都能置位，由主循环安全地启动配网门户
volatile bool portalRequested = false;

void buttonClick()
{
  Serial.println("Button clicked");
#ifdef DASHBOARD
  int checked = gardenCheckInNext();
  if (checked >= 0)
    Serial.printf("Garden check-in: %s\n", garden.names[checked]);
  else
    Serial.println("Garden: all habits completed today");
  refreshRequested = true;
#else
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("force update todo");
    strcpy(runningValue.todoLastModified, "");
    saveRunningValue(runningValue);
    updating = true;
    downloadAndDrawTodo();
    updating = false;
  }
  else
  {
    Serial.println("WiFi not connected");
  }
#endif
}

void buttonDoubleClick()
{
  Serial.println("Button double clicked");
#ifdef DASHBOARD
  refreshRequested = true; // 双击也触发刷新，怎么按都有反馈
#endif
}

void buttonLongPress()
{
  Serial.println("Button long pressed -> request config portal");
  portalRequested = true;
}

void initButton()
{
  Serial.println("Initializing button...");
#ifdef ESP8266
  if (BUTTON_PIN == 16)
  {
    // GPIO16 无内部上拉，按官方固件方案：配置为输出并置高，按键按下拉到 GND
    Serial.println("GPIO16 特殊配置：输出高电平驱动");
    gpio16_output_conf();
    gpio16_output_set(1);
  }
  else
  {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
  }
#else
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
  delay(5);
  int idle = digitalRead(BUTTON_PIN);
  Serial.printf("Button on GPIO%d, idle level=%d (按下应为0)\n", BUTTON_PIN, idle);
  button.attachClick(buttonClick);
  button.attachDoubleClick(buttonDoubleClick);
  button.attachLongPressStart(buttonLongPress);
}

#endif
