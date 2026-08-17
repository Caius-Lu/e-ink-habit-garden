#ifndef _TODO_LIST_H_
#define _TODO_LIST_H_

#include <WiFiClientSecure.h>
#include <algorithm>

#ifdef ESP32
#include <HTTPClient.h>
#endif

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#endif

#include <GxEPD2_BW.h>
#include "display.h"
#include "store.h"
#include "config.h"
#include "clock.h"
#include "network.h"
#include <LittleFS.h>
#ifdef DASHBOARD
#include "settingsExt.h" // 仪表盘模式：待办的 Key/URL 存自己的设置结构
#include "certs.h"
#endif

WiFiClientSecure client;
HTTPClient https;

int updating = false;
String cachedFileName = "/todo.bitmap";

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  // char code2;
  for (unsigned int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
        code0 = c - 10 + 'A';
      }
      // code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      // encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}

void downloadAndDrawTodo(String user = "eson", uint16_t color = GxEPD_BLACK)
{
  const char *apiRoot = apiUrl.getValue();
  const char *apikey = apiKey.getValue();

#ifdef ESP32
  client.setCACert(GTS_ROOT_R4); // einktodo 现用 WE1 中间证书，链到 GTS Root R4
#endif

#ifdef ESP8266
  // ESP8266 堆紧张：BearSSL 默认缓冲要 ~25KB 会内存不足，缩小到 ~5KB
  client.setBufferSizes(4096, 1024);
  X509List cert(GTS_ROOT_R4);
  client.setTrustAnchors(&cert);
#endif

  String savedTodoLastModified = runningValue.todoLastModified;

  Serial.println("Last-Modified: " + savedTodoLastModified);

  Serial.printf("download and draw %s todo\n", user.c_str());

  String url = String(apiRoot) + "?width=" + String(display.width()) + "&height=" + String(display.height());

  Serial.printf("GET %s\n", url.c_str());
  Serial.printf("If-Modified-Since: %s\n", savedTodoLastModified.c_str());
  Serial.printf("Authorization: Bearer [configured, length=%u]\n", strlen(apikey));

  https.begin(client, url);
  https.addHeader("If-Modified-Since", savedTodoLastModified);
  https.addHeader("Authorization", "Bearer " + String(apikey));

  const char *headerKeys[] = {"Content-Picture-Width", "Content-Picture-Height", "API-Version", "Last-Modified"};
  https.collectHeaders(headerKeys, 4);

  int httpCode = https.GET();
  int contentLength = https.getSize();

  Serial.printf("HTTPS GET: %d\n", httpCode);
  Serial.printf("Content-Length: %d\n", contentLength);
  if (httpCode == HTTP_CODE_NOT_MODIFIED)
  {
    Serial.println("Not Modified");
    return;
  }
  if (httpCode == HTTP_CODE_NO_CONTENT)
  {
    Serial.println("No Content");
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(0, 0);
    display.print("No Content");
    display.display();
    return;
  }
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.printf("HTTPS GET failed, error: %s\n", https.errorToString(httpCode).c_str());
    return;
  }

  if (contentLength <= 0)
  {
    Serial.println("Content-Length not set");
    return;
  }

  Serial.printf("headers count %d\n", https.headers());
  for (int i = 0; i < https.headers(); i++)
  {
    String headerName = https.headerName(i);
    String headerValue = https.header(i);
    Serial.printf("header[%s]: %s\n", headerName.c_str(), headerValue.c_str());
  }

  WiFiClient *stream = https.getStreamPtr();
  int16_t w = https.header("Content-Picture-Width").toInt();
  int16_t h = https.header("Content-Picture-Height").toInt();

  String lastModified = https.header("Last-Modified");
  Serial.printf("Last-Modified: %s\n", lastModified.c_str());

  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  File file = LittleFS.open(cachedFileName, "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  Serial.println("Start reading response body");
  int bitsDown = 0;
  int bitsTotal = w * h;
  int bytesRead = 0;

  uint8_t buf[128];
  while (https.connected() && (bytesRead = stream->readBytes(buf, std::min(128, bitsTotal - bitsDown))) > 0)
  {
    file.write(buf, bytesRead);
    bitsDown += bytesRead;
    // Serial.printf("Read %d bytes, %d/%d\n", bytesRead, bitsDown, bitsTotal);
  }
  file.close();

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println("Read all response body");
  https.end();

  strcpy(runningValue.todoLastModified, lastModified.c_str());
  saveRunningValue(runningValue);

  initDisplay();
  display.fillScreen(GxEPD_WHITE);

#ifdef E_INK_750
  File readFile = LittleFS.open(cachedFileName, "r");
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  do
  {
    readFile.seek(0);
    uint16_t x = 0;
    uint16_t y = 0;
    while (readFile.available())
    {
      readFile.read(buf, sizeof(buf));
      // draw pixel from buf, 8 pixels in a byte, width is w, height is h
      for (size_t i = 0; i < sizeof(buf); i++)
      {
        for (int j = 0; j < 8; j++)
        {
          if (x >= w)
          {
            x = 0;
            y++;
          }
          if (y >= h)
          {
            break;
          }
          if (buf[i] & (0x80 >> j))
          {
            display.drawPixel(x, y, color);
          }
          x++;
        }
      }
    }
    drawCurrentTime();
  } while (display.nextPage());
  readFile.close();

  // do
  // {

  //   display.drawBitmap(0, 0, bitmap, w, h, color);
  //   drawCurrentTime();
  // } while (display.nextPage());
  // } while (display.nextPageBW());
#else
  display.drawBitmap(0, 0, buf, w, h, color);
  display.display();
#endif

  display.powerOff();

  Serial.println("draw bitmap done");
}

