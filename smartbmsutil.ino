// General format (big-endian):
// D203        
// HEADER_READ CONTENT_LENGTH CONTENT CRC
// <-----------strGoodData-------------->
// <-----------CRC relevant--------->
// Content length (byte): 124 -> Run Info
// Content length (byte): 32  -> Run Info Last Battery Value
// Content length (byte): 82  -> Set Info
// Content length (byte):buffer 64  -> Version
// Content length (byte): 6   -> PWD

// Format send by Smart BMS APP:
// D203        0000    003E       D7B9
// HEADER_READ LOC No  MaxLength  CRC

#define RECEIVE_BUFFER_SIZE 500
#define READ_PACKET_HEADER_LENGTH 3 // the number of bytes for header of read packets (header, content length)
#define READ_PACKET_CRC_LENGTH 2 // the number of bytes for CRC of read packets
#define READ_PACKET_OVERHEAD_LENGTH  READ_PACKET_HEADER_LENGTH + READ_PACKET_CRC_LENGTH // the number of bytes for overhead (header, content length, CRC)
#define READ_RETRIES 5 // the number of retries until reading value is aborted

byte smartBmsReceiveBuffer[RECEIVE_BUFFER_SIZE];
uint16_t indexSmartBmsReceiveBuffer = 0;

const char COMMAND_RUN_INFO[] = {0xD2, 0x03, 0x00, 0x00, 0x00, 0x3E, 0xD7, 0xB9};
const char COMMAND_SET_DATA_INFO[] = {0xD2, 0x03, 0x00, 0x80, 0x00, 0x29, 0x96, 0x5F};

const int SIZE_RUN_INFO = sizeof(SmartbmsutilRunInfo);

void smartbmsutilResetReceiveBuffer() {
  memset(smartBmsReceiveBuffer, 0, sizeof(smartBmsReceiveBuffer));
  indexSmartBmsReceiveBuffer = 0;
}

void smartbmsutilGetCRC(byte *crcArray, byte *sourceByteArray, int crcRelevantDataLength) {
  int CRC = 65535;
  for (int i = 0; i < crcRelevantDataLength; i++) {
    CRC ^= sourceByteArray[i] & 255;
    for (int j = 0; j < 8; j++) {
      if ((CRC & 1) != 0) {
        CRC = (CRC >> 1) ^ 40961;
      } else {
        CRC >>= 1;
      }
    }
  }
  CRC = ((65280 & CRC) >> 8) | ((CRC & 255) << 8);

  crcArray[0] = byte(CRC >> 8);
  crcArray[1] = byte(CRC & 0x00FF);
}

// returns true if CRC is OK
bool smartbmsutilCheckCrc(byte *buffer, int size) {
  int contentLength = buffer[2];
  if (size < READ_PACKET_OVERHEAD_LENGTH + contentLength) {
    Serial.print("smartbmsutilCheckCrc: Array length too small: ");
    Serial.println(size);
    return false;
  }
  
  byte crcBuffer[2];
  smartbmsutilGetCRC(crcBuffer, buffer, READ_PACKET_HEADER_LENGTH + contentLength);
  if (crcBuffer[0] != buffer[READ_PACKET_HEADER_LENGTH + contentLength] || crcBuffer[1] != buffer[READ_PACKET_HEADER_LENGTH + contentLength + 1]) {
    Serial.println("smartbmsutilCheckCrc: CRC does not match");
    return false;
  }
  return true;
}

// return true if packet is valid (full packet with correct header, correct crc)
bool smartbmsutilIsValidPacket(byte *buffer, int size) {
  if (size < READ_PACKET_HEADER_LENGTH) {
    Serial.print("smartbmsutilIsValidPacket: Array length too small: ");
    Serial.println(size);
    return false;
  }
  if (buffer[0] != 0xD2) {
    return false;
  }
  if (buffer[1] != 0x03) {
    return false;
  }
  return smartbmsutilCheckCrc(buffer, size);
}

// swap endians of int16_t (starting from 4th byte)
void smartbmsutilSwapBmsBytesEndian(byte *buffer, int size) {
  byte tmpValue;
  for (int i = 0; i < ((size - READ_PACKET_HEADER_LENGTH) / 2); i++) {
    tmpValue = buffer[READ_PACKET_HEADER_LENGTH + 2*i];
    buffer[READ_PACKET_HEADER_LENGTH + 2*i] = buffer[READ_PACKET_HEADER_LENGTH + 2*i + 1];
    buffer[READ_PACKET_HEADER_LENGTH + 2*i + 1] = tmpValue;
  }
}

