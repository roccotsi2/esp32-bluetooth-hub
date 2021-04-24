// about 93 pixel per cm

void displayInit() {
  epd_init();
  frameBuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!frameBuffer) Serial.println("Memory alloc failed!");
  memset(frameBuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void updateDisplay() {
  epd_poweron();
  epd_clear();
  epd_draw_grayscale_image(epd_full_screen(), frameBuffer);
  epd_poweroff_all();
}

void displayDrawStaticContentBms() {  
  /*int x = 50;
  int y = 50;  
  write_string(&OpenSans8B, "OpenSans8B", &x, &y, frameBuffer);

  x = 50;
  y = 150;  
  write_string(&OpenSans10B, "OpenSans10B", &x, &y, frameBuffer);

  x = 50;
  y = 250;  
  write_string(&OpenSans12B, "OpenSans12B", &x, &y, frameBuffer);

  x = 50;
  y = 350;  
  write_string(&OpenSans18B, "OpenSans18B", &x, &y, frameBuffer);

  x = 50;
  y = 450;  
  write_string(&OpenSans24B, "OpenSans24B", &x, &y, frameBuffer);

  updateDisplay();*/
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

void drawProgressBar(int x, int y, int width, int height, int value) {
  if (value > 100) {
    value = 100;
  }
  if (value < 0) {
    value = 0;
  }
  int fillWidth = (width * value) / 100;
  epd_draw_rect(x, y, width, height, COLOR_BLACK, frameBuffer);
  epd_fill_rect(x, y, fillWidth, height, COLOR_GREY, frameBuffer);
}

void drawBattery(int x, int y, int length, int height, int number) {
  drawHLine(x, y, length - 10, COLOR_BLACK, 2);
  drawHLine(x, y + height, length - 10, COLOR_BLACK, 2);
  drawVLine(x, y, height, COLOR_BLACK, 2);
  drawVLine(x + length - 10, y, height / 4, COLOR_BLACK, 2);
  drawVLine(x + length - 10, y + height - (height / 4), height / 4, COLOR_BLACK, 2);
  drawHLine(x + length - 10, y + height / 4, 10, COLOR_BLACK, 2);
  drawHLine(x + length - 10, y + height - (height / 4), 10, COLOR_BLACK, 2);
  drawVLine(x + length, y + height / 4, y + height - (height / 4) - (y + height / 4), COLOR_BLACK, 2);
  char numberText[2];
  sprintf(numberText, "%ld", number);
  drawString(8, x + length / 2 - 10, y + height / 2 + 8, numberText);
}

void drawHeader(char *title) {
  // title
  drawString(24, 0, 35, title);

  // button reset
  drawString(12, 220, 27, "Res");

  // button left
  drawHLine(380, 18, 15, COLOR_BLACK, 5);
  epd_fill_triangle(357, 20, 380, 10, 380, 30, COLOR_BLACK, frameBuffer);

  // button right
  //drawString(12, 445, 32, "❭");
  drawHLine(430, 18, 15, COLOR_BLACK, 5);
  epd_fill_triangle(468, 20, 445, 10, 445, 30, COLOR_BLACK, frameBuffer);

  // button select
  drawString(12, 495, 27, "Sel");

  // horizontal line
  drawHLine(0, 40, EPD_WIDTH, COLOR_BLACK, 2);

  // vertical line left from first button
  drawVLine(208, 0, 40, COLOR_BLACK, 2);

  // vertical line right from fifth button
  drawVLine(560, 0, 40, COLOR_BLACK, 2);
}

void displayDrawDynamicContentBms(SmartbmsutilRunInfo runInfo) {
  float currentV = runInfo.currentV / 10.0;
  float currentA = (runInfo.currentA - 30000) / 10.0;
  float currentKw = runInfo.currentKw / 1000.0;
  float zMin = round(runInfo.minCellVoltage / 10.0) / 100.0;
  float zMax = round(runInfo.maxCellVoltage / 10.0) / 100.0;
  float zAvg = round(runInfo.avgVoltage / 10.0) / 100.0;
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
  char cycleText[10];
  sprintf(cycleText, "%ld", runInfo.avgVoltage);

  drawHeader("BMS");

  // divide in left and right area
  drawVLine(EPD_WIDTH / 2, 40, EPD_HEIGHT - 40, COLOR_BLACK, 2);

  // devide right area in top and bottom area
  drawHLine(EPD_WIDTH / 2, 340, EPD_WIDTH / 2, COLOR_BLACK, 2);

  drawString(18, 5, 90, "Ladung:");
  drawProgressBar(235, 50, 230, 50, 33);

  drawString(18, 5, 140, "Spannung:");
  drawString(18, 230, 140, currentVText);

  drawString(18, 5, 190, "Strom:");
  drawString(18, 230, 190, currentAText);

  drawString(18, 5, 240, "Leistung:");
  drawString(18, 230, 240, currentKwText);

  drawString(18, 5, 290, "Z");
  drawString(8, 28, 300, "min");
  drawString(18, 60, 290, ":");
  drawString(18, 80, 290, zMinText);

  drawString(18, 230, 290, "Z");
  drawString(8, 253, 300, "max");
  drawString(18, 285, 290, ":");
  drawString(18, 305, 290, zMaxText);

  drawString(18, 5, 340, "Z");
  drawString(8, 28, 350, "avg");
  drawString(18, 60, 340, ":");
  drawString(18, 80, 340, zAvgText);

  drawString(18, 230, 340, "Zyklen:");
  drawString(18, 370, 340, cycleText);

  drawBattery(EPD_WIDTH / 2 + 10, 70, 50, 30, 1);

  updateDisplay();
}
