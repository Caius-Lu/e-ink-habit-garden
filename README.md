# 墨水盆景

这是一块适合长期摆在桌面的 7.5 寸电子墨水屏：每天完成阅读、运动、早睡等“小日课”，就给屏幕里的植物浇一滴水。累计完成越多，种子会逐步长成幼苗、枝叶和花朵。

它保留了天气、每日名言、手机管理页和实体按钮，但不再以待办清单为中心。

## 日常使用

- 单击实体按钮：依次完成今天尚未打卡的一项日课，并刷新屏幕。
- 长按按钮：进入 Wi-Fi 配网模式。
- 手机访问 `http://einktodo.local` 或屏幕显示的设备 IP：查看成长状态、单独打卡、修改三件日课、城市和名言。
- 数据保存在设备 LittleFS 中，断电或重启不会丢失。

植物有六个成长阶段，累计浇水达到 2、6、15、30、60 滴时升级。漏掉一天不会扣除累计成长值，只会重新计算该习惯的连续天数。

## 构建与烧录

项目使用 PlatformIO。当前实物为 ESP8266 + CH340 串口时：

```powershell
pio run -e esp8266
pio run -e esp8266 -t upload --upload-port COM3
pio device monitor --port COM3 --baud 115200
```

ESP32 使用 `esp32` 环境。详细屏幕型号与编译宏在 [platformio.ini](platformio.ini) 中配置。

## 默认接线

| 平台 | CS | DC | RST | BUSY | 按钮 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ESP8266 | GPIO15 | GPIO4 | GPIO2 | GPIO5 | GPIO16 接 GND |
| ESP32 | GPIO15 | GPIO27 | GPIO26 | GPIO25 | GPIO16 接 GND |

## 手机页面

- `/`：今日浇水、连续天数、成长阶段和设备状态。
- `/settings`：修改三项日课和天气城市；旧版 GitHub、AI 用量与 einktodo 参数仍保留为兼容配置。
- `/quotes`：一行一条编辑底栏名言。

管理页只面向可信局域网，没有登录功能。API Key 不再内置在源码或打印到串口。

## 项目结构

- `src/habitLogic.h`：可独立测试的连续天数与植物阶段规则。
- `src/habitGarden.h`：日课名称、打卡和 LittleFS 持久化。
- `src/dashboard.h`：800×480 墨水盆景界面。
- `src/webui.h`：手机管理与打卡页面。
- `test/test_habit_logic/`：习惯规则的边界测试。

项目最初基于 [einktodo](https://einktodo.com) 待办显示方案扩展。
