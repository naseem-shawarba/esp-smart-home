#include "alarm_portal.h"

#include <Arduino.h>
#include "config.h"
#include "web_portal.h"
#include "web_config.h"
#include "device_config.h"
#include "rgb_led.h"

namespace alarm_portal
{
  static void tick()
  {
    // Blink blue while the portal is open (stealth-aware via rgb_led).
    if (millis() % 1500 > 750)
    {
      rgb_led::blue();
    }
    else
    {
      rgb_led::off();
    }
  }

  static bool exitPressed()
  {
    return digitalRead(portalPin) == HIGH;
  }

  void open()
  {
    web_portal::Options opts;
    opts.deviceName = device_config::name();
    opts.durationMs = device_config::portalWindowMs();
    opts.showMeshFields = true; // this is a sensor node
    opts.extraMenuLabel = "Alarm settings";
    opts.extraMenuHref = "/settings";
    opts.registerExtraRoutes = web_config::registerRoutes;
    opts.shouldExit = exitPressed;
    opts.onTick = tick;

    web_portal::run(opts);
    rgb_led::blue();
  }
}
