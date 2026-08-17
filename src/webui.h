// 常驻 Web 管理页（连上 WiFi 后手机浏览器访问设备 IP）
// / 状态与立即刷新  /settings 城市/GitHub/用量配置  /quotes 名言编辑
#ifndef _WEBUI_H_
#define _WEBUI_H_

#ifdef ESP8266
#include <ESP8266WebServer.h>
#define WEB_SERVER_CLASS ESP8266WebServer
#else
#include <WebServer.h>
#define WEB_SERVER_CLASS WebServer
#endif
#include "network.h"
#include "todoList.h"
#include "settingsExt.h"
#include "quotes.h"
#include "weather.h"
#include "github.h"
#include "aiUsage.h"
#include "habitGarden.h"
#include <ArduinoJson.h>

extern volatile bool refreshRequested;
extern volatile bool portalRequested;

WEB_SERVER_CLASS server(80);
bool webUiRunning = false;

String htmlEscape(const String &s)
{
  String r = "";
  for (unsigned int i = 0; i < s.length(); i++)
  {
    char c = s[i];
    if (c == '&') r += "&amp;";
    else if (c == '<') r += "&lt;";
    else if (c == '>') r += "&gt;";
    else if (c == '"') r += "&quot;";
    else r += c;
  }
  return r;
}

// 注意：ESP8266 上 PROGMEM 不能被 String()/直接解引用安全读取（会触发非对齐访问异常），
// 网页模板放 RAM（约 5KB），避免崩溃
const char HTML_HEAD[] =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>墨水屏仪表盘</title><style>"
    ":root{--ink:#26352d;--paper:#f2eddf;--leaf:#547257;--sun:#d4774d}"
    "body{font-family:Georgia,'Noto Serif SC','Songti SC',serif;max-width:680px;margin:24px auto;"
    "padding:0 16px;color:var(--ink);background:var(--paper)}"
    "h1{font-size:28px;letter-spacing:2px;margin-bottom:4px}h2{font-size:17px;margin:24px 0 9px}"
    ".sub{color:#748078;margin:0 0 20px}.garden{padding:20px;background:#fffdf5;border:1px solid #c9c2ae;"
    "border-radius:18px;box-shadow:5px 5px 0 #d9d0bc}.habit{display:flex;align-items:center;gap:12px;"
    "padding:12px 0;border-bottom:1px dashed #d8d0bd}.habit:last-child{border:0}.habit b{flex:1}"
    "table{border-collapse:collapse;width:100%;background:#fffdf5;border-radius:8px}"
    "td{padding:8px 10px;border-bottom:1px solid #eee;font-size:14px}"
    "td:first-child{color:#888;width:38%}"
    "input[type=text],input[type=password],textarea{width:100%;box-sizing:border-box;"
    "padding:8px;font-size:14px;border:1px solid #ccc;border-radius:6px;margin:4px 0 12px}"
    "button{background:var(--leaf);color:#fff;border:0;border-radius:999px;padding:10px 20px;font-size:15px}"
    "a.btn{display:inline-block;margin:6px 8px 6px 0;padding:9px 16px;background:#fff;color:#222;"
    "border-radius:6px;text-decoration:none;font-size:14px;border:1px solid #ddd}"
    ".hint{color:#999;font-size:12px;margin:2px 0 10px}"
    ".banner{background:#e8f5e9;border:1px solid #a5d6a7;padding:8px 12px;border-radius:6px;"
    "margin-bottom:12px;font-size:14px}"
    ".done{color:var(--leaf)}.todo{background:var(--sun)}</style></head><body>"
    "<h1>墨水盆景</h1><p class='sub'>把认真生活的日子，养成一株看得见的植物。</p>";
const char HTML_FOOT[] = "</body></html>";

String minutesAgo(unsigned long fetchedAt)
{
  if (fetchedAt == 0)
    return "从未";
  unsigned long s = (millis() - fetchedAt) / 1000;
  if (s < 90)
    return "刚刚";
  return String(s / 60) + " 分钟前";
}

