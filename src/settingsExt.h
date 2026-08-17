// 仪表盘扩展设置：存 EEPROM 512 偏移之后，独立 magic，不影响原 store.h 的数据
#ifndef _SETTINGS_EXT_H_
#define _SETTINGS_EXT_H_

#include <EEPROM.h>

#define DASH_EEPROM_OFFSET 512
#define DASH_SETTINGS_MAGIC 0xD45C

struct DashSettings
{
  char city[40] = "北京";      // 天气城市名（Open-Meteo geocoding 解析经纬度）
  float lat = 0;
  float lon = 0;
  char githubUser[40] = "";    // GitHub 用户名（留空隐藏该栏）
  char githubToken[64] = "";   // GitHub Token（可选，用于未读通知数）
  char usageLabel[24] = "AI 用量"; // 用量栏标题
  char usageUrl[200] = "";     // 用量接口 URL（返回 JSON，留空隐藏该栏）
  char usageKey[80] = "";      // 用量接口 Bearer Key（可选）
  char usagePath[48] = "percent"; // JSON 数字路径，如 data.percent
  // 不在固件中内置密钥；首次在配网页或 /settings 填写。
  char todoApiKey[48] = ""; // einktodo API Key
  char todoApiUrl[120] = "https://www.einktodo.com/api/display/v2"; // einktodo API URL
};

DashSettings dashSettings;

void saveDashSettings()
{
  EEPROM.begin(1024);
  EEPROM.put(DASH_EEPROM_OFFSET, (int)DASH_SETTINGS_MAGIC);
  EEPROM.put(DASH_EEPROM_OFFSET + sizeof(int), dashSettings);
  EEPROM.commit();
  EEPROM.end();
}

void loadDashSettings()
{
  EEPROM.begin(1024);
  int magic;
  EEPROM.get(DASH_EEPROM_OFFSET, magic);
  if (magic == DASH_SETTINGS_MAGIC)
  {
    EEPROM.get(DASH_EEPROM_OFFSET + sizeof(int), dashSettings);
  }
  EEPROM.end();
}

#endif
