//static boolean doConnect = false;
//static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristicRead;
static BLERemoteCharacteristic* pRemoteCharacteristicWrite;
//static BLEAdvertisedDevice* myDevice;
static std::string myDeviceAddresses[2];
static BLEScan* pBLEScan;
static BLEClient* pClient;
byte currentDeviceNo = 0;
byte countDevices = 0;
byte scanDeviceIndex;
char* scanName;
BLEUUID scanServiceUuid;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (smartbmsutilDataReceived(pData, length)) {
    // disconnect after data was successful read
    bluetoothDisconnect();
    delay(200); // wait to be sure that connection is correctly closed
    bluetoothDataReceived = true;
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connection established");
  }

  void onDisconnect(BLEClient* pclient) {
    //connected = false;
    Serial.println("Connection lost");    
  }
};

bool bluetoothConnectToServer(byte deviceIndex, BLEUUID charReadUUID, BLEUUID charWriteUUID) {
  if (countDevices == 0 || currentDeviceNo >= countDevices) {
    return false;
  }
  Serial.print("Forming a connection to ");
  Serial.println(myDeviceAddresses[currentDeviceNo].c_str());

  // Connect to the remove BLE Server.
  int count = 0;
  boolean success = false;
  unsigned long lastMillis = millis();
  while (count < 10 && !success && (millis() - lastMillis < 29000)) { // connection timeout is 30 sec (do not retry if connection timed out before)
    delay(100);
    success = pClient->connect(BLEAddress(myDeviceAddresses[currentDeviceNo]));
    count++;
  }
  if (!success) {
    Serial.println("Unable to establish connection");
    return false;
  }
  
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    bluetoothDisconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic for reading in the service of the remote BLE server.
  pRemoteCharacteristicRead = pRemoteService->getCharacteristic(charReadUUID);
  if (pRemoteCharacteristicRead == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charReadUUID.toString().c_str());
    bluetoothDisconnect();
    return false;
  }
  Serial.println(" - Found our characteristic for reading");

  // Obtain a reference to the characteristic for writing in the service of the remote BLE server.
  pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(charWriteUUID);
  if (pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charWriteUUID.toString().c_str());
    bluetoothDisconnect();
    return false;
  }
  Serial.println(" - Found our characteristic for writing");

  if(pRemoteCharacteristicRead->canNotify()) {
    Serial.println("Try to register for notification");
    pRemoteCharacteristicRead->registerForNotify(notifyCallback);
    Serial.println("pRemoteCharacteristicRead: notification registered");
  } else {
    Serial.println("pRemoteCharacteristicRead cannot be notified");
  }

  Serial.println(" - Registered for Notification");

  //connected = true;
  return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.getName().rfind(scanName, 0) == 0 && advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(scanServiceUuid)) {
      BLEDevice::getScan()->stop();
      //myDevice = new BLEAdvertisedDevice(advertisedDevice);
      //doConnect = true;
      doScan = true;
      Serial.println("Device found");
      countDevices++;
      myDeviceAddresses[scanDeviceIndex] = advertisedDevice.getAddress().toString().c_str();
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void bluetoothSetupBluetoothBle() {
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  // create and configure BLE client
  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
}

bool bluetoothScan(byte deviceIndex, char* name, BLEUUID serviceUuid) {
  if (deviceIndex < 0 || deviceIndex > sizeof(myDeviceAddresses) - 1) {
    Serial.println("Device index invalid");
    return false;
  }
  scanDeviceIndex = deviceIndex;
  scanName = name;
  scanServiceUuid = serviceUuid;
  Serial.println("Start scan...");
  pBLEScan->start(20, false); // scan for 20 sec
  Serial.println("Scan finished");
  
  return myDeviceAddresses[scanDeviceIndex].length() > 0;
}

bool bluetoothIsConnected() {
  //return connected;
  if (pClient != NULL) {
    return pClient->isConnected();
  } else {
    return false;
  }
}

/*void bluetoothTryConnect(byte deviceIndex, BLEUUID charReadUUID, BLEUUID charWriteUUID) {
  Serial.println("bluetoothTryConnect()");
  if (countDevices > 0 && currentDeviceNo < countDevices && !bluetoothIsConnected()) {
    if (connectToServer(deviceIndex, charReadUUID, charWriteUUID)) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    //doConnect = false;
  }
}*/

void bluetoothDisconnect() {
  if (bluetoothIsConnected()) {
    Serial.print("Disconnecting from: ");
    Serial.println(charWriteUUID.toString().c_str());
    /*if (pRemoteCharacteristicRead != NULL) {
      pRemoteCharacteristicRead->registerForNotify(NULL);
    }*/
    pClient->disconnect();
    //delete pRemoteCharacteristicRead;
    //delete pRemoteCharacteristicWrite;
    pRemoteCharacteristicRead = NULL;
    pRemoteCharacteristicWrite = NULL;
    //connected = false;
    Serial.println("Disconnected");
  }
}

void bluetoothSendByteArray(byte *buffer, int dataLength) {
  //Serial.print("Sending: ");
  //hexutilPrintByteArrayInHex(buffer, dataLength);
  int sentBytes = 0;
  int chunkSize = BLEDevice::getMTU() - 3;
  byte tmpBuffer[chunkSize];
  while (sentBytes < dataLength) {
    int countBytes = (dataLength - sentBytes) > chunkSize ? chunkSize : (dataLength - sentBytes);
    for (int i = 0; i < countBytes; i++) {
      tmpBuffer[i] = buffer[sentBytes + i];
    }
    pRemoteCharacteristicWrite->writeValue(tmpBuffer, countBytes); 
    //pRemoteCharacteristicWrite->notify();
    sentBytes = sentBytes + countBytes;
  }
}

std::string bluetoothReadData() {
  std::string value = pRemoteCharacteristicRead->readValue();
  return value;
}