bool smartbmsutilDataReceived(byte *pData, size_t length) {
  if (length == 0) {
    // nothing to do
    return false;
  }

  Serial.print("Received: ");
  hexutilPrintByteArrayInHex(pData, length);

  Serial.print("Num bytes received: ");
  Serial.println(length);
  
  if (indexSmartBmsReceiveBuffer + length > RECEIVE_BUFFER_SIZE) {
    Serial.println("smartBmsReceiveBuffer too small. Resetting buffer.");
    smartbmsutilResetReceiveBuffer();
    return false;
  }

  for (int i = 0; i < length; i++) {
    smartBmsReceiveBuffer[indexSmartBmsReceiveBuffer] = pData[i];
    indexSmartBmsReceiveBuffer++;
  }

  bool isReadHeader = smartBmsReceiveBuffer[0] == 0xD2 && smartBmsReceiveBuffer[1] == 0x03;

  if (isReadHeader) {
    int contentLength = smartBmsReceiveBuffer[2];
    Serial.print("Content length: ");
    Serial.println(contentLength);
    if (contentLength + READ_PACKET_OVERHEAD_LENGTH == indexSmartBmsReceiveBuffer) {
      // packet is complete
      Serial.println("Packet complete");
      if (smartbmsutilIsValidPacket(smartBmsReceiveBuffer, indexSmartBmsReceiveBuffer)) {
        // packet is valid
        Serial.println("Packet valid");
        if (smartBmsReceiveBuffer[2] == 0x7C) {
          // Packet is RunInfo (content length = 0x7C)
          Serial.println("Packet is RunInfo");
          _currentSmartbmsutilRunInfo = smartbmsutilGetRunInfo(smartBmsReceiveBuffer, indexSmartBmsReceiveBuffer);
          Serial.println("SmartbmsutilRunInfo created");
          bmsConnectionSuccessful = true;
          /*displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData);
          Serial.println("SmartbmsutilRunInfo drawed");*/
          //smartbmsutilPrintRunInfo(&runInfo);

          // reset smartBmsReceiveBuffer
          smartbmsutilResetReceiveBuffer();
          return true;
        }
      } else {
        Serial.println("Packet is invalid");
        // reset smartBmsReceiveBuffer
        smartbmsutilResetReceiveBuffer();
      }
    }
  }
  return false;
}

// this method assumes that packet is valid (CRC checked)
SmartbmsutilRunInfo smartbmsutilGetRunInfo(byte *buffer, int size) {
  // use copy of buffer to not change original buffer for endian conversion
  byte tmpBuffer[size];
  memcpy(tmpBuffer, buffer, size);

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(tmpBuffer, size - 2);

  // copy tmpBuffer to struct
  SmartbmsutilRunInfo result;
  memcpy(&result, tmpBuffer, sizeof(result));  
  return result;
}

void smartbmsutilWriteSmartbmsutilRunInfoToBuffer(byte *buffer, int size, SmartbmsutilRunInfo *smartbmsutilRunInfo) {
  if (size < sizeof(SmartbmsutilRunInfo)) {
    Serial.println("smartbmsutilWriteSmartbmsutilRunInfoToBuffer: buffer too small");
    return;
  }
  memcpy(buffer, smartbmsutilRunInfo, sizeof(SmartbmsutilRunInfo));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfo) - 2);
}

void smartbmsutilWriteSmartbmsutilVersionInfoToBuffer(byte *buffer, int size, SmartbmsutilVersionInfo *smartbmsutilVersionInfo) {
  if (size < sizeof(SmartbmsutilVersionInfo)) {
    Serial.println("smartbmsutilWriteSmartbmsutilVersionInfoToBuffer: buffer too small");
    return;
  }
  memcpy(buffer, smartbmsutilVersionInfo, sizeof(SmartbmsutilVersionInfo));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  //smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilVersionInfo));
}

void smartbmsutilWriteSmartbmsutilRunInfoLastBatteryValueToBuffer(byte *buffer, int size, SmartbmsutilRunInfoLastBatteryValue *smartbmsutilRunInfoLastBatteryValue) {
  if (size < sizeof(SmartbmsutilRunInfoLastBatteryValue)) {
    Serial.println("smartbmsutilWriteSmartbmsutilRunInfoLastBatteryValueToBuffer: buffer too small");
    return;
  }
  memcpy(buffer, smartbmsutilRunInfoLastBatteryValue, sizeof(SmartbmsutilRunInfoLastBatteryValue));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfoLastBatteryValue) - 2);
}

