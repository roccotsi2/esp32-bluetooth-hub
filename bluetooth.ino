//static boolean doConnect = false;
//static boolean connected = false;
//static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristicReadBms;
static BLERemoteCharacteristic* pRemoteCharacteristicWriteBms;
static BLERemoteCharacteristic* pRemoteCharacteristicReadScale;
static BLERemoteCharacteristic* pRemoteCharacteristicWriteScale;
//static BLEAdvertisedDevice* myDevice;
static std::string myDeviceAddresses[2];
static BLEScan* pBLEScan;
static BLEClient* pClientBms;
static BLEClient* pClientScale;
byte currentDeviceNo = 0; // 0 = BMS, 1 = Scale
byte countDevices = 0;
byte scanDeviceIndex;
char* scanName;
BLEUUID scanServiceUuid;

bool scanRunning;

BLEClient* bluetoothGetClient(byte deviceIndex) {
  if (deviceIndex == 0) {
    return pClientBms;
  } else if (deviceIndex == 1) {
    return pClientScale;
  } else {
    Serial.print("ERROR: invalid deviceIndex: ");
    Serial.println(deviceIndex);
    return NULL;
  }
}

BLERemoteCharacteristic* bluetoothGetCharacteristic(byte deviceIndex, boolean read) {
  if (deviceIndex == 0) {
    // BMS
    if (read) {
      return pRemoteCharacteristicReadBms;
    } else  {
      return pRemoteCharacteristicWriteBms;
    }
  } else if (deviceIndex == 1) {
    // Scale
    if (read) {
      return pRemoteCharacteristicReadScale;
    } else  {
      return pRemoteCharacteristicWriteScale;
    }
  } else {
    Serial.print("ERROR bluetoothGetCharacteristic: invalid deviceIndex: ");
    Serial.println(deviceIndex);
    return NULL;
  }
}

void bluetoothSetCharacteristic(byte deviceIndex, boolean read, BLERemoteCharacteristic* characteristic) {
  if (deviceIndex == 0) {
    // BMS
    if (read) {
      pRemoteCharacteristicReadBms = characteristic;
    } else  {
      pRemoteCharacteristicWriteBms = characteristic;
    }
  } else if (deviceIndex == 1) {
    // Scale
    if (read) {
      pRemoteCharacteristicReadScale = characteristic;
    } else  {
      pRemoteCharacteristicWriteScale = characteristic;
    }
  } else {
    Serial.print("ERROR bluetoothSetCharacteristic: invalid deviceIndex: ");
    Serial.println(deviceIndex);
  }
}

static void notifyCallbackBms(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (bluetoothDataReceived == true) {
    // nothing to do, as we already received the answer (sometimes there are two notifications)
    Serial.println("Received BMS notification twice. Skipped");
    return;
  }
  
  if (smartbmsutilDataReceived(pData, length)) {
    // disconnect after data was successful read
    bluetoothDisconnect();
    delay(200); // wait to be sure that connection is correctly closed
    bluetoothDataReceived = true;
  }
}

static void notifyCallbackScale(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (bluetoothDataReceived == true) {
    // nothing to do, as we already received the answer (sometimes there are two notifications)
    Serial.println("Received Scale notification twice. Skipped");
    return;
  }
  
  if (scaleutilDataReceived(pData, length)) {
    lastMillisMeasuredScale = millis(); // update last measurement for calculation of gas usage
    
    // disconnect after data was successful read
    bluetoothDisconnect();
    delay(200); // wait to be sure that connection is correctly closed
    bluetoothDataReceived = true;
  }
}

class MyClientCallbackBms : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connection established: BMS");
  }

  void onDisconnect(BLEClient* pclient) {
    //connected = false;
    Serial.println("Connection lost: BMS");    
  }
};

class MyClientCallbackScale : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connection established: Scale");
  }

  void onDisconnect(BLEClient* pclient) {
    //connected = false;
    Serial.println("Connection lost: Scale");    
  }
};

/*void bluetoothListServices() {
  std::map<std::string, BLERemoteService*> *pRemoteServices = pClient->getServices();

  if (pRemoteServices == nullptr) {
    Serial.println("Failed to find services");
    return;
  }
  
  for (auto it=pRemoteServices->begin(); it!=pRemoteServices->end(); ++it) {
    Serial.println(it->first.c_str());
  }
}*/

