// Command to read current brutto: READ
// Format of answer: 16bit brutto weight in gram (big endian)

#define RECEIVE_BUFFER_SIZE 10

byte scaleReceiveBuffer[RECEIVE_BUFFER_SIZE];

const char COMMAND_READ_WEIGHT[] = "READ";
const int SIZE_CURRENT_WEIGHT = sizeof(ScaleCurrentWeight);
const int MAX_NETTO_WEIGHT = 11000;
const int TARA_WEIGHT_GRAM = 5400;

void scaleutilUpdateGasData(ScaleCurrentWeight *scaleCurrentWeight) {   
  _gasData.nettoWeightGram = scaleCurrentWeight->currentBruttoGram - TARA_WEIGHT_GRAM;
  _gasData.fillingLevelPercent = (_gasData.nettoWeightGram * 100) / MAX_NETTO_WEIGHT;
  _gasData.usagePerDayGram = 715; // TODO: calculate by difference measurements
  _gasData.remainingDays = _gasData.nettoWeightGram / _gasData.usagePerDayGram;
}

void scaleutilResetReceiveBuffer() {
  memset(scaleReceiveBuffer, 0, sizeof(scaleReceiveBuffer));
}

// swap endians of int16_t (starting from beginning)
void scaleutilSwapBytesEndian(byte *buffer, int size) {
  byte tmpValue;
  for (int i = 0; i < 1; i++) {
    tmpValue = buffer[2*i];
    buffer[2*i] = buffer[2*i + 1];
    buffer[2*i + 1] = tmpValue;
  }
}

bool scaleutilDataReceived(byte *pData, size_t length) {
  if (length == 0) {
    // nothing to do
    return false;
  }

  Serial.print("Num bytes received: ");
  Serial.println(length);

  for (int i = 0; i < length; i++) {
    scaleReceiveBuffer[i] = pData[i];
  }

  ScaleCurrentWeight scaleCurrentWeight = scaleutilScaleCurrentWeight(scaleReceiveBuffer, length);
  Serial.println("ScaleCurrentWeight created");
  scaleConnectionSuccessful = true;
  
  scaleutilUpdateGasData(&scaleCurrentWeight);
  
  displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData);
  Serial.println("ScaleCurrentWeight drawed");
  return true;
}

ScaleCurrentWeight scaleutilScaleCurrentWeight(byte *buffer, int size) {
  // use copy of buffer to not change original buffer for endian conversion
  byte tmpBuffer[size];
  memcpy(tmpBuffer, buffer, size);

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  scaleutilSwapBytesEndian(tmpBuffer, size);

  // copy tmpBuffer to struct
  ScaleCurrentWeight result;
  memcpy(&result, tmpBuffer, sizeof(result));  
  return result;
}

/*void scaleutilWriteScaleCurrentWeightToBuffer(byte *buffer, int size, ScaleCurrentWeight *scaleCurrentWeight) {
  if (size < sizeof(ScaleCurrentWeight)) {
    Serial.println("scaleutilWriteScaleCurrentWeightToBuffer: buffer too small");
    return;
  }
  memcpy(buffer, scaleCurrentWeight, sizeof(ScaleCurrentWeight));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  scaleutilSwapBytesEndian(buffer, sizeof(ScaleCurrentWeight));
}*/

void scaleutilPrintScaleCurrentWeight(ScaleCurrentWeight *scaleCurrentWeight) {
  Serial.print("Brutto: ");
  Serial.println(scaleCurrentWeight->currentBruttoGram);
}

/**
 * Send command to get ScaleCurrentWeight data (non blocking)
 */
void scaleutilSendCommandScaleCurrentWeightAsync() {
  Serial.println("Sending Command for ScaleCurrentWeight"); 
  int size =  sizeof(COMMAND_READ_WEIGHT);
  byte buffer[size];
  for (int i = 0; i < size; i++) {
    buffer[i] = (byte) COMMAND_READ_WEIGHT[i];
  }
  bluetoothDataReceived = false;
  bluetoothSendByteArray(buffer, sizeof(buffer));
}
