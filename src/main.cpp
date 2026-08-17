#ifdef DASHBOARD

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "store.h"
#include "clock.h"
#include "display.h"
#include "network.h"
#include "todoList.h"
#include "led.h"
#include "habitGarden.h"
#include "button.h"
#include "settingsExt.h"
#include "quotes.h"
#include "weather.h"
#include "github.h"
#include "aiUsage.h"
#include "dashboard.h"
#include "webui.h"

#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif

// 各数据源拉取间隔（毫秒）
const unsigned long WEATHER_INTERVAL = 30UL * 60 * 1000;
unsigned long lastWeatherFetch = 0;

// ---- GPIO 扫描：串口发 scan 后开启 60 秒，用于定位按键接线 ----
uint32_t lastGpioScan = 0;
uint32_t gpioScanUntil = 0;
uint32_t lastGpioReport = 0;
int lastGpioState[17];
void initGpioScan()
{
  for (int i = 0; i < 17; i++)
    lastGpioState[i] = -2;
}
void gpioScanTask()
{
  if (gpioScanUntil == 0 || millis() > gpioScanUntil)
    return;
  if (millis() - lastGpioScan < 250)
    return;
  lastGpioScan = millis();
  int pins[9] = {0, 2, 4, 5, 12, 13, 14, 15, 16}; // 只读不改配置，不影响屏幕
  for (int i = 0; i < 9; i++)
  {
    int p = pins[i];
    int v = digitalRead(p);
    if (lastGpioState[p] != -2 && v != lastGpioState[p])
      Serial.printf("[pins] GPIO%d: %d -> %d\n", p, lastGpioState[p], v);
    lastGpioState[p] = v;
  }
  if (millis() - lastGpioReport >= 3000)
  {
    lastGpioReport = millis();
    Serial.printf("[pins] state:");
    for (int i = 0; i < 9; i++)
      Serial.printf(" %d=%d", pins[i], lastGpioState[pins[i]]);
    Serial.println();
  }
}

// ---- 串口命令：fr=刷新 portal=配网 scan=扫引脚 reboot=重启 ----
void handleSerialCommand()
{
  if (!Serial.available())
    return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0)
    return;
  if (cmd == "fr" || cmd == "refresh")
  {
    Serial.println("命令: 强制刷新");
    refreshRequested = true;
  }
  else if (cmd == "portal" || cmd == "config")
  {
    Serial.println("命令: 进入配网");
    portalRequested = true;
  }
  else if (cmd == "scan")
  {
    Serial.println("命令: GPIO 扫描 60 秒，请按按键...");
    gpioScanUntil = millis() + 60000;
  }
  else if (cmd == "reboot")
  {
    Serial.println("命令: 重启");
    ESP.restart();
  }
  else
  {
    Serial.printf("未知命令: %s（可用: fr / portal / scan / reboot）\n", cmd.c_str());
  }
}

// 全量刷新：force=true 忽略间隔且必定刷屏；否则只拉到期的，有变化才刷屏
void refreshAll(bool force)
{
  if (updating)
    return;
  updating = true;
  turnOnLed();

  bool changed = force;
  if (force || millis() - lastWeatherFetch >= WEATHER_INTERVAL)
  {
    Serial.println("[fetch] weather...");
    if (fetchWeather())
      changed = true;
    lastWeatherFetch = millis();
    Serial.println("[fetch] weather done");
  }
  if (changed)
    renderDashboard();

  turnOffLed();
  updating = false;
}

// 心跳：每 5 秒打印一次运行状态（卡死定位用）
void heartbeat()
{
  static unsigned long lastHb = 0;
  if (millis() - lastHb < 5000)
    return;
  lastHb = millis();
  Serial.printf("[hb] %lus heap=%u upd=%d wifi=%d\n",
                millis() / 1000, ESP.getFreeHeap(), updating ? 1 : 0, WiFi.status());
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Dashboard Setup Start");

  initStore();
  initButton();
  initLED();
  initGpioScan();
  loadDashSettings();

  if (!LittleFS.begin())
  {
    Serial.println("LittleFS mount failed, formatting...");
    LittleFS.format();
    LittleFS.begin();
  }
  loadGarden();

  if (initWifiWithManager())
  {
    setClock();
    startWebUi();
    if (MDNS.begin("einktodo"))
      Serial.println("mDNS: http://einktodo.local");
    refreshAll(true);
  }
}

void loop()
{
  heartbeat();
  button.tick();
  wifiManager.process();
  gpioScanTask();
  handleSerialCommand();
#ifdef ESP8266
  MDNS.update(); // ESP8266 需要手动喂养 mDNS
#endif

  if (refreshRequested)
  {
    refreshRequested = false;
    refreshAll(true);
  }

  // 配网门户：按键长按 / 网页 / 串口都能触发，在主循环里安全启动
  if (portalRequested && !updating && !wifiManager.getConfigPortalActive())
  {
    portalRequested = false;
    stopWebUi(); // 必须先释放 80 端口，否则门户的网页服务绑定失败（连上热点打不开192.168.4.1）
    Serial.println("启动配网门户");
    wifiManager.startConfigPortal(AP_SSID.c_str(), AP_PASSWORD.c_str());
  }

  // 到期自动拉取（数据没变化不刷屏，保护墨水屏）
  if (!updating && WiFi.status() == WL_CONNECTED)
  {
    if (millis() - lastWeatherFetch >= WEATHER_INTERVAL)
    {
      refreshAll(false);
    }
  }

  handleWebUi();
  delay(10);
}

#else // ===== 原版逻辑（esp32_classic / esp8266 环境）=====

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "todoList.h"
#include "config.h"
#include "network.h"
#include "led.h"
#include "display.h"
#include "clock.h"
#include "button.h"

void setup()
{

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Steup Start");

  // if File System is not formatted, format it
  // if (!LittleFS.begin())
  // {
  //   Serial.println("An Error has occurred while mounting LittleFS, formatting...");
  //   LittleFS.format();
  //   if (!LittleFS.begin())
  //   {
  //     Serial.println("Formatting LittleFS failed!");
  //     return;
  //   }
  // }

  initStore();
  initButton();

  if (initWifiWithManager())
  {

    setClock();

    delay(1000);

    updating = true;
    downloadAndDrawTodo();
    updating = false;
  }
}

void loop()
{
  button.tick();
  wifiManager.process();

  // 每 1 分钟检查一次是否需要更新 todo
  time_t now = time(nullptr);
  if (now % 60 == 0 && !updating && WiFi.status() == WL_CONNECTED)
  {
    updating = true;
    downloadAndDrawTodo();
    delay(800);
    updating = false;
  }

  // wait input from Serial
  // String input = Serial.readStringUntil('\n');
  // if (input.length() > 0)
  // {
  //   Serial.print("input: ");
  //   Serial.println(input);
  //   char *inputChar = (char *)malloc(input.length() + 1);

  //   用 : 分割字符串
  //   input.toCharArray(inputChar, input.length() + 1);
  //   char *command = strtok(inputChar, ":");
  //   Serial.print("command: ");
  //   Serial.println(command);

  //   if (strcmp(command, "fr") == 0)
  //   {
  //     Serial.println("fr:强制刷新 todo");
  //     strcpy(runningValue.todoLastModified, "");
  //     saveRunningValue(runningValue);
  //     updating = true;
  //     downloadAndDrawTodo();
  //     updating = false;
  //   }

  //   free(inputChar);
  // }
}

#endif
