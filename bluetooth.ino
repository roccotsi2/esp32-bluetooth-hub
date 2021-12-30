// The remote service we wish to connect to (Smart BMS)
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in for reading
static BLEUUID    charReadUUID("0000fff1-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in for writing
static BLEUUID    charWriteUUID("0000fff2-0000-1000-8000-00805f9b34fb");

//static boolean doConnect = false;
//static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristicRead;
static BLERemoteCharacteristic* pRemoteCharacteristicWrite;
//static BLEAdvertisedDevice* myDevice;
static std::string myDeviceAddresses[2];
static BLEScan* pBLEScan;
static BLEClient* pClient;
int currentDeviceNo = 0;
int countDevices = 0;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (smartbmsutilDataReceived(pData, length)) {
    // disconnect after data was successful read
    bluetoothDisconnect();
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

bool connectToServer() {
  if (countDevices == 0 || currentDeviceNo >= countDevices) {
    return false;
  }
  Serial.print("Forming a connection to ");
  Serial.println(myDeviceAddresses[currentDeviceNo].c_str());

  // Connect to the remove BLE Server.
  int count = 0;
  boolean success = false;
  unsigned long lastMillis = millis();
  while (count < 5 && !success && (millis() - lastMillis < 29000)) { // connection timeout is 30 sec (do not retry if connection timed out)
    delay(100);
    success = pClient->connect(BLEAddress(myDeviceAddresses[currentDeviceNo]));  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    //success = pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
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

/*
  if(pRemoteCharacteristicRead->canNotify()) {
    Serial.println("Try to register for notification");
    pRemoteCharacteristicRead->registerForNotify(notifyCallback);
    Serial.println("pRemoteCharacteristicRead: notification registered");
  } else {
    Serial.println("pRemoteCharacteristicRead cannot be notified");
  }

  Serial.println(" - Registered for Notification");*/

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
    if (advertisedDevice.getName().rfind("DL-", 0) == 0 && advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      //myDevice = new BLEAdvertisedDevice(advertisedDevice);
      //doConnect = true;
      doScan = true;
      Serial.println("Device found");
      countDevices++;
      myDeviceAddresses[countDevices - 1] = advertisedDevice.getAddress().toString().c_str();
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

void bluetoothStartScan() {
  pBLEScan->start(0, false);
}

bool bluetoothIsConnected() {
  //return connected;
  if (pClient != NULL) {
    return pClient->isConnected();
  } else {
    return false;
  }
}

void bluetoothTryConnect() {
  Serial.println("bluetoothTryConnect()");
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (countDevices > 0 && currentDeviceNo < countDevices && !bluetoothIsConnected()) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    //doConnect = false;
  }
}

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
