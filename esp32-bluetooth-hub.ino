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

const byte DEVICE_INDEX_BMS = 0;
const byte DEVICE_INDEX_SCALE = 1;

// The remote service we wish to connect to (Smart BMS)
static BLEUUID serviceUUIDBms("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID charReadUUIDBms("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID charWriteUUIDBms("0000fff2-0000-1000-8000-00805f9b34fb");

// The remote service we wish to connect to (scale)
static BLEUUID serviceUUIDScale("df469ada-185e-11ec-9621-0242ac130002");
static BLEUUID charReadUUIDScale("e8f1aaac-185e-11ec-9621-0242ac130002");
static BLEUUID charWriteUUIDScale("71c90414-1860-11ec-9621-0242ac130002");

// variables
uint8_t *frameBuffer;
unsigned long lastMillisMeasuredBms = 0;
unsigned long lastMillisMeasuredScale = 0;
int counter = 0;
SmartbmsutilRunInfo _currentSmartbmsutilRunInfo;
GasData _gasData;
SavedDataConfiguration configuration;
bool bmsFound = false; // stores if the BMS was found initially
bool scaleFound = false; // stores if the scale was found initially
bool bmsConnectionSuccessful = false; // stores if the last connection attemp was successful
bool scaleConnectionSuccessful = false; // stores if the last connection attemp was successful
bool bluetoothDataReceived = false; // set to true, if data was successfull received
bool bluetoothDisabled = false; // if set to true, no connection via bluetooth is done

bool waitUntilDataReceived(int timeoutSeconds) {
  unsigned long startMillis = millis();
  while (!bluetoothDataReceived && (millis() - startMillis < timeoutSeconds * 1000)) {
    delay(100);
  }

  if (!bluetoothDataReceived) {
    Serial.println("waitUntilDataReceived: no data received, timeout reached");
    return false;
  } else {
    Serial.println("waitUntilDataReceived: data received");
    return true;
  }
}

void fetchAndDisplayBmsData() {
  Serial.println("Fetch current RunInfo");
  bluetoothDisconnect(); // disconnect to be sure that no former connection is established
  delay(200);
  if (!bluetoothIsConnected()) {
    if (bluetoothConnectToServer(DEVICE_INDEX_BMS, serviceUUIDBms, charReadUUIDBms, charWriteUUIDBms)) {
      counter++;
      Serial.print("# Connects: ");
      Serial.println(counter);
    
      if (bluetoothIsConnected()) {
        smartbmsutilSendCommandRunInfoAsync(); // send command async, data is displayed by callback
        if (!waitUntilDataReceived(10)) {
          bmsConnectionSuccessful = false;
        }
      } else {
        Serial.println("Could not connect to BMS, no data was sent");
        bmsConnectionSuccessful = false;
      }
    } else {
      Serial.println("Could not connect to BMS, no data was sent");
      bmsConnectionSuccessful = false;
    }
  } else {
    Serial.println("Could not disconnect bluetooth, no data was sent");
    bmsConnectionSuccessful = false;
  }  

  if (!bmsConnectionSuccessful) {
    displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData); // refresh the display to update the status
  }
}

void fetchAndDisplayScaleData() {
  Serial.println("Fetch current scale data");
  bluetoothDisconnect(); // disconnect to be sure that no former connection is established
  delay(200);
  if (!bluetoothIsConnected()) {
    if (bluetoothConnectToServer(DEVICE_INDEX_SCALE, serviceUUIDScale, charReadUUIDScale, charWriteUUIDScale)) {
      counter++;
      Serial.print("# Connects: ");
      Serial.println(counter);
    
      if (bluetoothIsConnected()) {
        /*smartbmsutilSendCommandRunInfoAsync(); // send command async, data is displayed by callback
        if (!waitUntilDataReceived(10)) {
          bmsConnectionSuccessful = false;
        }*/
      } else {
        Serial.println("Could not connect to scale, no data was sent");
        scaleConnectionSuccessful = false;
      }
    } else {
      Serial.println("Could not connect to scale, no data was sent");
      scaleConnectionSuccessful = false;
    }
  } else {
    Serial.println("Could not disconnect bluetooth, no data was sent");
    scaleConnectionSuccessful = false;
  }  

  if (!bmsConnectionSuccessful) {
    displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData); // refresh the display to update the status
  }
}

void setup() {
  Serial.begin(115200);

  buttonsInit();

  displayInit();
  displayStartingMessage();

  if (DEMO_MODE == 0) {
    Serial.println("Starting Arduino BLE Client application...");
    bluetoothSetupBluetoothBle();
    
    if (!configuration.skipBms) {
      bmsFound = bluetoothScan(DEVICE_INDEX_BMS, "DL-", serviceUUIDBms);
      Serial.print("bmsFound = ");
      Serial.println(bmsFound);
    } else {
      Serial.println("BMS skipped");
    }

    if (!configuration.skipScale) {
      scaleFound = bluetoothScan(DEVICE_INDEX_SCALE, "SCALE-", serviceUUIDScale);
      Serial.print("scaleFound = ");
      Serial.println(scaleFound);
    } else {
      Serial.println("Scale skipped");
    }
  } else {
    SmartbmsutilRunInfo runInfo;
    smartbmsdemoFillSmartbmsutilRunInfo(&runInfo);
    displayDrawContentBmsDetail(&runInfo);
  }

  // TODO: remove demo scale data
  _gasData.fillingLevelPercent = 78;
  _gasData.nettoWeightGram = (11000 * _gasData.fillingLevelPercent) / 100;
  _gasData.remainingDays = 12;
  _gasData.usagePerDayGram = _gasData.nettoWeightGram / _gasData.remainingDays;

  Serial.println("Setup finished");
}

void loop() {
  if (DEMO_MODE == 0 && !bluetoothDisabled) {
    unsigned long currentMillis = millis();
    if (!configuration.skipBms && (lastMillisMeasuredBms == 0 || ((currentMillis - lastMillisMeasuredBms) > configuration.updateIntervalBmsSeconds * 1000))) {
      lastMillisMeasuredBms = currentMillis;
      fetchAndDisplayBmsData();

      Serial.print("Free heap: ");
      Serial.println(ESP.getFreeHeap());
    }

    if (!configuration.skipScale && (lastMillisMeasuredScale == 0 || ((currentMillis - lastMillisMeasuredScale) > configuration.updateIntervalScaleSeconds * 1000))) {
      lastMillisMeasuredScale = currentMillis;
      fetchAndDisplayScaleData();
    }
  }

  int buttonPress = buttonsCheckButtonPressed(GPIO_3);
  if (buttonPress == BUTTON_SHORT_PRESSED) {
    Serial.println("Sel pressed");
  } else if (buttonPress == BUTTON_LONG_PRESSED) {
    Serial.println("Sel long pressed");
    bluetoothDisabled = !bluetoothDisabled; // invert bluetoothDisabled
    displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData); // refresh the display to show the inverted connection state
  }
}
