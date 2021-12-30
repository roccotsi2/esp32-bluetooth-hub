#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include "BLEDevice.h"
#include "datatypes.h"
#include "epd_driver.h"
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

#define GPIO_1        35 // <
#define GPIO_2        34 // >
#define GPIO_3        39 // SEL

#define COLOR_WHITE   0xFF
#define COLOR_GREY    0x88
#define COLOR_BLACK   0x00

// 0: bluetooth connection will be established
// 1: test values instead of bluetooth connection
#define DEMO_MODE 0

// define button pressed values
const byte BUTTON_NOT_PRESSED = 0;
const byte BUTTON_SHORT_PRESSED = 1;
const byte BUTTON_LONG_PRESSED = 2;
const int MILLIS_LONG_BUTTON_PRESS = 2000;

// define timings
const unsigned long INTERVAL_READ_BMS_MILLIS = 5000; // read BMS each minute

// variables
uint8_t *frameBuffer;
unsigned long lastMillisMeasured = 0;
int counter = 0;
SmartbmsutilRunInfo _currentSmartbmsutilRunInfo;
GasData _gasData;

/*void displayPressedButton(int buttonNo, char *state) {
  epd_poweron();
  epd_clear();
  int cursor_x = 0;
  int cursor_y = 300;
  char buttonText[100];
  sprintf(buttonText, "Button %s: %s", String(buttonNo), state);
  writeln((GFXfont *)&FiraSans, buttonText, &cursor_x, &cursor_y, NULL);
  epd_poweroff();
}*/

void fetchAndDisplayBmsData() {
  Serial.println("Fetch current RunInfo");
  if (!bluetoothIsConnected()) {
    Serial.println("Not connected, try to connect");
    bluetoothTryConnect();
    counter++;
  }

  Serial.print("# Connects: ");
  Serial.println(counter);

  if (bluetoothIsConnected()) {
    // smartbmsutilSendCommandRunInfoAsync();
    byte buffer[sizeof(SmartbmsutilRunInfo)];
    bool success = smartbmsutilReadRunInfo(buffer, sizeof(buffer));
    if (success) {
      smartbmsutilDataReceived(buffer, sizeof(buffer));
    }
    bluetoothDisconnect();
  }
  delay(500); // wait 500ms to be sure that connection is correctly closed
}

void setup() {
  Serial.begin(115200);

  buttonsInit();

  displayInit();
  displayStartingMessage();

  if (DEMO_MODE == 0) {
    Serial.println("Starting Arduino BLE Client application...");
    bluetoothSetupBluetoothBle();
    bluetoothStartScan();
  }

  if (DEMO_MODE == 1) {
    SmartbmsutilRunInfo runInfo;
    smartbmsdemoFillSmartbmsutilRunInfo(&runInfo);
    displayDrawContentBmsDetail(&runInfo);
  }

  // TODO: remove demo scale data
  _gasData.fillingLevelPercent = 78;
  _gasData.nettoWeightGram = (11000 * _gasData.fillingLevelPercent) / 100;
  _gasData.remainingDays = 12;
  _gasData.usagePerDayGram = _gasData.nettoWeightGram / _gasData.remainingDays;
}

void loop() {
  if (DEMO_MODE == 0) {
    unsigned long currentMillis = millis();
    if (lastMillisMeasured == 0 || ((currentMillis - lastMillisMeasured) > INTERVAL_READ_BMS_MILLIS)) {
      lastMillisMeasured = currentMillis;
      fetchAndDisplayBmsData();

      //delay(5000);

      Serial.print("Free heap: ");
      Serial.println(ESP.getFreeHeap());
    }
  }

  int buttonPress = buttonsCheckButtonPressed(GPIO_3);
  if (buttonPress == BUTTON_SHORT_PRESSED) {
    Serial.println("Sel pressed");
  } else if (buttonPress == BUTTON_LONG_PRESSED) {
    Serial.println("Sel long pressed");
    bluetoothDisconnect();
  }
}
