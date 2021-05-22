#define READ_PACKET_OVERHEAD_LENGTH 5

// define simulation constants
#define UPDATE_INTERVAL_MILLIS 5000
#define COUNT_BATTERIES 4
#define MIN_CELL_VOLTAGE 4127
#define MIN_CURRENT_VOLT 12
#define MIN_CURRENT_AMP 1
#define MAX_CURRENT_VOLT 14
#define MAX_CURRENT_AMP 100
#define STEPS_VOLT 1
#define STEPS_AMP 1

void smartbmsdemoFillSmartbmsutilRunInfo(SmartbmsutilRunInfo *currentSmartbmsutilRunInfo) {
  int currentV = 12; 
  int currentA = 14; 
  
  currentSmartbmsutilRunInfo->header1 = 0xD2;
  currentSmartbmsutilRunInfo->header2 = 0x03;
  currentSmartbmsutilRunInfo->contentLength = sizeof(SmartbmsutilRunInfo) - READ_PACKET_OVERHEAD_LENGTH;

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 32; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      currentSmartbmsutilRunInfo->batteryVoltages[i] = batteryMilliVolt;
    } else {
      currentSmartbmsutilRunInfo->batteryVoltages[i] = 0;
    }
    batteryMilliVolt = batteryMilliVolt + 10;
  }

  int batteryTemperature = 60;
  for (int i = 0; i < 8; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      currentSmartbmsutilRunInfo->batteryTemp[i] = batteryTemperature;
      batteryTemperature++;
    } else {
      currentSmartbmsutilRunInfo->batteryTemp[i] = 0;
    }
  }

  if ((currentV + STEPS_VOLT) > MAX_CURRENT_VOLT) {
    currentV = MIN_CURRENT_VOLT;
  } else {
    currentV = currentV + STEPS_VOLT;
  }

  if ((currentA + STEPS_AMP) > MAX_CURRENT_AMP) {
    currentA = MIN_CURRENT_AMP;
  } else {
    currentA = currentA + STEPS_AMP;
  }
 
  int currentKw = currentV * currentA;
  int maxCellVoltage = MIN_CELL_VOLTAGE + ((COUNT_BATTERIES - 1) * 10);
  int averageCellVoltage = (MIN_CELL_VOLTAGE + maxCellVoltage) / 2;

  currentSmartbmsutilRunInfo->currentV = currentV * 10; // current V (in 0,1V)
  currentSmartbmsutilRunInfo->currentA = 30000 + (currentA * 10); // current A (offset: 30000 in 0,1A)
  currentSmartbmsutilRunInfo->chargePercent = 775; // State of charge in percent (in 0,1 percent)
  currentSmartbmsutilRunInfo->maxCellVoltage = maxCellVoltage; // max cell voltage (in mV)
  currentSmartbmsutilRunInfo->minCellVoltage = MIN_CELL_VOLTAGE; // min cell voltage (in mV)
  currentSmartbmsutilRunInfo->dummy1 = 0;
  currentSmartbmsutilRunInfo->dummy2 = 0;
  currentSmartbmsutilRunInfo->dummy3 = 0;
  currentSmartbmsutilRunInfo->dummy4 = 0;
  currentSmartbmsutilRunInfo->countBatteryVoltages = COUNT_BATTERIES; // count battery volt
  currentSmartbmsutilRunInfo->countBatteryTemp = COUNT_BATTERIES; // count battery temperatures (max-> 8)
  currentSmartbmsutilRunInfo->cycle = 57;
  currentSmartbmsutilRunInfo->jh = 1;
  currentSmartbmsutilRunInfo->cdmos = 0;
  currentSmartbmsutilRunInfo->fdmos = 1;
  currentSmartbmsutilRunInfo->avgVoltage = averageCellVoltage; // average voltage (in mV)
  currentSmartbmsutilRunInfo->diffVoltage = 4000; // differential voltage (in mV)
  currentSmartbmsutilRunInfo->currentKw = currentKw; // current KW (in W)
  currentSmartbmsutilRunInfo->alarm1 = 0;
  currentSmartbmsutilRunInfo->alarm2 = 0;
  currentSmartbmsutilRunInfo->alarm3 = 0;  
  currentSmartbmsutilRunInfo->alarm4 = 0;
  
  byte crcBuffer[2];
  byte buffer[sizeof(SmartbmsutilRunInfo)];
  memcpy(buffer, currentSmartbmsutilRunInfo, sizeof(SmartbmsutilRunInfo));  
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfo) - 2);
  smartbmsutilGetCRC(crcBuffer, buffer, sizeof(SmartbmsutilRunInfo) - 2);
  currentSmartbmsutilRunInfo->crcHigh = crcBuffer[0];
  currentSmartbmsutilRunInfo->crcLow = crcBuffer[1];
}
