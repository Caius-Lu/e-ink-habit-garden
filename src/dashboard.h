#ifndef _DASHBOARD_H_
#define _DASHBOARD_H_

#include <Arduino.h>
#include <time.h>
#include "display.h"
#include "weather.h"
#include "quotes.h"
#include "habitGarden.h"

#define SCREEN_W 800
#define SCREEN_H 480

#ifdef ESP8266
#define GARDEN_FONT u8g2_font_wqy14_t_gb2312
#else
#define GARDEN_FONT u8g2_font_wqy16_t_gb2312
#endif

time_t lastRenderTime = 0;
String renderQuote;

uint16_t accentColor()
{
  return GxEPD_RED;
}

String utf8Slice(const String &source, int startChar, int count)
{
  int byteStart = 0, chars = 0;
  while (byteStart < (int)source.length() && chars < startChar)
  {
    unsigned char c = source[byteStart];
    byteStart += c < 0x80 ? 1 : ((c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4));
    chars++;
  }
  int byteEnd = byteStart;
  chars = 0;
  while (byteEnd < (int)source.length() && chars < count)
  {
    unsigned char c = source[byteEnd];
    byteEnd += c < 0x80 ? 1 : ((c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4));
    chars++;
  }
  return source.substring(byteStart, min(byteEnd, (int)source.length()));
}

int utf8CharCount(const String &source)
{
  int count = 0;
  for (int i = 0; i < (int)source.length(); count++)
  {
    unsigned char c = source[i];
    i += c < 0x80 ? 1 : ((c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4));
  }
  return count;
}

bool startsWithPunctuation(const String &text)
{
  return text.startsWith("，") || text.startsWith("。") || text.startsWith("！") ||
         text.startsWith("？") || text.startsWith("；") || text.startsWith("：") ||
         text.startsWith("、") || text.startsWith("…");
}

void inkText(int x, int y, const String &s, uint16_t color = GxEPD_BLACK)
{
  u8g2Fonts.setFont(GARDEN_FONT);
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  u8g2Fonts.setForegroundColor(color);
  u8g2Fonts.setCursor(x, y);
  u8g2Fonts.print(s);
  // 1px 叠印形成适合老款三色屏的粗体效果，远看更清楚。
  u8g2Fonts.setCursor(x + 1, y);
  u8g2Fonts.print(s);
}

void drawLeaf(int x, int y, bool left, uint16_t color)
{
  int tip = left ? x - 38 : x + 38;
  display.fillTriangle(x, y + 8, tip, y - 7, tip, y + 20, color);
  display.drawLine(x, y + 8, tip, y + 6, GxEPD_WHITE);
}

void drawFlower(int x, int y)
{
  display.fillCircle(x - 10, y, 10, GxEPD_BLACK);
  display.fillCircle(x + 10, y, 10, GxEPD_BLACK);
  display.fillCircle(x, y - 10, 10, GxEPD_BLACK);
  display.fillCircle(x, y + 10, 10, GxEPD_BLACK);
  display.fillCircle(x, y, 7, GxEPD_BLACK);
}

void drawSunIcon(int x, int y)
{
  const uint16_t sunColor = accentColor();
  // 实心红色太阳：粗光芒适合三色屏，不使用容易产生重影的细圆环。
  display.fillCircle(x, y, 13, sunColor);
  display.fillRect(x - 2, y - 31, 5, 10, sunColor);
  display.fillRect(x - 2, y + 22, 5, 10, sunColor);
  display.fillRect(x - 31, y - 2, 10, 5, sunColor);
  display.fillRect(x + 22, y - 2, 10, 5, sunColor);
  display.drawLine(x - 22, y - 22, x - 16, y - 16, sunColor);
  display.drawLine(x - 21, y - 22, x - 15, y - 16, sunColor);
  display.drawLine(x + 16, y - 16, x + 22, y - 22, sunColor);
  display.drawLine(x + 15, y - 16, x + 21, y - 22, sunColor);
  display.drawLine(x - 22, y + 22, x - 16, y + 16, sunColor);
  display.drawLine(x - 21, y + 22, x - 15, y + 16, sunColor);
  display.drawLine(x + 16, y + 16, x + 22, y + 22, sunColor);
  display.drawLine(x + 15, y + 16, x + 21, y + 22, sunColor);
}

