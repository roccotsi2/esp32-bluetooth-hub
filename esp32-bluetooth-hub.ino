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
#include "firasans.h"
#include <Wire.h>
#include <touch.h>
#include "touchutil_datatypes.h"

#define GPIO_1        35 // <
#define GPIO_2        34 // >
#define GPIO_3        39 // SEL

#define COLOR_WHITE   0xFF
#define COLOR_GREY    0x88
#define COLOR_BLACK   0x00

#define TOUCH_INT     13

// 0: bluetooth connection will be established
// 1: test values instead of bluetooth connection
#define DEMO_MODE 1

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

static SemaphoreHandle_t mutexDisplay;

// variables
uint8_t *frameBuffer; // for display
unsigned long lastMillisMeasured = 0; // for BMS and scale
unsigned long lastMillisMeasuredScale = 0; // needed to calculate gas usage
uint16_t counterConnects = 0;
SmartbmsutilRunInfo _currentSmartbmsutilRunInfo;
GasData _gasData;
SavedDataConfiguration configuration;
bool bmsFound = false; // stores if the BMS was found initially
bool scaleFound = false; // stores if the scale was found initially
bool bmsConnectionSuccessful = false; // stores if the last connection attemp was successful
bool scaleConnectionSuccessful = false; // stores if the last connection attemp was successful
bool bluetoothDataReceived = false; // set to true, if data was successfull received
bool bluetoothDisabled = false; // if set to true, no connection via bluetooth is done
TouchClass touch;

// Touch variables
char pressedListBoxItem[30];
int buttonIdSetupHeader;
int buttonIdSetupBms;
int buttonIdSetupGas;
int buttonIdSetupBmsEnable;
int buttonIdSetupCancel;
int buttonIdSetupBmsScanBluetooth;

// Bluetooth variables
char foundBluetoothAddresses[10][20]; // 10 adresses with each max. 20 characters
char foundBluetoothNames[10][30]; // 10 names with each max. 30 characters
int foundBluetoothDevices;

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

void switchBluetoothEnabledState() {
  bluetoothDisabled = !bluetoothDisabled; // invert bluetoothDisabled
  displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData); // refresh the display to show the inverted connection state
  Serial.print("Bluetooth ");
  if (bluetoothDisabled) {
    Serial.println("disabled");
  } else {
    Serial.println("enabled");
  }
}

