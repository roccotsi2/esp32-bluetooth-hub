// about 93 pixel per cm

void clearFrameBuffer() {
  memset(frameBuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void displayInit() {
  epd_init();
  frameBuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!frameBuffer) Serial.println("Memory alloc failed!");
  clearFrameBuffer();
}

void updateDisplay() {
  epd_poweron();
  
  epd_clear();
  //epd_clear_area_cycles(epd_full_screen(), 2, 50); // use own parameter for clearings
  
  epd_draw_grayscale_image(epd_full_screen(), frameBuffer);
  
  epd_poweroff_all();
}

GFXfont getFont(int size) {
  if (size == 8) {
    return OpenSans8B;
  } else if (size == 10) {
    return OpenSans10B;
  }  else if (size == 12) {
    return OpenSans12B;
  } else if (size == 18) {
    return OpenSans18B;
  } else if (size == 24) {
    return OpenSans24B;
  } else {
    // fallback
    return OpenSans12B;
  }
}

void getTextWidthAndHeight(int size, char *text, int *width, int *height) {
  int x = 100;
  int y = 100;
  int x1 = 0;
  int y1 = 0;
  GFXfont font = getFont(size);
  get_text_bounds(&font, text, &x, &y, &x1, &y1, width, height, NULL); 
}

void drawString(int size, int x, int y, char *text) {
  GFXfont font = getFont(size);
  write_string(&font, text, &x, &y, frameBuffer);
}

void drawHLine(int x, int y, int length, uint8_t color, int thickness) {
  for (int i = 0; i < thickness; i++) {
    epd_draw_hline(x, y + i, length, color, frameBuffer);
  }
}

void drawVLine(int x, int y, int length, uint8_t color, int thickness) {
  for (int i = 0; i < thickness; i++) {
    epd_draw_vline(x + i, y, length, color, frameBuffer);
  }
}

void drawRect(int x, int y, int width, int height, int thickness) {
  for (int i = 0; i < thickness; i++) {
    epd_draw_rect(x + i, y + i, width - 2*i, height - 2*i, COLOR_BLACK, frameBuffer); 
  }
}

void drawArrow(int xLine, int yLine, int length, bool toRight) {
  if (!toRight) {
    // right to left
    drawHLine(xLine, yLine, length, COLOR_BLACK, 5);
    epd_fill_triangle(xLine - 23, yLine + 2, xLine, yLine - 8, xLine, yLine + 12, COLOR_BLACK, frameBuffer);
  } else {
    // left to right
    drawHLine(xLine, yLine, length, COLOR_BLACK, 5);
    epd_fill_triangle(xLine + 38, yLine + 2, xLine + 15, yLine - 8, xLine + 15, yLine + 12, COLOR_BLACK, frameBuffer);
  }
}

/**
 * Draws a bluetooth symbol
 * @param x: Top left corner x coordinate
 * @param y: Top left corner y coordinate
 * @param height: The height of the symbol
 */
void drawBluetoothSymbol(int x, int y, int height) {
  int width = height / 2;
  int xMiddle = x + width / 2;
  epd_draw_triangle(xMiddle, y, x + width, y + height / 4, xMiddle, y + height / 2, COLOR_BLACK, frameBuffer); // upper triangle
  epd_draw_triangle(xMiddle, y + height / 2, x + width, y + 3 * height / 4, xMiddle, y + height, COLOR_BLACK, frameBuffer); // lower triangle
  epd_draw_line(x, y + height / 4, xMiddle, y + height / 2, COLOR_BLACK, frameBuffer); // upper line
  epd_draw_line(x, y + 3 * height / 4, xMiddle, y + height / 2, COLOR_BLACK, frameBuffer); // lower line
}

void drawProgressBar(int x, int y, int width, int height, int value) {
  if (value > 100) {
    value = 100;
  }
  if (value < 0) {
    value = 0;
  }
  int fillWidth = (width * value) / 100;
  int lineWidth = 2;
  drawRect(x, y, width, height, lineWidth);
  epd_fill_rect(x + lineWidth, y + lineWidth, fillWidth - 2*lineWidth, height - 2*lineWidth, COLOR_BLACK, frameBuffer);
}

void drawCheckbox(int x, int y, int width, int height, bool checked) {
  if (checked) {
    // filled rect
    epd_fill_rect(x, y, width, height, COLOR_BLACK, frameBuffer);
  } else {
    // not filled rect
    drawRect(x, y, width, height, 2);
  }  
}

void drawTextWithCheckbox(int textSize, int x, int y, char *text, bool checked) {
  drawString(textSize, x, y, text);
  int textHeight = 0;
  int textWidth = 0;
  getTextWidthAndHeight(textSize, text, &textWidth, &textHeight);
  drawCheckbox(x + textWidth + 20, y - textHeight + 2, textHeight, textHeight, checked);
}

/**
 * Draw a battery with a text inside
 *
 * @param x x-coordinate of the upper left corner
 * @param y x-coordinate of the upper left corner
 * @param length Length of the battery in pixels
 * @param height Height of the battery in pixels
 * @param text Text to write in the middle of the battery
 */
void drawBattery(int x, int y, int length, int height, char *text) {
  // draw battery (starts from upper left corner and goes clockwise)
  int quarterHeight = height / 4;
  int batteryCapLength = length / 10;
  int lineWidth = 2 + length / 100; // adaptive line with
  drawHLine(x, y, length - batteryCapLength, COLOR_BLACK, lineWidth); // top horizontal line
  drawVLine(x + length - batteryCapLength, y, quarterHeight, COLOR_BLACK, lineWidth); // top right vertical line
  drawHLine(x + length - batteryCapLength, y + quarterHeight, batteryCapLength, COLOR_BLACK, lineWidth); // top right horizontal line
  drawVLine(x + length - lineWidth, y + quarterHeight, height - (quarterHeight) - (quarterHeight), COLOR_BLACK, lineWidth); // middle right vertical line
  drawHLine(x + length - batteryCapLength, y + height - (quarterHeight), batteryCapLength, COLOR_BLACK, lineWidth); // bottom right horizontal line
  drawVLine(x + length - batteryCapLength, y + height - (quarterHeight), quarterHeight + lineWidth, COLOR_BLACK, lineWidth); // bottom right vertical line 
  drawHLine(x, y + height, length - batteryCapLength, COLOR_BLACK, lineWidth); // bottom horizontal line
  drawVLine(x, y, height, COLOR_BLACK, lineWidth); // left vertical line

  // draw the text in the middle of battery (adaptive text size)
  int textSize = 8;
  if (height > 30 && height < 40) {
    textSize = 12;
  } else if (height >= 40 && height < 50) {
    textSize = 12;
  } else if (height >= 50 && height < 60) {
    textSize = 18;
  } else if (height >= 60) {
    textSize = 24;
  }
  int textHeight = 0;
  int textWidth = 0;
  getTextWidthAndHeight(textSize, text, &textWidth, &textHeight);
  //drawString(textSize, x + (length - textWidth) / 2 - batteryCapLength + textWidth/2, y + height/2 + textHeight/2, text);
  drawString(textSize, x + length/2 - textWidth/2, y + height/2 + textHeight/2, text);
}

void drawHeader(char *title) {
  // title
  drawString(24, 0, 37, title);

  // button reset
  drawString(12, 220, 27, "Res");

  // button left
  drawArrow(380, 18, 15, false);

  // button right
  drawArrow(430, 18, 15, true);

  // button select
  drawString(12, 495, 27, "Sel");

  // horizontal line
  drawHLine(0, 40, EPD_WIDTH, COLOR_BLACK, 2);

  // vertical line left from first button
  drawVLine(208, 0, 40, COLOR_BLACK, 2);

  // vertical line right from fifth button
  drawVLine(560, 0, 40, COLOR_BLACK, 2);

  // BMS connection state
  int textHeight = 0;
  int textWidth = 0;
  char textBmsConnection[] = "BMS:";
  getTextWidthAndHeight(12, textBmsConnection, &textWidth, &textHeight);
  drawString(12, 580, 27, textBmsConnection);
  if (bluetoothIsConnected()) {
    drawBluetoothSymbol(580 + textWidth + 10, 5, 30);
  }
}

void drawBmsSectionBorders() {
  // divide in left and right area
  drawVLine(EPD_WIDTH / 2, 40, EPD_HEIGHT - 40, COLOR_BLACK, 2);

  // devide right area in top and bottom area
  drawHLine(EPD_WIDTH / 2, 340, EPD_WIDTH / 2, COLOR_BLACK, 2);
}

void drawBmsOverviewData(SmartbmsutilRunInfo *runInfo) {
  float currentV = runInfo->currentV / 10.0;
  float currentA = (runInfo->currentA - 30000) / 10.0;
  float currentKw = runInfo->currentKw / 1000.0;
  float zMin = round(runInfo->minCellVoltage / 10.0) / 100.0;
  float zMax = round(runInfo->maxCellVoltage / 10.0) / 100.0;
  float zAvg = round(runInfo->avgVoltage / 10.0) / 100.0;
  int textSizeNormal = 18;
  int textSizeSmall = 8;
  char currentVText[10];
  sprintf(currentVText, "%.1f V", currentV);
  char currentAText[10];
  sprintf(currentAText, "%.1f A", currentA);
  char currentKwText[10];
  sprintf(currentKwText, "%.3f kW", currentKw);
  char zMinText[10];
  sprintf(zMinText, "%.2f V", zMin);
  char zMaxText[10];
  sprintf(zMaxText, "%.2f V", zMax);
  char zAvgText[10];
  sprintf(zAvgText, "%.2f V", zAvg);
  char cycleValueText[10];
  sprintf(cycleValueText, "%ld", runInfo->cycle);

  int textNormalHeight = 0;
  int textNormalWidth = 0;
  int textXStart = 5;
  int textYStart = 90;
  int leftMiddleX = EPD_WIDTH / 4;
  getTextWidthAndHeight(textSizeNormal, "Ladung:", &textNormalWidth, &textNormalHeight);
  int heightHeader = 40;
  int numRows = 8;
  int textDistanceVertical = (EPD_HEIGHT - heightHeader - numRows * textNormalHeight) / numRows; // 30;

  drawString(textSizeNormal, textXStart, textYStart, "Ladung:");
  drawProgressBar(leftMiddleX, textYStart - textNormalHeight + 5, leftMiddleX - textXStart, textNormalHeight, runInfo->chargePercent / 10);

  int currentY = textYStart + textNormalHeight + textDistanceVertical;
  drawString(textSizeNormal, textXStart, currentY, "Spannung:");
  drawString(textSizeNormal, leftMiddleX, currentY, currentVText);

  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawString(textSizeNormal, textXStart, currentY, "Strom:");
  drawString(textSizeNormal, leftMiddleX, currentY, currentAText);

  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawString(textSizeNormal, textXStart, currentY, "Leistung:");
  drawString(textSizeNormal, leftMiddleX, currentY, currentKwText);

  char zText[] = "Z";
  int zTextWidth = 0;
  int zTextHeight = 0;
  getTextWidthAndHeight(textSizeNormal, zText, &zTextWidth, &zTextHeight);
  char minText[] = "min";
  int textSmallHeight = 0;
  int textSmallWidth = 0;
  getTextWidthAndHeight(textSizeSmall, minText, &textSmallWidth, &textSmallHeight);
  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawString(textSizeNormal, textXStart, currentY, "Z");
  drawString(textSizeSmall, textXStart + zTextWidth, currentY + 10, "min");
  drawString(textSizeNormal, textXStart + zTextWidth + textSmallWidth, currentY, ":");
  drawString(textSizeNormal, textXStart + zTextWidth + textSmallWidth + 20, currentY, zMinText);

  drawString(textSizeNormal, leftMiddleX, currentY, "Z");
  drawString(textSizeSmall, leftMiddleX + zTextWidth, currentY + 10, "max");
  drawString(textSizeNormal, leftMiddleX + zTextWidth + textSmallWidth, currentY, ":");
  drawString(textSizeNormal, leftMiddleX + zTextWidth + textSmallWidth + 20, currentY, zMaxText);

  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawString(textSizeNormal, textXStart, currentY, "Z");
  drawString(textSizeSmall, textXStart + zTextWidth, currentY + 10, "avg");
  drawString(textSizeNormal, textXStart + zTextWidth + textSmallWidth, currentY, ":");
  drawString(textSizeNormal, textXStart + zTextWidth + textSmallWidth + 20, currentY, zAvgText);

  char cycleText[] = "Zyklen:";
  getTextWidthAndHeight(textSizeNormal, cycleText, &textNormalWidth, &textNormalHeight);
  drawString(textSizeNormal, leftMiddleX, currentY, cycleText);
  drawString(textSizeNormal, leftMiddleX + textNormalWidth + 10, currentY, cycleValueText);

  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawTextWithCheckbox(textSizeNormal, textXStart, currentY, "Laden:", runInfo->cdmos == 1);

  drawTextWithCheckbox(textSizeNormal, leftMiddleX, currentY, "Entladen:", runInfo->fdmos == 1);

  currentY = currentY + textNormalHeight + textDistanceVertical;
  drawTextWithCheckbox(textSizeNormal, textXStart, currentY, "Alarm:", smartbmsutilHasAlarmSet(runInfo));
}

void drawBmsBatteries(SmartbmsutilRunInfo *runInfo) {
  char text[10];
  int countBatteriesPerRow = runInfo->countBatteryVoltages > 8 ? 4 : 2;
  int textHeight = 0;
  int textWidth = 0;
  getTextWidthAndHeight(8, "0.000 V", &textWidth, &textHeight);
  int margin = 30;
  int batteryIndex = 0;
  int startX = EPD_WIDTH / 2 + margin;
  int startY = 60;
  int batteryHeight = 30;
  int batteryFieldWidth = (EPD_WIDTH / 2 - 2 * margin) / countBatteriesPerRow;
  int batteryFieldHeight = batteryHeight + textHeight + margin;
  int batteryLength = batteryFieldWidth - margin;
  int numRows = round(runInfo->countBatteryVoltages / countBatteriesPerRow);
  for (int row = 0; row < numRows; row++) {
    for (int column = 0; column < countBatteriesPerRow; column++) { 
      int batteryX = startX + column * batteryFieldWidth;
      int batteryY = startY + row * batteryFieldHeight;
      char numberText[2];
      sprintf(numberText, "%ld", batteryIndex + 1);
      drawBattery(batteryX, batteryY, batteryLength, batteryHeight, numberText);
      sprintf(text, "%.3f V", runInfo->batteryVoltages[batteryIndex] / 1000.0); 
      drawString(8, batteryX + (batteryLength - textWidth) / 2, batteryY + batteryHeight + textHeight + 10, text);
      batteryIndex++;
    }
  }
}

void drawBmsTemperatures(SmartbmsutilRunInfo *runInfo) {
  char text[10];
  int countTemperaturesPerRow = 2;
  int textHeight = 0;
  int textWidth = 0;
  getTextWidthAndHeight(18, "T1: 21??C", &textWidth, &textHeight);
  int margin = 20;
  int temperatureIndex = 0;
  int startX = EPD_WIDTH / 2 + margin;
  int startY = 390;
  int temperatureFieldWidth = (EPD_WIDTH / 2 - 2 * margin) / countTemperaturesPerRow;
  int temperatureFieldHeight = 80;
  int numRows = round(runInfo->countBatteryTemp / countTemperaturesPerRow);
  for (int row = 0; row < numRows; row++) {
    for (int column = 0; column < countTemperaturesPerRow; column++) { 
      int temperatureX = startX + column * temperatureFieldWidth;
      int temperatureY = startY + row * temperatureFieldHeight;
      sprintf(text, "T%d: %d??C", temperatureIndex + 1, runInfo->batteryTemp[temperatureIndex] - 40); 
      drawString(18, temperatureX, temperatureY, text);
      temperatureIndex++;
    }
  }
}

void displayStartingMessage() {
  clearFrameBuffer();
  drawHeader("");
  int textHeight = 0;
  int textWidth = 0;
  char text[] = "Starte...";
  getTextWidthAndHeight(18, text, &textWidth, &textHeight);
  drawString(18, EPD_WIDTH / 2 - textWidth / 2, EPD_HEIGHT / 2, text);
  updateDisplay();
}

void displayDrawContentBms(SmartbmsutilRunInfo *runInfo) {
  clearFrameBuffer();
  drawHeader("BMS");
  drawBmsSectionBorders();
  drawBmsOverviewData(runInfo);
  drawBmsBatteries(runInfo);
  drawBmsTemperatures(runInfo);   
  updateDisplay();
}