void drawPlant(int stage)
{
  const int cx = 588;
  const int ground = 406;

  // 保持留白，避免装饰图形与名言、天气争夺视觉焦点。
  display.drawFastHLine(410, ground, 355, GxEPD_BLACK);
  display.drawFastHLine(438, ground + 7, 300, GxEPD_BLACK);

  // 花盆
  display.fillRect(cx - 48, ground - 30, 96, 13, accentColor());
  display.drawRect(cx - 42, ground - 17, 84, 55, GxEPD_BLACK);
  display.drawLine(cx - 42, ground - 17, cx - 29, ground + 38, GxEPD_BLACK);
  display.drawLine(cx + 42, ground - 17, cx + 29, ground + 38, GxEPD_BLACK);

  if (stage == 0)
  {
    display.fillCircle(cx, ground - 42, 6, GxEPD_BLACK);
    inkText(530, 225, "种子正在醒来");
    return;
  }

  int top = 350 - stage * 29;
  display.drawLine(cx - 2, ground - 30, cx - 2, top, GxEPD_BLACK);
  display.drawLine(cx + 2, ground - 30, cx + 2, top, GxEPD_BLACK);
  for (int i = 0; i < stage; i++)
  {
    int ly = 350 - i * 28;
    drawLeaf(cx, ly, i % 2 == 0, GxEPD_BLACK);
  }
  if (stage >= 5) drawFlower(cx, top - 8);
  else display.fillCircle(cx, top, 7 + stage, GxEPD_BLACK);
}

void drawGardenPage()
{
  display.fillScreen(GxEPD_WHITE);
  time_t now = time(nullptr);
  struct tm *lt = localtime(&now);
  const char *wd[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
  char date[28];
  strftime(date, sizeof(date), "%Y-%m-%d", lt);
  inkText(20, 30, String(date) + "  " + wd[lt->tm_wday]);
  inkText(20, 55, "墨水盆景 · 今日浇水 " + String(gardenDoneCount()) + "/" + String(HABIT_COUNT));
  if (weather.ok)
  {
    inkText(610, 30, String((int)weather.temp) + " ℃  " + weatherText(weather.code));
    inkText(570, 55, "今天 " + String((int)weather.tmin[0]) + " - " + String((int)weather.tmax[0]) + " ℃");
  }
  display.fillRect(16, 67, 768, 2, GxEPD_BLACK);

  // 左侧日课卡片：删去无意义的纵向分割线和底部重复说明。
  display.drawRect(16, 86, 366, 344, GxEPD_BLACK);
  display.drawRect(18, 88, 362, 340, GxEPD_BLACK);
  inkText(34, 119, "今天，照顾好这三件小事");
  for (int i = 0; i < HABIT_COUNT; i++)
  {
    int y = 176 + i * 76;
    bool done = habitDoneToday(i);
    // 未完成显示红色空心环，完成后填成红色实心点；每天清零后仍保留配色。
    display.drawCircle(50, y - 6, 11, accentColor());
    display.drawCircle(50, y - 6, 9, accentColor());
    if (done)
    {
      display.fillCircle(50, y - 6, 7, accentColor());
    }
    // 文字始终用黑色保证可读性，完成状态用实心圆表达。
    inkText(78, y, garden.names[i], GxEPD_BLACK);
    inkText(225, y, "连续 " + String(garden.progress[i].streak) + " 天");
    if (i < HABIT_COUNT - 1) display.fillRect(34, y + 28, 330, 2, GxEPD_BLACK);
  }

  uint32_t total = gardenTotal();
  int stage = plantStage(total);
  inkText(430, 176, "第 " + String(stage + 1) + " 阶段 · " + String(total) + " 滴水");

  // 名言固定在右上空白区，最多两行，不再与花盆或外壳重叠。
  if (renderQuote.length() > 0)
  {
    const int firstLineChars = 22;
    String first = utf8Slice(renderQuote, 0, firstLineChars);
    String second = utf8Slice(renderQuote, firstLineChars, 22);
    // 禁止句号、逗号等单独落在第二行；尾巴很短时也直接并回第一行。
    while (second.length() > 0 && startsWithPunctuation(second))
    {
      first += utf8Slice(second, 0, 1);
      second = utf8Slice(second, 1, 22);
    }
    if (utf8CharCount(second) <= 4)
    {
      first += second;
      second = "";
    }
    inkText(420, 105, "“" + first + (second.length() == 0 ? "”" : ""));
    if (second.length() > 0) inkText(440, 132, second + "”");
  }
  // 保留太阳填补右侧留白，并与名言、植株保持足够间距。
  drawSunIcon(722, 265);
  drawPlant(stage);
}

void renderDashboard()
{
  Serial.println("render garden");
  initDisplay();
  u8g2Fonts.begin(display);
  display.setRotation(0);
  display.setFullWindow();
  lastRenderTime = time(nullptr);
  renderQuote = randomQuote();
  display.firstPage();
  do { drawGardenPage(); } while (display.nextPage());
  display.powerOff();
  Serial.println("render done");
}

#endif
