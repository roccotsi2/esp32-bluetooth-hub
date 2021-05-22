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

#define GPIO_1        35
#define GPIO_2        34
#define GPIO_3        39

#define COLOR_WHITE   0xFF
#define COLOR_GREY    0x88
#define COLOR_BLACK   0x00

// define button pressed values
const byte BUTTON_NOT_PRESSED = 0;
const byte BUTTON_SHORT_PRESSED = 1;
const byte BUTTON_LONG_PRESSED = 2;
const int MILLIS_LONG_BUTTON_PRESS = 2000;

uint8_t demoMode = 0;

uint8_t *frameBuffer;

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

void setup() {
  Serial.begin(115200);

  buttonsInit();

  if (!demoMode) {
    Serial.println("Starting Arduino BLE Client application...");
    bluetoothSetupBluetoothBle();
  }
  displayInit();

  if (demoMode) {
    SmartbmsutilRunInfo runInfo;
    smartbmsdemoFillSmartbmsutilRunInfo(&runInfo);
    displayDrawContentBms(&runInfo);
  }
}

void loop() {
  if (!demoMode) {
    if (!bluetoothIsConnected()) {
      bluetoothTryConnect();
    }
    
    delay(3000);
  }

  /*int buttonPress = buttonsCheckButtonPressed(GPIO_1);
  if (buttonPress == BUTTON_SHORT_PRESSED) {
    displayPressedButton(1, "PRESSED ❬ ❭");
  } else if (buttonPress == BUTTON_LONG_PRESSED) {
    displayPressedButton(1, "LONG PRESSED");
  }*/
}
