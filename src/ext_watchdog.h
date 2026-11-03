// ext_watchdog.h -- pulse for an external hardware watchdog. Arduino-free, header-only.
//
// nRF52 boards run without any watchdog (meshcom_wdt_feed() is a no-op there), so a hung
// firmware stays hung. With EXT_WDT_PIN defined, nrf52loop() drives that pin HIGH for
// EXT_WDT_PULSE_MS every EXT_WDT_INTERVAL_MS; an external watchdog (TPL5010 DONE input,
// ATtiny, timer relay, ...) power-cycles the node when the pulses stop.
//
// The pulse is generated from the main loop on purpose: a timer or a separate task would
// keep pulsing while the loop is stuck and the watchdog would never fire. The pulse width
// is therefore at least one loop pass (the loop ends with delay(100)).
//
// The pin driving stays in nrf52_main.cpp (extWdtSetup, extWdtTick, extWdtForceLow);
// extWdtPhase() is the pure step tested on the host (test/test_ext_watchdog).
#ifndef EXT_WATCHDOG_H
#define EXT_WATCHDOG_H

#include <stdint.h>

#ifndef EXT_WDT_INTERVAL_MS
#define EXT_WDT_INTERVAL_MS 30000   // time between two pulse starts
#endif
#ifndef EXT_WDT_PULSE_MS
#define EXT_WDT_PULSE_MS    100     // minimum HIGH time of one pulse
#endif

struct ExtWdtStep
{
    bool high;         // new pin state
    uint32_t timer;    // start of the last pulse
    bool changed;      // high differs from the input: the caller drives the pin only then
};

// One pulse step; the pin starts LOW. Wrap-safe for the uint32 millis().
// Start with timer = now - interval to get the first pulse right after boot.
inline ExtWdtStep extWdtPhase(uint32_t now, uint32_t timer, bool high,
                              uint32_t interval = EXT_WDT_INTERVAL_MS,
                              uint32_t pulse = EXT_WDT_PULSE_MS)
{
    ExtWdtStep s = {high, timer, false};
    if (!high)
    {
        if ((uint32_t)(now - timer) >= interval)
        {
            s.high = true;
            s.timer = now;
            s.changed = true;
        }
    }
    else if ((uint32_t)(now - timer) >= pulse)
    {
        s.high = false;
        s.changed = true;
    }
    return s;
}

#endif // EXT_WATCHDOG_H
