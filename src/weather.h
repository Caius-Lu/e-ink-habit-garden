// 天气模块：Open-Meteo（免费、无需 key）
// 城市名 -> geocoding 接口解析经纬度（设置保存时执行一次，结果存 EEPROM）
#ifndef _WEATHER_H_
#define _WEATHER_H_

#include <ArduinoJson.h>
#include "httpsUtil.h"
#include "settingsExt.h"

String urlencode(String str); // 定义在 todoList.h

struct WeatherData
{
  bool ok = false;
  float temp = 0;    // 当前气温
  float feels = 0;   // 体感温度
  int hum = 0;       // 湿度 %
  int code = 0;      // 当前天气代码
  float tmax[4] = {0}; // 今起 4 天最高/最低温
  float tmin[4] = {0};
  int dcode[4] = {0};
  unsigned long fetchedAt = 0;
};

WeatherData weather;

// 天气代码 -> 中文描述（Open-Meteo WMO code）
const char *weatherText(int code)
{
  if (code == 0) return "晴";
  if (code <= 2) return "多云";
  if (code == 3) return "阴";
  if (code == 45 || code == 48) return "雾";
  if (code >= 51 && code <= 57) return "毛毛雨";
  if (code >= 61 && code <= 67) return "雨";
  if (code >= 71 && code <= 77) return "雪";
  if (code >= 80 && code <= 82) return "阵雨";
  if (code == 85 || code == 86) return "阵雪";
  if (code >= 95) return "雷雨";
  return "";
}

// 图标类型枚举值：0太阳 1多云 2阴 3雾 4小雨 5雨 6雪 7雷
int weatherIconType(int code)
{
  if (code == 0) return 0;
  if (code == 1 || code == 2) return 1;
  if (code == 3) return 2;
  if (code == 45 || code == 48) return 3;
  if ((code >= 51 && code <= 57) || (code >= 80 && code <= 82)) return 4;
  if (code >= 61 && code <= 67) return 5;
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) return 6;
  if (code >= 95) return 7;
  return 2;
}

bool geocodeCity()
{
  String url = "https://geocoding-api.open-meteo.com/v1/search?name=" + urlencode(String(dashSettings.city)) + "&count=1&language=zh&format=json";
  String body;
  if (!fetchHttps(url.c_str(), ISRG_ROOT_X1, nullptr, body, 8192))
    return false;
  JsonDocument doc;
  if (deserializeJson(doc, body))
    return false;
  JsonArrayConst results = doc["results"].as<JsonArrayConst>();
  if (results.isNull() || results.size() == 0)
  {
    Serial.printf("geocode 未找到城市: %s\n", dashSettings.city);
    return false;
  }
  dashSettings.lat = results[0]["latitude"].as<float>();
  dashSettings.lon = results[0]["longitude"].as<float>();
  saveDashSettings();
  Serial.printf("geocode %s -> (%.4f, %.4f)\n", dashSettings.city, dashSettings.lat, dashSettings.lon);
  return true;
}

// 拉取天气；返回 true 表示数据有变化（需要刷屏）
bool fetchWeather()
{
  if (strlen(dashSettings.city) == 0)
    return false;
  if ((dashSettings.lat == 0 && dashSettings.lon == 0) && !geocodeCity())
    return false;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(dashSettings.lat, 4) + "&longitude=" + String(dashSettings.lon, 4) +
               "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code"
               "&daily=weather_code,temperature_2m_max,temperature_2m_min&forecast_days=4&timezone=auto";
  String body;
  if (!fetchHttps(url.c_str(), ISRG_ROOT_X1, nullptr, body, 16384))
  {
    weather.ok = false;
    weather.fetchedAt = millis();
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, body))
    return false;

  WeatherData w;
  w.temp = doc["current"]["temperature_2m"].as<float>();
  w.hum = doc["current"]["relative_humidity_2m"].as<int>();
  w.feels = doc["current"]["apparent_temperature"].as<float>();
  w.code = doc["current"]["weather_code"].as<int>();

  JsonArrayConst dmax = doc["daily"]["temperature_2m_max"].as<JsonArrayConst>();
  JsonArrayConst dmin = doc["daily"]["temperature_2m_min"].as<JsonArrayConst>();
  JsonArrayConst dcode = doc["daily"]["weather_code"].as<JsonArrayConst>();
  for (int i = 0; i < 4; i++)
  {
    if (!dmax.isNull() && i < (int)dmax.size()) w.tmax[i] = dmax[i].as<float>();
    if (!dmin.isNull() && i < (int)dmin.size()) w.tmin[i] = dmin[i].as<float>();
    if (!dcode.isNull() && i < (int)dcode.size()) w.dcode[i] = dcode[i].as<int>();
  }
  w.ok = true;
  w.fetchedAt = millis();

  bool changed = !weather.ok;
  if (!changed)
  {
    changed = (int)(weather.temp) != (int)(w.temp) || weather.code != w.code ||
              weather.hum != w.hum || (int)(weather.feels) != (int)(w.feels);
    for (int i = 0; i < 4 && !changed; i++)
      changed = (int)weather.tmax[i] != (int)w.tmax[i] || (int)weather.tmin[i] != (int)w.tmin[i] || weather.dcode[i] != w.dcode[i];
  }
  weather = w;
  Serial.printf("weather %.1fC code=%d -> changed=%d\n", weather.temp, weather.code, changed);
  return changed;
}

#endif
