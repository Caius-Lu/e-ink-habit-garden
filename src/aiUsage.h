// AI 用量模块：通用接口，返回 JSON，按点分路径取数字，按百分比画进度条
// 用途示例：
//   - OpenAI 平台组织用量（需 Admin Key，GET /v1/organization/usage，自行换算百分比）
//   - 自建脚本/代理返回 {"percent": 62.5}
//   - Codex/ChatGPT 消费版用量官方无公开 API，可让自建脚本轮询后输出 JSON
#ifndef _AI_USAGE_H_
#define _AI_USAGE_H_

#include <ArduinoJson.h>
#include "httpsUtil.h"
#include "settingsExt.h"

struct UsageData
{
  bool ok = false;
  float value = 0; // 百分比
  unsigned long fetchedAt = 0;
};

UsageData usage;

bool usageConfigured()
{
  return strlen(dashSettings.usageUrl) > 0;
}

// 按点分路径（a.b.c）从 JSON 取数值
static bool jsonPathToFloat(JsonDocument &doc, const char *path, float &out)
{
  char buf[48];
  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  JsonVariantConst v = doc.as<JsonVariantConst>();
  char *saveptr = nullptr;
  for (char *seg = strtok_r(buf, ".", &saveptr); seg != nullptr; seg = strtok_r(nullptr, ".", &saveptr))
  {
    v = v[seg];
    if (v.isNull())
      return false;
  }
  if (!v.is<float>() && !v.is<int>() && !v.is<long long>())
    return false;
  out = v.as<float>();
  return true;
}

// 返回 true 表示有变化（需要刷屏）
bool fetchUsage()
{
  if (!usageConfigured())
    return false;

  String body;
  const char *key = strlen(dashSettings.usageKey) > 0 ? dashSettings.usageKey : nullptr;
  // 用户自定义接口域名不定，不做根证书校验
  if (!fetchHttps(dashSettings.usageUrl, nullptr, key, body, 16384))
  {
    usage.ok = false;
    usage.fetchedAt = millis();
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, body))
    return false;

  float val = 0;
  if (!jsonPathToFloat(doc, dashSettings.usagePath, val))
    return false;

  bool changed = !usage.ok || (int)(usage.value * 10) != (int)(val * 10);
  usage.ok = true;
  usage.value = val;
  usage.fetchedAt = millis();
  Serial.printf("usage %s = %.1f -> changed=%d\n", dashSettings.usageLabel, usage.value, changed);
  return changed;
}

#endif
