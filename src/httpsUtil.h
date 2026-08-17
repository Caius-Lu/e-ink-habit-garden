// 通用 HTTPS GET 工具：带根证书校验，失败时降级为不校验重试一次（hobby 取舍：
// 保证家里路由劫持/DNS 污染以外的大部分场景可用；einktodo 原链路仍是证书校验）
#ifndef _HTTPS_UTIL_H_
#define _HTTPS_UTIL_H_

#include <Arduino.h>
#include <WiFiClientSecure.h>
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif
#include "certs.h"

bool fetchHttps(const char *url, const char *rootCA, const char *bearer, String &out, uint32_t maxLen = 61440)
{
  out = "";
  bool ok = false;
#ifdef ESP8266
  // ESP8266 内存紧张：缩小 BearSSL 缓冲 + 限制响应体大小
  if (maxLen > 16384)
    maxLen = 16384;
  // ESP8266 的 BearSSL 用 X509List 装根证书（需在连接期间保持存活）
  X509List *ca = (rootCA != nullptr) ? new X509List(rootCA) : nullptr;
#endif

  for (int attempt = 0; attempt < 2 && !ok; attempt++)
  {
    WiFiClientSecure wc;
#ifdef ESP8266
    wc.setBufferSizes(4096, 1024);
    if (attempt == 0 && ca != nullptr)
      wc.setTrustAnchors(ca);
    else
      wc.setInsecure();
#else
    if (attempt == 0 && rootCA != nullptr)
      wc.setCACert(rootCA);
    else
      wc.setInsecure();
#endif

    HTTPClient http;
    if (!http.begin(wc, url))
      continue;
    if (bearer != nullptr && strlen(bearer) > 0)
      http.addHeader("Authorization", String("Bearer ") + bearer);
    http.setUserAgent("EInkDashboard/1.0");
#ifndef ESP8266
    http.setConnectTimeout(8000);
#endif
    http.setTimeout(12000);

    int code = http.GET();
    if (code == HTTP_CODE_OK)
    {
      String body = http.getString();
      if (body.length() > 0 && body.length() < maxLen)
      {
        out = body;
        ok = true;
      }
    }
    else
    {
      Serial.printf("fetchHttps %s attempt %d -> HTTP %d\n", url, attempt, code);
    }
    http.end();
  }

#ifdef ESP8266
  delete ca;
#endif
  return ok;
}

#endif
