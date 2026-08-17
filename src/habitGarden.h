#ifndef _HABIT_GARDEN_H_
#define _HABIT_GARDEN_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h>
#include "clock.h"
#include "habitLogic.h"

#define HABIT_COUNT 3

struct HabitGarden
{
  char names[HABIT_COUNT][25];
  HabitProgress progress[HABIT_COUNT];
};

HabitGarden garden;
const char *gardenFileName = "/garden.json";

int32_t gardenToday()
{
  time_t now = time(nullptr);
  if (now < 24 * 3600)
    return 0;
  return (int32_t)((now + TZ_SEC) / 86400);
}

void resetGardenDefaults()
{
  memset(&garden, 0, sizeof(garden));
  strncpy(garden.names[0], "阅读", sizeof(garden.names[0]) - 1);
  strncpy(garden.names[1], "运动", sizeof(garden.names[1]) - 1);
  strncpy(garden.names[2], "早睡", sizeof(garden.names[2]) - 1);
}

bool saveGarden()
{
  JsonDocument doc;
  JsonArray habits = doc["habits"].to<JsonArray>();
  for (int i = 0; i < HABIT_COUNT; i++)
  {
    JsonObject h = habits.add<JsonObject>();
    h["name"] = garden.names[i];
    h["total"] = garden.progress[i].total;
    h["streak"] = garden.progress[i].streak;
    h["lastDay"] = garden.progress[i].lastDay;
  }
  File f = LittleFS.open(gardenFileName, "w");
  if (!f) return false;
  bool ok = serializeJson(doc, f) > 0;
  f.close();
  return ok;
}

void loadGarden()
{
  resetGardenDefaults();
  File f = LittleFS.open(gardenFileName, "r");
  if (!f) return;
  JsonDocument doc;
  if (deserializeJson(doc, f))
  {
    f.close();
    return;
  }
  f.close();
  JsonArrayConst habits = doc["habits"].as<JsonArrayConst>();
  for (int i = 0; i < HABIT_COUNT && i < (int)habits.size(); i++)
  {
    strlcpy(garden.names[i], habits[i]["name"] | garden.names[i], sizeof(garden.names[i]));
    garden.progress[i].total = habits[i]["total"] | 0;
    garden.progress[i].streak = habits[i]["streak"] | 0;
    garden.progress[i].lastDay = habits[i]["lastDay"] | 0;
  }
}

bool habitDoneToday(int index)
{
  return index >= 0 && index < HABIT_COUNT && garden.progress[index].lastDay == gardenToday();
}

bool gardenCheckIn(int index)
{
  if (index < 0 || index >= HABIT_COUNT)
    return false;
  if (!checkInHabit(garden.progress[index], gardenToday()))
    return false;
  saveGarden();
  return true;
}

int gardenCheckInNext()
{
  for (int i = 0; i < HABIT_COUNT; i++)
    if (!habitDoneToday(i) && gardenCheckIn(i)) return i;
  return -1;
}

uint32_t gardenTotal()
{
  uint32_t total = 0;
  for (int i = 0; i < HABIT_COUNT; i++) total += garden.progress[i].total;
  return total;
}

int gardenDoneCount()
{
  int done = 0;
  for (int i = 0; i < HABIT_COUNT; i++) if (habitDoneToday(i)) done++;
  return done;
}

#endif
