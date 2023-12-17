#include <Adafruit_NeoPixel.h>
#include <BleKeyboard.h>
#include <esp_task_wdt.h>

// While we wait for Feather ESP32 V2 to get added to the Espressif BSP,
// we have to select PICO D4 and UNCOMMENT this line!
#define ADAFRUIT_FEATHER_ESP32_V2

// then these pins will be defined for us
#if defined(ADAFRUIT_FEATHER_ESP32_V2)
#define PIN_NEOPIXEL 0
#define NEOPIXEL_I2C_POWER 2
#endif

#define WAKEUP_PIN GPIO_NUM_26
#define SLEEP_TIMEOUT 2 * 60 * 1000
#define WDT_TIMEOUT 3
// #define PRINT_DEBUG_LOGS 1

std::string DEVICE_NAME = "Bektistamp 3000";
std::string MFG_NAME = "Samudra Bekti";
String MACRO = "fastamp";

#if defined(PIN_NEOPIXEL)
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

BleKeyboard bleKeyboard(DEVICE_NAME, MFG_NAME);

unsigned long timer = 0;

void setup() {
#if defined(PRINT_DEBUG_LOGS)
  Serial.begin(115200);
  print_wakeup_reason();
#endif

  // Configure GPIO26 as a wake-up source when the voltage is 0V.
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, LOW);
  pinMode(WAKEUP_PIN, INPUT_PULLUP);

  bleKeyboard.begin();
  enableInternalPower();

  blinkLED(4, 0x00FFFF);

  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
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

      bleKeyboard.print(MACRO);
      delay(500);

      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press(KEY_RETURN);
      delay(50);
      bleKeyboard.releaseAll();

      blinkLED(4, 0x00FF00);
    }
  } else if (millis() - timer >= SLEEP_TIMEOUT) {
    blinkLED(4, 0xFFFF00);
#if defined(PRINT_DEBUG_LOGS)
    Serial.println("Going into deep sleep mode, bye!");
    Serial.println("--------------------------------");
#endif
    disableInternalPower();
    esp_deep_sleep_start();
  } else {
#if defined(PRINT_DEBUG_LOGS)
    Serial.println(millis() - timer);
#endif
    delay(100);
    esp_task_wdt_reset();
  }
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
