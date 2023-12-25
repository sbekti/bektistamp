#include <Arduino.h>
#include <BleKeyboard.h>
#include <esp_task_wdt.h>
#include <Adafruit_NeoPixel.h>
#include <InterpolationLib.h>
#include "main.h"

#define CPU_FREQ_MHZ 80
#define WDT_TIMEOUT_SECS 3
#define SLEEP_TIMEOUT_MSECS 30 * 1000
#define WAKEUP_PIN GPIO_NUM_26

#define BLE_DEVICE_NAME "Bektistamp 3000"
#define BLE_MFG_NAME "Samudra Bekti"

const char *MACROS[] = {"fastamp", "goodenough"};
#define MACRO_COUNT (sizeof(MACROS) / sizeof(char *))

#define COLOR_RED 0xFF0000
#define COLOR_BLUE 0x0000FF
#define COLOR_GREEN 0x00FF00
#define COLOR_WHITE 0xFFFFFF

double BAT_PCT_VALUES[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
#define BAT_LUT_SIZE (sizeof(BAT_PCT_VALUES) / sizeof(double))
double BAT_VOLTAGE_VALUES[BAT_LUT_SIZE] = {3270, 3610, 3690, 3710, 3730, 3750, 3770, 3790, 3800, 3820, 3840, 3850, 3870, 3910, 3950, 3980, 4020, 4080, 4110, 4150, 4200};

BleKeyboard bleKeyboard(BLE_DEVICE_NAME, BLE_MFG_NAME);
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
uint32_t timer = 0;

void setup()
{
  setCpuFrequencyMhz(CPU_FREQ_MHZ);
  enableI2CPower();

  // Some boards work best if we also make a serial connection
  Serial.begin(115200);
  printWakeupReason();

  // set LED to be an output pin
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(WAKEUP_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, LOW);

  esp_task_wdt_init(WDT_TIMEOUT_SECS, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                    // add current thread to WDT watch

  bleKeyboard.begin();

  int batteryLevel = getBatteryLevel();
  Serial.print("Battery pct: ");
  Serial.println(batteryLevel);
  bleKeyboard.setBatteryLevel(batteryLevel);

  if (batteryLevel <= 20)
  {
    blinkLED(4, COLOR_RED);
  }
  else
  {
    blinkLED(4, COLOR_BLUE);
  }
}

void loop()
{
  if (!digitalRead(WAKEUP_PIN))
  {
    timer = millis();
    if (bleKeyboard.isConnected())
    {
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press('a');
      delay(50);
      bleKeyboard.releaseAll();

      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press('c');
      delay(50);
      bleKeyboard.releaseAll();

      bleKeyboard.print(MACROS[random(MACRO_COUNT)]);
      delay(500);

      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_RETURN);
      delay(50);
      bleKeyboard.releaseAll();

      blinkLED(4, COLOR_GREEN);
    }
  }
  else if (millis() - timer >= SLEEP_TIMEOUT_MSECS)
  {
    Serial.println("Going into deep sleep mode, bye!");
    Serial.println("--------------------------------");
    enterDeepSleep();
  }
  else
  {
    delay(25);
    esp_task_wdt_reset();
  }
}

void enterDeepSleep()
{
  blinkLED(4, COLOR_WHITE);
  disableI2CPower();

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

  esp_deep_sleep_start();
}

void enableI2CPower()
{
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
}

void disableI2CPower()
{
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);
}

void blinkLED(int times, uint32_t color)
{
  for (int i = 0; i < times; ++i)
  {
    ledOn(color);
    delay(100);
    ledOff();
    delay(100);
  }
}

void ledOn(uint32_t color)
{
  pixel.begin();
  pixel.setBrightness(20);
  pixel.setPixelColor(0, color);
  pixel.show();
}

void ledOff()
{
  pixel.setPixelColor(0, 0x0);
  pixel.show();
}

void printWakeupReason()
{
  esp_sleep_wakeup_cause_t wake_up_source;
  wake_up_source = esp_sleep_get_wakeup_cause();

  switch (wake_up_source)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wake-up from external signal with RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wake-up from external signal with RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wake up caused by a timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wake up caused by a touchpad");
    break;
  default:
    Serial.printf("Wake up not caused by Deep Sleep: %d\n", wake_up_source);
    break;
  }
}

int getBatteryLevel()
{
  float voltage = analogReadMilliVolts(BATT_MONITOR);
  voltage *= 2; // we divided by 2, so multiply back

  Serial.print("VBat: ");
  Serial.println(voltage);

  if (voltage <= BAT_VOLTAGE_VALUES[0])
  {
    return 0;
  }
  else if (voltage >= BAT_VOLTAGE_VALUES[BAT_LUT_SIZE - 1])
  {
    return 100;
  }

  double batteryPct = Interpolation::ConstrainedSpline(BAT_VOLTAGE_VALUES, BAT_PCT_VALUES, BAT_LUT_SIZE, voltage);
  return round(batteryPct);
}