void handleRoot()
{
  String ip = WiFi.localIP().toString();
  unsigned long up = millis() / 1000;
  String page = String(HTML_HEAD);
  page += "<div class='garden'><h2>今日浇水 · " + String(gardenDoneCount()) + "/" + String(HABIT_COUNT) + "</h2>";
  for (int i = 0; i < HABIT_COUNT; i++)
  {
    bool done = habitDoneToday(i);
    page += "<div class='habit'><span>" + String(done ? "●" : "○") + "</span><b>" + htmlEscape(garden.names[i]) + "</b>";
    page += "<span>连续 " + String(garden.progress[i].streak) + " 天</span>";
    if (!done) page += "<form method='post' action='/checkin'><input type='hidden' name='habit' value='" + String(i) + "'><button>浇水</button></form>";
    else page += "<span class='done'>今天已完成</span>";
    page += "</div>";
  }
  page += "</div><h2>设备状态</h2><table>";
  page += "<tr><td>设备地址</td><td>http://" + ip + "</td></tr>";
  page += "<tr><td>WiFi</td><td>" + htmlEscape(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + " dBm)</td></tr>";
  page += "<tr><td>运行时间</td><td>" + String(up / 3600) + " 时 " + String((up % 3600) / 60) + " 分</td></tr>";
  page += "<tr><td>空闲内存</td><td>" + String(ESP.getFreeHeap() / 1024) + " KB</td></tr>";
  page += "<tr><td>天气</td><td>" + (weather.ok ? String("正常 · ") + minutesAgo(weather.fetchedAt) : "未配置或失败") + "</td></tr>";
  page += "<tr><td>成长值</td><td>" + String(gardenTotal()) + " 滴水 · 阶段 " + String(plantStage(gardenTotal()) + 1) + "/6</td></tr>";
  page += "<tr><td>名言</td><td>共 " + String(countQuotes()) + " 条</td></tr>";
  page += "</table><p>";
  page += "<form method='post' action='/refresh'><button>立即刷新屏幕</button></form>";
  page += "<form method='post' action='/portal' style='margin-top:8px'><button style='background:#c0392b'>进入配网模式（换WiFi/填einktodo Key）</button></form>";
  page += "<p><a class='btn' href='/settings'>盆景设置</a><a class='btn' href='/quotes'>名言管理</a>";
  page += "<p class='hint'>屏幕刷新约需 25 秒，刷新期间请勿断电。按钮：单击立即刷新，长按进入 WiFi 配网。</p>";
  page += HTML_FOOT;
  server.send(200, "text/html", page);
}

void handleCheckin()
{
  int index = server.arg("habit").toInt();
  if (gardenCheckIn(index)) refreshRequested = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleApiStatus()
{
  JsonDocument doc;
  doc["name"] = "墨水盆景";
  doc["done"] = gardenDoneCount();
  doc["total"] = gardenTotal();
  doc["stage"] = plantStage(gardenTotal()) + 1;
  doc["ip"] = WiFi.localIP().toString();
  JsonArray habits = doc["habits"].to<JsonArray>();
  for (int i = 0; i < HABIT_COUNT; i++)
  {
    JsonObject h = habits.add<JsonObject>();
    h["index"] = i;
    h["name"] = garden.names[i];
    h["done"] = habitDoneToday(i);
    h["streak"] = garden.progress[i].streak;
    h["total"] = garden.progress[i].total;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json; charset=utf-8", json);
}

void handleApiCheckin()
{
  int index = server.arg("habit").toInt();
  bool changed = gardenCheckIn(index);
  if (changed) refreshRequested = true;
  server.send(changed ? 200 : 409, "application/json", changed ? "{\"ok\":true}" : "{\"ok\":false}");
}

void handleApiQuotes()
{
  if (server.method() == HTTP_POST)
  {
    if (!saveQuotesText(server.arg("plain")))
    {
      server.send(500, "text/plain", "save failed");
      return;
    }
    refreshRequested = true;
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  server.send(200, "text/plain; charset=utf-8", loadQuotesText());
}

void handleRefresh()
{
  refreshRequested = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handlePortal()
{
  portalRequested = true;
  server.sendHeader("Location", "/");
  String page = String(HTML_HEAD);
  page += "<div class='banner'>已请求进入配网模式：请连接热点 E-ink Todo List AP（密码 einktodo.com）</div>";
  page += HTML_FOOT;
  server.send(200, "text/html", page); // 303 后浏览器一般不再展示 body，直接再给一页提示
}

void handleSettings()
{
  String page = String(HTML_HEAD);
  if (server.hasArg("saved"))
    page += "<div class='banner'>已保存</div>";

  if (server.method() == HTTP_POST)
  {
    for (int i = 0; i < HABIT_COUNT; i++)
    {
      String key = "habit" + String(i);
      String value = server.arg(key);
      value.trim();
      if (value.length() > 0) strlcpy(garden.names[i], value.c_str(), sizeof(garden.names[i]));
    }
    saveGarden();
    String city = server.arg("city");
    city.trim();
    String oldCity = String(dashSettings.city);
    strncpy(dashSettings.city, city.c_str(), sizeof(dashSettings.city) - 1);
    dashSettings.city[sizeof(dashSettings.city) - 1] = 0;

    String gu = server.arg("githubUser");
    gu.trim();
    strncpy(dashSettings.githubUser, gu.c_str(), sizeof(dashSettings.githubUser) - 1);
    dashSettings.githubUser[sizeof(dashSettings.githubUser) - 1] = 0;

    String gt = server.arg("githubToken");
    gt.trim();
    if (gt != "(不变)")
      strncpy(dashSettings.githubToken, gt.c_str(), sizeof(dashSettings.githubToken) - 1);
    dashSettings.githubToken[sizeof(dashSettings.githubToken) - 1] = 0;

    String ul = server.arg("usageLabel");
    strncpy(dashSettings.usageLabel, ul.c_str(), sizeof(dashSettings.usageLabel) - 1);
    dashSettings.usageLabel[sizeof(dashSettings.usageLabel) - 1] = 0;

    String uu = server.arg("usageUrl");
    uu.trim();
    strncpy(dashSettings.usageUrl, uu.c_str(), sizeof(dashSettings.usageUrl) - 1);
    dashSettings.usageUrl[sizeof(dashSettings.usageUrl) - 1] = 0;

    String uk = server.arg("usageKey");
    uk.trim();
    if (uk != "(不变)")
      strncpy(dashSettings.usageKey, uk.c_str(), sizeof(dashSettings.usageKey) - 1);
    dashSettings.usageKey[sizeof(dashSettings.usageKey) - 1] = 0;

    String up = server.arg("usagePath");
    up.trim();
    strncpy(dashSettings.usagePath, up.c_str(), sizeof(dashSettings.usagePath) - 1);
    dashSettings.usagePath[sizeof(dashSettings.usagePath) - 1] = 0;

    String tk = server.arg("todoApiKey");
    tk.trim();
    if (tk != "(不变)")
      strncpy(dashSettings.todoApiKey, tk.c_str(), sizeof(dashSettings.todoApiKey) - 1);
    dashSettings.todoApiKey[sizeof(dashSettings.todoApiKey) - 1] = 0;

    String tu = server.arg("todoApiUrl");
    tu.trim();
    strncpy(dashSettings.todoApiUrl, tu.c_str(), sizeof(dashSettings.todoApiUrl) - 1);
    dashSettings.todoApiUrl[sizeof(dashSettings.todoApiUrl) - 1] = 0;

    // 城市变了就重新解析经纬度（失败则下次刷新重试）
    if (city.length() > 0 && city != oldCity)
    {
      dashSettings.lat = 0;
      dashSettings.lon = 0;
      geocodeCity();
    }
    saveDashSettings();
    server.sendHeader("Location", "/settings?saved=1");
    server.send(303);
    return;
  }

  page += "<form method='post' action='/settings'>";
  page += "<h2>我的三件日课</h2><p class='hint'>实体按钮每按一次，会依次完成一项尚未完成的日课。</p>";
  for (int i = 0; i < HABIT_COUNT; i++)
    page += "<input type='text' name='habit" + String(i) + "' value='" + htmlEscape(garden.names[i]) + "' maxlength='8'>";
  page += "<h2>天气城市</h2>";
  page += "<input type='text' name='city' value='" + htmlEscape(dashSettings.city) + "' placeholder='如：上海'>";
  page += "<p class='hint'>Open-Meteo 免费接口，无需 key。中文城市名即可。</p>";
  page += "<h2>GitHub</h2>";
  page += "<p class='hint'>用户名必填；Token 选填（classic token 需要 notifications 权限），填了才显示未读通知。</p>";
  page += "<input type='text' name='githubUser' value='" + htmlEscape(dashSettings.githubUser) + "' placeholder='GitHub 用户名'>";
  page += "<input type='password' name='githubToken' value='(不变)' placeholder='Token（留(不变)表示不修改）'>";
  page += "<h2>AI 用量</h2>";
  page += "<p class='hint'>任何返回 JSON 的接口，按点分路径取数字（0-100 的百分比）。"
          "例如自建脚本返回 {\"percent\": 62.5}，路径填 percent；嵌套填 data.percent。"
          "Bearer Key 选填。留空 URL 则隐藏该栏。</p>";
  page += "<input type='text' name='usageLabel' value='" + htmlEscape(dashSettings.usageLabel) + "' placeholder='显示标题，如 Codex 用量'>";
  page += "<input type='text' name='usageUrl' value='" + htmlEscape(dashSettings.usageUrl) + "' placeholder='https://your.api/usage'>";
  page += "<input type='password' name='usageKey' value='(不变)' placeholder='Bearer Key（选填）'>";
  page += "<input type='text' name='usagePath' value='" + htmlEscape(dashSettings.usagePath) + "' placeholder='percent'>";
  page += "<h2>待办（einktodo）</h2>";
  page += "<p class='hint'>API Key 和 API URL 也可以在这里改，不用进配网。</p>";
  page += "<input type='password' name='todoApiKey' value='(不变)' placeholder='einktodo API Key（首次填写）'>";
  page += "<input type='text' name='todoApiUrl' value='" + htmlEscape(dashSettings.todoApiUrl) + "' placeholder='https://www.einktodo.com/api/display/v2'>";
  page += "<p><button>保存</button> <a class='btn' href='/'>返回</a></p></form>";
  page += HTML_FOOT;
  server.send(200, "text/html", page);
}

void handleQuotes()
{
  String page = String(HTML_HEAD);
  if (server.method() == HTTP_POST)
  {
    saveQuotesText(server.arg("quotes"));
    server.sendHeader("Location", "/quotes?saved=1");
    server.send(303);
    return;
  }
  if (server.hasArg("saved"))
    page += "<div class='banner'>已保存，下次刷新屏幕生效</div>";
  page += "<form method='post' action='/quotes'>";
  page += "<h2>名言库（一行一条，随机轮换显示在屏幕底部）</h2>";
  page += "<textarea name='quotes' rows='12'>";
  page += htmlEscape(loadQuotesText());
  page += "</textarea>";
  page += "<p><button>保存</button> <a class='btn' href='/'>返回</a></p></form>";
  page += HTML_FOOT;
  server.send(200, "text/html", page);
}

void startWebUi()
{
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.on("/quotes", handleQuotes);
  server.on("/refresh", HTTP_POST, handleRefresh);
  server.on("/checkin", HTTP_POST, handleCheckin);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/checkin", HTTP_POST, handleApiCheckin);
  server.on("/api/quotes", HTTP_GET, handleApiQuotes);
  server.on("/api/quotes", HTTP_POST, handleApiQuotes);
  server.on("/portal", HTTP_POST, handlePortal);
  server.onNotFound([]()
                    { server.send(404, "text/html", "<meta charset='utf-8'>404"); });
  server.begin();
  webUiRunning = true;
  Serial.printf("Web UI: http://%s\n", WiFi.localIP().toString().c_str());
}

void stopWebUi()
{
  if (webUiRunning)
  {
    server.stop();
    webUiRunning = false;
    Serial.println("Web UI stopped");
  }
}

// loop 里调用：处理请求，并避免与 WiFiManager 配网门户的 80 端口冲突
void handleWebUi()
{
  if (wifiManager.getConfigPortalActive())
  {
    stopWebUi();
    return;
  }
  if (!webUiRunning && WiFi.status() == WL_CONNECTED)
    startWebUi();
  if (webUiRunning)
    server.handleClient();
}

#endif
