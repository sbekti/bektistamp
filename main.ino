#include <Adafruit_NeoPixel.h>
#include <BleKeyboard.h>
#include <esp_task_wdt.h>
#include <esp_bt.h>

// While we wait for Feather ESP32 V2 to get added to the Espressif BSP,
// we have to select PICO D4 and UNCOMMENT this line!
#define ADAFRUIT_FEATHER_ESP32_V2

// then these pins will be defined for us
#if defined(ADAFRUIT_FEATHER_ESP32_V2)
#define PIN_NEOPIXEL 0
#define NEOPIXEL_I2C_POWER 2
#endif

#define CPU_FREQ_MHZ 80
#define WAKEUP_PIN GPIO_NUM_26
#define VBATPIN A13
#define SLEEP_TIMEOUT_MSECS 2 * 60 * 1000
#define WDT_TIMEOUT_SECS 3
// #define PRINT_DEBUG_LOGS 1

#define COLOR_BLUE 0x0000FF
#define COLOR_GREEN 0x00FF00
#define COLOR_WHITE 0xFFFFFF

std::string DEVICE_NAME = "Bektistamp 3000";
std::string MFG_NAME = "Samudra Bekti";

const char* MACROS[] = { "fastamp", "goodenough" };
#define MACRO_COUNT (sizeof(MACROS)/sizeof(char*))

#if defined(PIN_NEOPIXEL)
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

BleKeyboard bleKeyboard(DEVICE_NAME, MFG_NAME);

unsigned long timer = 0;

void setup() {
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

#if defined(PRINT_DEBUG_LOGS)
  Serial.begin(115200);
  print_wakeup_reason();
#endif

  // Configure GPIO26 as a wake-up source when the voltage is 0V.
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, LOW);
  pinMode(WAKEUP_PIN, INPUT_PULLUP);

  bleKeyboard.begin();
  enableInternalPower();

  blinkLED(4, COLOR_BLUE);

  esp_task_wdt_init(WDT_TIMEOUT_SECS, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                // add current thread to WDT watch

  int batteryLevel = getBatteryLevel();
  bleKeyboard.setBatteryLevel(batteryLevel);
}

void loop() {
  if (!digitalRead(WAKEUP_PIN)) {
    timer = millis();
    if (bleKeyboard.isConnected()) {
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
  } else if (millis() - timer >= SLEEP_TIMEOUT_MSECS) {
    blinkLED(4, COLOR_WHITE);
#if defined(PRINT_DEBUG_LOGS)
    Serial.println("Going into deep sleep mode, bye!");
    Serial.println("--------------------------------");
#endif
    start_deep_sleep();
  } else {
#if defined(PRINT_DEBUG_LOGS)
    Serial.println(millis() - timer);
#endif
    delay(100);
    esp_task_wdt_reset();
  }
}

void start_deep_sleep() {
  disableInternalPower();
  btStop();
  esp_deep_sleep_start();
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wake_up_source;
  wake_up_source = esp_sleep_get_wakeup_cause();

  switch (wake_up_source) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wake-up from external signal with RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wake-up from external signal with RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wake up caused by a timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wake up caused by a touchpad"); break;
    default: Serial.printf("Wake up not caused by Deep Sleep: %d\n", wake_up_source); break;
  }
}

int getBatteryLevel() {
  float batteryVoltage = analogReadMilliVolts(VBATPIN);
  batteryVoltage *= 2;  // we divided by 2, so multiply back

  int batteryLevel = map(batteryVoltage, 3412, 4192, 0, 100);
  return constrain(batteryLevel, 0, 100);
}

void blinkLED(int times, uint32_t color) {
  for (int i = 0; i < times; ++i) {
    LEDon(color);
    delay(100);
    LEDoff();
    delay(100);
  }
}

void LEDon(uint32_t color) {
#if defined(PIN_NEOPIXEL)
  pixel.begin();            // INITIALIZE NeoPixel
  pixel.setBrightness(20);  // not so bright
  pixel.setPixelColor(0, color);
  pixel.show();
#endif
}

void LEDoff() {
#if defined(PIN_NEOPIXEL)
  pixel.setPixelColor(0, 0x0);
  pixel.show();
#endif
}

void enableInternalPower() {
#if defined(NEOPIXEL_POWER)
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

#if defined(NEOPIXEL_I2C_POWER)
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
#endif

#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  // turn on the I2C power by setting pin to opposite of 'rest state'
  pinMode(PIN_I2C_POWER, INPUT);
  delay(1);
  bool polarity = digitalRead(PIN_I2C_POWER);
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, !polarity);
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif
}

void disableInternalPower() {
#if defined(NEOPIXEL_POWER)
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW);
#endif

#if defined(NEOPIXEL_I2C_POWER)
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);
#endif

#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  // turn on the I2C power by setting pin to rest state (off)
  pinMode(PIN_I2C_POWER, INPUT);
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW);
#endif
}
