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

}

void drawString(int x, int y, char *text) {
  write_string(&FiraSans, text, &x, &y, frameBuffer);
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

void drawHeader(char *title) {
  // title
  drawString(0, 35, title);

  // button reset
  drawString(212, 35, "Res");

  // button left
  drawString(372, 32, "❬");

  // button right
  drawString(445, 32, "❭");

  // button select
  drawString(485, 35, "Sel");

  // horizontal line
  epd_draw_hline(0, 40, EPD_WIDTH, COLOR_BLACK, frameBuffer);

  // vertical line left from first button
  epd_draw_vline(208, 0, 40, COLOR_BLACK, frameBuffer);

  // vertical line right from fifth button
  epd_draw_vline(560, 0, 40, COLOR_BLACK, frameBuffer);
}

void displayDrawDynamicContentBms(SmartbmsutilRunInfo runInfo) {
  float currentV = runInfo.currentV / 10.0;
  float currentA = (runInfo.currentA - 30000) / 10.0;
  float currentKw = runInfo.currentKw / 1000.0;
  char currentVText[10];
  sprintf(currentVText, "%.1f V", currentV);
  char currentAText[10];
  sprintf(currentAText, "%.1f A", currentA);
  char currentKwText[10];
  sprintf(currentKwText, "%.3f kW", currentKw);

  drawHeader("BMS");

  // divide in left and right area
  epd_draw_vline(EPD_WIDTH / 2, 40, EPD_HEIGHT - 40, COLOR_BLACK, frameBuffer);

  // devide right area in top and bottom area
  epd_draw_hline(EPD_WIDTH / 2, 340, EPD_WIDTH / 2, COLOR_BLACK, frameBuffer);

  drawString(5, 90, "SOC:");
  drawProgressBar(100, 50, 350, 50, 33);

  drawString(5, 140, "Spannung:");
  drawString(230, 140, currentVText);

  drawString(5, 190, "Strom:");
  drawString(230, 190, currentAText);

  drawString(5, 240, "Leistung:");
  drawString(230, 240, currentKwText);

  updateDisplay();
}