void smartbmsutilWriteSmartbmsutilSetDataInfoToBuffer(byte *buffer, int size, SmartbmsutilSetDataInfo *smartbmsutilSetDataInfo) {
  if (size < sizeof(SmartbmsutilSetDataInfo)) {
    Serial.println("smartbmsutilWriteSmartbmsutilSetDataInfo: buffer too small");
    return;
  }
  memcpy(buffer, smartbmsutilSetDataInfo, sizeof(SmartbmsutilSetDataInfo));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilSetDataInfo) - 2);
}


void smartbmsutilPrintRunInfo(SmartbmsutilRunInfo *runInfo) {
  Serial.print("Battery voltages: ");
  for (int i = 0; i < runInfo->countBatteryVoltages; i++) {
    Serial.print(runInfo->batteryVoltages[i] / 1000.0, 3);
    Serial.print("V ");
  }
  Serial.println();

  Serial.print("Battery temps: ");
  for (int i = 0; i < runInfo->countBatteryTemp; i++) {
    Serial.print(runInfo->batteryTemp[i] - 40);
    Serial.print("°C ");
  }
  Serial.println();

  Serial.print("Max Cell voltage: ");
  Serial.println(runInfo->maxCellVoltage / 1000.0, 3);

  Serial.print("Min Cell voltage: ");
  Serial.println(runInfo->minCellVoltage / 1000.0, 3);

  Serial.print("Current voltage: ");
  Serial.println(runInfo->currentV / 10.0);

  Serial.print("Current A: ");
  Serial.println((runInfo->currentA - 30000) / 10.0);

  Serial.print("Current KW: ");
  Serial.println(runInfo->currentKw / 1000.0, 3);

  Serial.print("SOC: : ");
  Serial.println(runInfo->chargePercent / 10.0, 1);
}

bool smartbmsutilArrayEquals(const char *buffer1, const char *buffer2, int size) {
  for (int i = 0; i < size; i++) {
    if (buffer1[i] != buffer2[i]) {
      return false;
    }
  }
  return true; // the arrays are equal up to index (size - 1)
}

bool smartbmsutilIsCommandRunInfo(const char *buffer, int size) {
  if (size < sizeof(COMMAND_RUN_INFO)) {
    return false;
  }
  return smartbmsutilArrayEquals(buffer, COMMAND_RUN_INFO, sizeof(COMMAND_RUN_INFO));
}

bool smartbmsutilIsCommandSetDataInfo(const char *buffer, int size) {
  if (size < sizeof(COMMAND_SET_DATA_INFO)) {
    return false;
  }
  return smartbmsutilArrayEquals(buffer, COMMAND_SET_DATA_INFO, sizeof(COMMAND_SET_DATA_INFO));
}

bool smartbmsutilHasAlarmSet(SmartbmsutilRunInfo *runInfo) {
  return runInfo->alarm1 != 0 || runInfo->alarm2 != 0 || runInfo->alarm3 != 0 || runInfo->alarm4 != 0;
}

/**
 * Send command to get RunInfo data (non blocking)
 */
void smartbmsutilSendCommandRunInfoAsync() {
  Serial.println("Sending Command for RunInfo"); 
  int size =  sizeof(COMMAND_RUN_INFO);
  byte buffer[size];
  for (int i = 0; i < size; i++) {
    buffer[i] = (byte) COMMAND_RUN_INFO[i];
  }
  bluetoothDataReceived = false;
  bluetoothSendByteArray(buffer, sizeof(buffer));
}

/**
 * Send command to get RunInfo data and read the result (blocking).
 * Does not work with real BMS!!!
 */
boolean smartbmsutilReadRunInfo(byte *buffer, int size) {  
  // send command to read Run Info
  smartbmsutilSendCommandRunInfoAsync();

  // read result
  int numRead = 0;
  int count = 0;
  std::string rxValue;
  while (count < READ_RETRIES && numRead != SIZE_RUN_INFO) {
    delay(500);
    rxValue = bluetoothReadData();
    numRead = rxValue.length();
    Serial.print("numRead = ");
    Serial.println(numRead);
    hexutilPrintByteArrayInHex(buffer, size);
    count++;
  }

  if (numRead == SIZE_RUN_INFO) {
    if (size < numRead) {
      Serial.print("Buffer too small, needed size: ");
      Serial.println(numRead);
      return false;
    }
    // copy to buffer as length is correct
    for (int i = 0; i < numRead; i++) {
      buffer[i] = rxValue[i];
    }
    return true;
  } else {
    Serial.println("Unable to read correct RunInfo");
    return false;
  }
}
