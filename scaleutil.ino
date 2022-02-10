// Command to read current brutto: READ
// Format of answer: 16bit brutto weight in gram (big endian)

#define RECEIVE_BUFFER_SIZE 10
#define USAGE_BUFFER_SIZE 100

byte scaleReceiveBuffer[RECEIVE_BUFFER_SIZE];
int16_t usagePerDayGramBuffer[USAGE_BUFFER_SIZE] = {0};
uint8_t usagePerDayGramBufferCount = 0;
uint8_t usagePerDayGramBufferIndex = 0;

const char COMMAND_READ_WEIGHT[] = "READ";
const int SIZE_CURRENT_WEIGHT = sizeof(ScaleCurrentWeight);
const int MAX_NETTO_WEIGHT = 11000;
const int TARA_WEIGHT_GRAM = 5400;
const unsigned int SECONDS_PER_DAY = 24 * 60 * 60;

void scaleutilAddUsagePerDayGramBuffer(int16_t value) {
  // ring buffer
  usagePerDayGramBuffer[usagePerDayGramBufferIndex] = value;
  usagePerDayGramBufferIndex++;
  if (usagePerDayGramBufferIndex == USAGE_BUFFER_SIZE) {
    usagePerDayGramBufferIndex = 0;
    
  }
  if (usagePerDayGramBufferCount < USAGE_BUFFER_SIZE) {
    usagePerDayGramBufferCount++;
  }
}

int16_t scaleutilGetMedianUsagePerDayGram() {
  if (usagePerDayGramBufferCount == 0) {
    return 0;
  }
  uint32_t sum = 0;
  for (int i = 0; i < usagePerDayGramBufferCount; i++) {
    sum = sum + usagePerDayGramBuffer[i];
  }
  return sum / usagePerDayGramBufferCount;
}

void scaleutilUpdateGasData(ScaleCurrentWeight *scaleCurrentWeight) {   
  int oldNettoWeightGram = _gasData.nettoWeightGram;
  
  _gasData.nettoWeightGram = scaleCurrentWeight->currentBruttoGram - TARA_WEIGHT_GRAM;
  _gasData.fillingLevelPercent = (_gasData.nettoWeightGram * 100) / MAX_NETTO_WEIGHT;

  // calculate gas usage between current and last measurement. Use median of last measurements
  if (lastMillisMeasuredScale > 0) {
    // this is not the first measurement (at least 2 measurements needed for calculating gas usage)
    int measuredIntervalSeconds = (millis() - lastMillisMeasuredScale) / 1000;
    if (measuredIntervalSeconds > 0) {
      int usedGasGram = oldNettoWeightGram - _gasData.nettoWeightGram;
      if (usedGasGram < 0)  {
        usedGasGram = 0;
      }
      int16_t currentUsagePerDayGram = (usedGasGram * SECONDS_PER_DAY) / measuredIntervalSeconds;
      if (currentUsagePerDayGram > 0) {
        scaleutilAddUsagePerDayGramBuffer(currentUsagePerDayGram);
      }
      _gasData.usagePerDayGram = scaleutilGetMedianUsagePerDayGram();
      if (_gasData.usagePerDayGram > 0) {
        _gasData.remainingDays =  (float)_gasData.nettoWeightGram / (float)_gasData.usagePerDayGram;
      } else {
        // with usage of 0 gram, remaining days cannot be calculated
        _gasData.remainingDays = -1;
      }
    } else {
      Serial.print("ERROR: scaleutilUpdateGasData: measuredIntervalSeconds = ");
      Serial.println(measuredIntervalSeconds);
      Serial.println("No Gas data updated");
    }
  } else {
    // first measurement: with usage of 0 gram, remaining days cannot be calculated
    _gasData.remainingDays = -1;
  }
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
  
  /*displayDrawBmsAndGasOverview(&_currentSmartbmsutilRunInfo, &_gasData);
  Serial.println("ScaleCurrentWeight drawed");*/
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