// ===== 以下为仪表盘模式新增：半屏拉取 + 区域绘制（原 downloadAndDrawTodo 逻辑不变）=====
uint16_t todoBmpW = 0, todoBmpH = 0;

// 缓存位图没有独立的尺寸元数据；仪表盘请求固定尺寸，启动时即可恢复它。
void restoreTodoBitmapCache(uint16_t w, uint16_t h)
{
  todoBmpW = w;
  todoBmpH = h;
}

// 按指定尺寸向 einktodo 请求位图并缓存；返回 true 表示有新内容（需要刷屏）
bool fetchTodoBitmap(uint16_t w, uint16_t h)
{
#ifdef DASHBOARD
  const char *apiRoot = dashSettings.todoApiUrl;
  const char *apikey = dashSettings.todoApiKey;
#else
  const char *apiRoot = apiUrl.getValue();
  const char *apikey = apiKey.getValue();
#endif
  if (strlen(apiRoot) == 0 || strlen(apikey) == 0)
  {
    Serial.println("todo: API Key/URL 未配置（在设备网页 /settings 填写）");
    return false;
  }

  String savedTodoLastModified = runningValue.todoLastModified;
  String url = String(apiRoot) + "?width=" + String(w) + "&height=" + String(h);

  int httpCode = -1;
  int contentLength = -1;

  // 最多重试 2 次：第一次带 R4 根证书验证，失败后降级 insecure
  for (int attempt = 0; attempt < 2; attempt++)
  {
    WiFiClientSecure wc;
#ifdef ESP8266
    wc.setBufferSizes(4096, 1024);
    if (attempt == 0)
    {
      X509List ca(GTS_ROOT_R4);
      wc.setTrustAnchors(&ca);
    }
    else
    {
      wc.setInsecure();
    }
#else
    if (attempt == 0)
      wc.setCACert(GTS_ROOT_R4);
    else
      wc.setInsecure();
#endif

    HTTPClient http;
    if (!http.begin(wc, url))
    {
      Serial.printf("[todo] attempt %d: begin failed\n", attempt);
      continue;
    }
    http.addHeader("If-Modified-Since", savedTodoLastModified);
    http.addHeader("Authorization", "Bearer " + String(apikey));
    http.setUserAgent("EInkDashboard/1.0");

    const char *headerKeys[] = {"Content-Picture-Width", "Content-Picture-Height", "Last-Modified"};
    http.collectHeaders(headerKeys, 3);

    httpCode = http.GET();
    contentLength = http.getSize();
    Serial.printf("[todo] attempt %d: HTTP %d CL:%d\n", attempt, httpCode, contentLength);

    if (httpCode == HTTP_CODE_NOT_MODIFIED)
    {
      http.end();
      Serial.println("todo: Not Modified");
      return false;
    }

    if (httpCode == HTTP_CODE_OK && contentLength > 0)
    {
      uint16_t pw = http.header("Content-Picture-Width").toInt();
      uint16_t ph = http.header("Content-Picture-Height").toInt();
      todoBmpW = pw > 0 ? pw : w;
      todoBmpH = ph > 0 ? ph : h;
      String lastModified = http.header("Last-Modified");

      if (!LittleFS.begin())
      {
        Serial.println("todo: LittleFS mount failed");
        http.end();
        return false;
      }
      File file = LittleFS.open(cachedFileName, "w");
      if (!file)
      {
        http.end();
        return false;
      }

      WiFiClient *stream = http.getStreamPtr();
      int bytesTotal = (todoBmpW * todoBmpH + 7) / 8;
      int bytesDown = 0;
      uint8_t buf[128];
      while (http.connected() && bytesDown < bytesTotal)
      {
        int toRead = min(128, bytesTotal - bytesDown);
        int n = stream->readBytes(buf, toRead);
        if (n <= 0)
          break;
        file.write(buf, n);
        bytesDown += n;
      }
      file.close();
      http.end();

      if (bytesDown != bytesTotal)
      {
        LittleFS.remove(cachedFileName);
        Serial.printf("todo: incomplete bitmap %d/%d bytes\n", bytesDown, bytesTotal);
        return false;
      }

      strcpy(runningValue.todoLastModified, lastModified.c_str());
      saveRunningValue(runningValue);

      Serial.printf("todo bitmap %dx%d downloaded\n", todoBmpW, todoBmpH);
      return true;
    }

    http.end();
  }

  Serial.printf("[todo] all attempts failed, last HTTP %d\n", httpCode);
  return false;
}

// 把缓存位图画到 (ox, oy)（须在 display.nextPage() 分页循环内调用）
void drawTodoBitmapRegion(int16_t ox, int16_t oy)
{
  if (todoBmpW == 0 || todoBmpH == 0)
    return;
  if (!LittleFS.begin())
    return;
  File readFile = LittleFS.open(cachedFileName, "r");
  if (!readFile)
    return;

  uint16_t w = todoBmpW, h = todoBmpH;
  uint16_t x = 0, y = 0;
  uint8_t buf[128];
  while (readFile.available())
  {
    int n = readFile.read(buf, sizeof(buf));
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        if (x >= w)
        {
          x = 0;
          y++;
        }
        if (y >= h)
        {
          readFile.close();
          return;
        }
        if (buf[i] & (0x80 >> j))
          display.drawPixel(ox + x, oy + y, GxEPD_BLACK);
        x++;
      }
    }
  }
  readFile.close();
}

bool todoBitmapCached()
{
  if (!LittleFS.begin())
    return false;
  return LittleFS.exists(cachedFileName) && todoBmpW > 0;
}
#endif // _TODO_LIST_H_