void fetchBmsData() {
  Serial.println("Fetch current RunInfo");
  bluetoothDisconnect(); // disconnect to be sure that no former connection is established
  delay(200);
  if (!bluetoothIsConnected()) {
    if (bluetoothConnectToServer(DEVICE_INDEX_BMS, serviceUUIDBms, charReadUUIDBms, charWriteUUIDBms)) {
      counterConnects++;
      Serial.print("# Connects: ");
      Serial.println(counterConnects);
    
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
}

void fetchScaleData() {
  Serial.println("Fetch current scale data");
  bluetoothDisconnect(); // disconnect to be sure that no former connection is established
  delay(200);
  if (!bluetoothIsConnected()) {
    Serial.println("Calling bluetoothConnectToServer");
    if (bluetoothConnectToServer(DEVICE_INDEX_SCALE, serviceUUIDScale, charReadUUIDScale, charWriteUUIDScale)) {
      counterConnects++;
      Serial.print("# Connects: ");
      Serial.println(counterConnects);
    
      if (bluetoothIsConnected()) {
        scaleutilSendCommandScaleCurrentWeightAsync(); // send command async, data is displayed by callback
        if (!waitUntilDataReceived(10)) {
          scaleConnectionSuccessful = false;
        }
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
}

void taskFetchAndDisplayBmsAndGasData(void * parameter) {
  for(;;){ // infinite loop
    if (DEMO_MODE == 0 && !bluetoothDisabled && (bmsFound || scaleFound)) {
      unsigned long currentMillis = millis();
      if (lastMillisMeasured == 0 || ((currentMillis - lastMillisMeasured) > configuration.updateIntervalSeconds * 1000)) {
        lastMillisMeasured = currentMillis;
        
        // next values needs to be fetched (BMS and scale)
        if (bmsFound && !configuration.skipBms) {
          fetchBmsData();
        }
        if (scaleFound && !configuration.skipScale) {
          fetchScaleData();
        }
        displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData);
        Serial.print("Free heap: ");
        Serial.println(ESP.getFreeHeap());
      }
    }
    
    // Pause the task again for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void touchInit() {
  pinMode(TOUCH_INT, INPUT_PULLUP);

  Wire.begin(15, 14);

  if (!touch.begin()) {
    Serial.println("start touchscreen failed");
    while (1);
  }
  Serial.println("Started Touchscreen poll...");
}

void showDataScreen() {
  if (DEMO_MODE == 0) {
    displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData);
  } else {
    // DEMO data
    SmartbmsutilRunInfo runInfo;
    smartbmsdemoFillSmartbmsutilRunInfo(&runInfo);
    displayDrawContentBmsDetail(&runInfo);
  }
}

void startScanForBms() {
  bmsFound = bluetoothScan(DEVICE_INDEX_BMS, "DL-", serviceUUIDBms);
  Serial.print("bmsFound = ");
  Serial.println(bmsFound);
  /*for (int i = 0; i < bluetoothGetFoundBluetoothDevices(); i++) {
    char address[20];
    char name[30];
    bool success = bluetoothGetFoundBluetoothDeviceAddress(i, address);
    if (success) {
      Serial.println(address);
    }
    success = bluetoothGetFoundBluetoothDeviceName(i, name);
    if (success) {
      Serial.println(name);
    }
  }*/
}

void setup() {
  Serial.begin(115200);

  mutexDisplay = xSemaphoreCreateMutex();
  buttonsInit();
  touchInit();
  displayInit();
  displayStartingMessage();

  Serial.println("Starting Arduino BLE Client application...");
  bluetoothSetupBluetoothBle();

  if (DEMO_MODE == 0) {    
    if (!configuration.skipBms) {
      bmsFound = bluetoothScan(DEVICE_INDEX_BMS, "DL-", serviceUUIDBms);
      Serial.print("bmsFound = ");
      Serial.println(bmsFound);
    } else {
      bmsFound = false;
      Serial.println("BMS skipped");
    }

    if (!configuration.skipScale) {
      scaleFound = bluetoothScan(DEVICE_INDEX_SCALE, "SCALE-", serviceUUIDScale);
      Serial.print("scaleFound = ");
      Serial.println(scaleFound);
    } else {
      scaleFound = false;
      Serial.println("Scale skipped");
    }
  } else {
    // DEMO data
    SmartbmsutilRunInfo runInfo;
    smartbmsdemoFillSmartbmsutilRunInfo(&runInfo);
    displayDrawContentBmsDetail(&runInfo);
  }
  Serial.println("Setup finished");

  // start tasks
  xTaskCreatePinnedToCore(
    taskFetchAndDisplayBmsAndGasData,     // Function that should be called
    "taskFetchAndDisplayBmsAndGasData",   // Name of the task (for debugging)
    10000,                                // Stack size (bytes)
    NULL,                                 // Parameter to pass
    1,                                    // Task priority
    NULL,                                 // Task handle
    0                                     // Core you want to run the task on (0 or 1)
  );
}

void checkPhysicalButtons() {
  int buttonPress = buttonsCheckButtonPressed(GPIO_3);
  if (buttonPress == BUTTON_SHORT_PRESSED) {
    Serial.println("Sel pressed");
  } else if (buttonPress == BUTTON_LONG_PRESSED) {
    Serial.println("Sel long pressed");
    switchBluetoothEnabledState();
  }
}

void checkTouchControls() {
  if (digitalRead(TOUCH_INT)) {
    touchutilCheckTouch(frameBuffer);
    ButtonData buttonData;
    if (touchutilGetPressedButton(&buttonData)) {
      // button found -> buttonData is filled
      Serial.print("Button pressed: ");
      Serial.print(buttonData.text);        
      Serial.print(" (ID = ");
      Serial.print(buttonData.id);
      Serial.println(")");
      if (buttonData.id == buttonIdSetupHeader) {
        Serial.println("Button for SETUP pressed");
        displaySetupMenuMain();
      } else if (buttonData.id == buttonIdSetupBms) {
        Serial.println("Button for SETUP BMS pressed");
        displaySetupBms();
      } else if (buttonData.id == buttonIdSetupGas) {
        Serial.println("Button for SETUP Gas pressed");
      } else if (buttonData.id == buttonIdSetupBmsEnable) {
        configuration.skipBms = !configuration.skipBms;
        displaySetupBms();
      } else if (buttonData.id == buttonIdSetupCancel) {
        Serial.println("Button for SETUP Cancel pressed");
        showDataScreen();
      } else if (buttonData.id == buttonIdSetupBmsScanBluetooth) {
        displayScanBluetoothRunning();
        startScanForBms();
        displayScanBluetoothResult();
      }
    } else if (touchutilGetPressedListBoxItem(pressedListBoxItem, sizeof(pressedListBoxItem))) {
      Serial.print("ListBoxItem pressed: ");
      Serial.println(pressedListBoxItem); 
      for (int i = 0; i < foundBluetoothDevices; i++) {
        if (strcmp(pressedListBoxItem, foundBluetoothNames[i]) == 0) {
          strcpy(configuration.bluetoothNameBms, foundBluetoothNames[i]);
          strcpy(configuration.bluetoothAddressBms, foundBluetoothAddresses[i]);
          displaySetupBms();   
        }
      }
    }
  }
}

void loop() {
  checkPhysicalButtons();
  checkTouchControls();

  // Pause the task again for 100ms
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
