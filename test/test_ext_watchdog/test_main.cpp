// Host unit tests for the external watchdog pulse step (src/ext_watchdog.h).
// No hardware. Run with:
//   pio test -e native_ext_watchdog

#include <unity.h>

#include <cstdint>

#include "ext_watchdog.h"

void setUp(void) {}
void tearDown(void) {}

// Drives the step like nrf52loop() does: one call per loop pass of `pass` ms.
static int countPulses(uint32_t start, uint32_t duration, uint32_t pass, uint32_t *maxHigh)
{
    uint32_t timer = start - EXT_WDT_INTERVAL_MS;
    bool high = false;
    uint32_t risen = 0;
    int pulses = 0;
    *maxHigh = 0;
    for (uint32_t t = start; (uint32_t)(t - start) < duration; t += pass)
    {
        ExtWdtStep s = extWdtPhase(t, timer, high);
        timer = s.timer;
        high = s.high;
        if (s.changed && high)
        {
            pulses++;
            risen = t;
        }
        if (s.changed && !high && (uint32_t)(t - risen) > *maxHigh)
            *maxHigh = t - risen;
    }
    return pulses;
}

void test_first_pulse_right_after_boot(void)
{
    ExtWdtStep s = extWdtPhase(1000, 1000 - EXT_WDT_INTERVAL_MS, false);
    TEST_ASSERT_TRUE(s.high);
    TEST_ASSERT_TRUE(s.changed);
    TEST_ASSERT_EQUAL_UINT32(1000, s.timer);
}

void test_pulse_ends_after_pulse_width(void)
{
    ExtWdtStep s = extWdtPhase(1000 + EXT_WDT_PULSE_MS - 1, 1000, true);
    TEST_ASSERT_TRUE(s.high);
    TEST_ASSERT_FALSE(s.changed);
    s = extWdtPhase(1000 + EXT_WDT_PULSE_MS, 1000, true);
    TEST_ASSERT_FALSE(s.high);
    TEST_ASSERT_TRUE(s.changed);
    TEST_ASSERT_EQUAL_UINT32(1000, s.timer);   // interval counts from the rising edge
}

void test_one_pulse_per_interval_with_100ms_loop(void)
{
    uint32_t maxHigh;
    // 5 minutes of a 100 ms loop: pulses at 0, 30, ..., 270 s
    TEST_ASSERT_EQUAL_INT(10, countPulses(5000, 300000, 100, &maxHigh));
    TEST_ASSERT_EQUAL_UINT32(EXT_WDT_PULSE_MS, maxHigh);
}

void test_slow_loop_stretches_pulse_not_interval(void)
{
    uint32_t maxHigh;
    // a 700 ms loop pass: still one pulse per interval, HIGH lasts one pass
    TEST_ASSERT_EQUAL_INT(10, countPulses(5000, 300000, 700, &maxHigh));
    TEST_ASSERT_EQUAL_UINT32(700, maxHigh);
}

void test_millis_wraparound(void)
{
    uint32_t maxHigh;
    TEST_ASSERT_EQUAL_INT(4, countPulses(0xFFFFFFFFu - 50000, 100000, 100, &maxHigh));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_pulse_right_after_boot);
    RUN_TEST(test_pulse_ends_after_pulse_width);
    RUN_TEST(test_one_pulse_per_interval_with_100ms_loop);
    RUN_TEST(test_slow_loop_stretches_pulse_not_interval);
    RUN_TEST(test_millis_wraparound);
    return UNITY_END();
}
