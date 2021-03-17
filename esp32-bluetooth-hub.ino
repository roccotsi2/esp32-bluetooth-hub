#include "BLEDevice.h"
//#include "BLEScan.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  setupBluetoothBle();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  if (!isConnected()) {
    tryConnect();
  }

  if (isConnected()) {
    tryGetData();
  }
  
  delay(1000); // Delay a second between loops.
} // End of loop