bool bluetoothConnectToServer(byte deviceIndex, BLEUUID serviceUUID, BLEUUID charReadUUID, BLEUUID charWriteUUID) {
  currentDeviceNo = deviceIndex;
  BLEClient* pClient = bluetoothGetClient(currentDeviceNo);
 
  Serial.print("Forming a connection to ");
  Serial.println(myDeviceAddresses[currentDeviceNo].c_str());

  // Connect to the remote BLE Server.
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
  BLERemoteCharacteristic* pRemoteCharacteristicReadLocal = pRemoteService->getCharacteristic(charReadUUID);
  bluetoothSetCharacteristic(currentDeviceNo, true, pRemoteCharacteristicReadLocal); 
  if (pRemoteCharacteristicReadLocal == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charReadUUID.toString().c_str());
    bluetoothDisconnect();
    return false;
  }
  Serial.println(" - Found our characteristic for reading");

  // Obtain a reference to the characteristic for writing in the service of the remote BLE server.
  BLERemoteCharacteristic* pRemoteCharacteristicWriteLocal = pRemoteService->getCharacteristic(charWriteUUID);
  bluetoothSetCharacteristic(currentDeviceNo, false, pRemoteCharacteristicWriteLocal); 
  if (pRemoteCharacteristicWriteLocal == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charWriteUUID.toString().c_str());
    bluetoothDisconnect();
    return false;
  }
  Serial.println(" - Found our characteristic for writing");

  if(pRemoteCharacteristicReadLocal->canNotify()) {
    Serial.println("Try to register for notification");
    if (currentDeviceNo == 0) {
      pRemoteCharacteristicReadLocal->registerForNotify(notifyCallbackBms);
    } else if (currentDeviceNo == 1) {
      pRemoteCharacteristicReadLocal->registerForNotify(notifyCallbackScale);
    }
    Serial.println("pRemoteCharacteristicRead: notification registered");
  } else {
    Serial.println("pRemoteCharacteristicRead cannot be notified");
    bluetoothDisconnect();
    return false;
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
    // TODO: without these prints the scale is not detected. Needs a fix
    Serial.print("advertisedDevice.getName() = ");
    Serial.println(advertisedDevice.getName().c_str());
    Serial.print("scanName = ");
    Serial.println(scanName);
    Serial.print("advertisedDevice.getName().rfind(scanName, 0) = ");
    Serial.println(advertisedDevice.getName().rfind(scanName, 0));
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.getName().rfind(scanName, 0) == 0 && advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(scanServiceUuid)) {
      //BLEDevice::getScan()->stop();
      //myDevice = new BLEAdvertisedDevice(advertisedDevice);
      //doConnect = true;
      //doScan = true;
      countDevices++;
      //myDeviceAddresses[scanDeviceIndex] = advertisedDevice.getAddress().toString().c_str();
      if (foundBluetoothDevices < 10) {
        foundBluetoothDevices++;
        strcpy(foundBluetoothAddresses[foundBluetoothDevices-1], advertisedDevice.getAddress().toString().c_str());
        strcpy(foundBluetoothNames[foundBluetoothDevices-1], advertisedDevice.getName().c_str());
      }
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void bluetoothSetupBluetoothBle() {
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  // create and configure BLE clients
  pClientBms  = BLEDevice::createClient();
  pClientBms->setClientCallbacks(new MyClientCallbackBms());
  pClientScale  = BLEDevice::createClient();
  pClientScale->setClientCallbacks(new MyClientCallbackScale());
}

bool bluetoothScan(byte deviceIndex, char* name, BLEUUID serviceUuid) {
  if (deviceIndex < 0 || deviceIndex > sizeof(myDeviceAddresses) - 1) {
    Serial.println("Device index invalid");
    return false;
  }
  if (scanRunning) {
    Serial.println("Scan is already running");
    return false;
  }

  scanRunning = true;

  // reset the found device adresses + names
  for (int i = 0; i < 10; i++) {
    memset(foundBluetoothAddresses[i], 0, 20);
    memset(foundBluetoothNames[i], 0, 20);
  }
  foundBluetoothDevices = 0;
  
  scanDeviceIndex = deviceIndex;
  scanName = name;
  scanServiceUuid = serviceUuid;
  Serial.println("Start scan...");
  pBLEScan->start(20, false); // scan for 20 sec
  Serial.println("Scan finished");
  scanRunning = false;
  
  //return myDeviceAddresses[scanDeviceIndex].length() > 0;
  return true;
}

bool bluetoothIsConnected() {
  BLEClient* pClient = bluetoothGetClient(currentDeviceNo);
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
  BLEClient* pClient = bluetoothGetClient(currentDeviceNo);
  if (bluetoothIsConnected()) {
    Serial.print("Disconnecting from: ");
    Serial.println(myDeviceAddresses[currentDeviceNo].c_str());
    /*if (pRemoteCharacteristicRead != NULL) {
      pRemoteCharacteristicRead->registerForNotify(NULL);
    }*/
    pClient->disconnect();
    bluetoothSetCharacteristic(currentDeviceNo, true, NULL);
    bluetoothSetCharacteristic(currentDeviceNo, false, NULL);
    //connected = false;
    Serial.println("Disconnected");
  }
}

void bluetoothSendByteArray(byte *buffer, int dataLength) {
  BLERemoteCharacteristic* charWrite = bluetoothGetCharacteristic(currentDeviceNo, false);
  
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
    charWrite->writeValue(tmpBuffer, countBytes); 
    //pRemoteCharacteristicWrite->notify();
    sentBytes = sentBytes + countBytes;
  }
}

std::string bluetoothReadData() {
  BLERemoteCharacteristic* charRead = bluetoothGetCharacteristic(currentDeviceNo, true);
  std::string value = charRead->readValue();
  return value;
}

bool bluetoothIsScanRunning() {
  return scanRunning;
}

void bluetoothSetDeviceAddress(byte deviceIndex, char *address) {
  myDeviceAddresses[deviceIndex] = address;
  Serial.print("Device address set: ");
  Serial.println(address);
}
