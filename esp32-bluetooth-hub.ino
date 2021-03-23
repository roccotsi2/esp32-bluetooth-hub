#include "BLEDevice.h"
#include "datatypes.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  bluetoothSetupBluetoothBle();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  if (!bluetoothIsConnected()) {
    bluetoothTryConnect();
  }
  
  delay(1000); // Delay a second between loops.
} // End of loop
