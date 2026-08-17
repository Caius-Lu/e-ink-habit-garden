#ifndef _HABIT_LOGIC_H_
#define _HABIT_LOGIC_H_

#include <stdint.h>

struct HabitProgress
{
  uint32_t total;
  uint16_t streak;
  int32_t lastDay;
};

inline bool checkInHabit(HabitProgress &progress, int32_t today)
{
  if (today <= 0 || progress.lastDay == today)
    return false;
  progress.streak = progress.lastDay == today - 1 ? progress.streak + 1 : 1;
  progress.lastDay = today;
  progress.total++;
  return true;
}

inline uint8_t plantStage(uint32_t waterDrops)
{
  if (waterDrops >= 60) return 5;
  if (waterDrops >= 30) return 4;
  if (waterDrops >= 15) return 3;
  if (waterDrops >= 6) return 2;
  if (waterDrops >= 2) return 1;
  return 0;
}

#endif
