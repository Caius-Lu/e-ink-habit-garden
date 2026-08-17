#include <unity.h>
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "habitLogic.h"

void test_first_checkin_starts_streak()
{
  HabitProgress p = {0, 0, 0};
  TEST_ASSERT_TRUE(checkInHabit(p, 20000));
  TEST_ASSERT_EQUAL_UINT32(1, p.total);
  TEST_ASSERT_EQUAL_UINT16(1, p.streak);
  TEST_ASSERT_EQUAL_INT32(20000, p.lastDay);
}

void test_duplicate_checkin_is_ignored()
{
  HabitProgress p = {8, 3, 20000};
  TEST_ASSERT_FALSE(checkInHabit(p, 20000));
  TEST_ASSERT_EQUAL_UINT32(8, p.total);
  TEST_ASSERT_EQUAL_UINT16(3, p.streak);
}

void test_consecutive_day_extends_streak()
{
  HabitProgress p = {8, 3, 20000};
  TEST_ASSERT_TRUE(checkInHabit(p, 20001));
  TEST_ASSERT_EQUAL_UINT16(4, p.streak);
}

void test_gap_restarts_streak()
{
  HabitProgress p = {8, 9, 20000};
  TEST_ASSERT_TRUE(checkInHabit(p, 20003));
  TEST_ASSERT_EQUAL_UINT16(1, p.streak);
}

void test_plant_stage_is_bounded()
{
  TEST_ASSERT_EQUAL_UINT8(0, plantStage(0));
  TEST_ASSERT_EQUAL_UINT8(1, plantStage(3));
  TEST_ASSERT_EQUAL_UINT8(2, plantStage(8));
  TEST_ASSERT_EQUAL_UINT8(3, plantStage(18));
  TEST_ASSERT_EQUAL_UINT8(4, plantStage(35));
  TEST_ASSERT_EQUAL_UINT8(5, plantStage(80));
  TEST_ASSERT_EQUAL_UINT8(5, plantStage(10000));
}

void runTests()
{
  UNITY_BEGIN();
  RUN_TEST(test_first_checkin_starts_streak);
  RUN_TEST(test_duplicate_checkin_is_ignored);
  RUN_TEST(test_consecutive_day_extends_streak);
  RUN_TEST(test_gap_restarts_streak);
  RUN_TEST(test_plant_stage_is_bounded);
  UNITY_END();
}

#ifdef ARDUINO
void setup() { delay(2000); runTests(); }
void loop() {}
#else
int main(int argc, char **argv) { runTests(); return 0; }
#endif
