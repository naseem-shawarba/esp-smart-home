#pragma once

#include <Arduino.h>

// =========================
// Pin Definitions (GPIO)
// =========================
#define PIN_BITMASK(GPIO) (1ULL << GPIO) // 2 ^ GPIO_NUMBER in hex

// Wakeup pins
#define wakeupGpioDoor GPIO_NUM_13
#define wakeupGpioStatus GPIO_NUM_25
#define wakeupGpioToggle GPIO_NUM_26
#define wakeupGpioPortal GPIO_NUM_27

// Input/output pins
#define bluePin 23
#define greenPin 22
#define redPin 21
#define buzzerPin 19
#define doorPin 13
#define portalPin 27
// #define builtInLedPin 2

// =========================
// Buzzer settings
// =========================
const int buzzerChannel = 0;    // PWM channel 0
const int buzzerFreq = 2000;    // Frequency in Hz
const int buzzerResolution = 8; // 8-bit resolution

// =========================
// Alarm limits
// =========================
const int maxAlarmTriggers = 3;

// =========================
// Default Settings (used as fallbacks by the settings module when NVS is empty)
// =========================
const bool DEFAULT_stealthMode = false;                                             // LED + buzzer enabled
const unsigned long DEFAULT_alarmDeactivationWindowMs = 8000;                       // 8 seconds to deactivate alarm
const unsigned long DEFAULT_alarmDeactivationLedBlinkMs = 500;                      // LED blink interval
const unsigned long DEFAULT_postAlarmActivationDelayMs = 1000UL * 60 * 30;          // 30 minutes after alarm activation
const unsigned long DEFAULT_postAlarmTriggerDelayMs = 1000UL * 60 * 20;             // 20 minutes after alarm activation
const unsigned long DEFAULT_temporarilyDisableAlarmDelayMs = 1000UL * 60 * 60 * 24; // 1 Day
