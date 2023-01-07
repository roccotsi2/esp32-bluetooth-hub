/*void buttonsInit() {
  pinMode(GPIO_1, INPUT);
  pinMode(GPIO_2, INPUT);
  pinMode(GPIO_3, INPUT);
}

byte buttonsCheckButtonPressed(byte port) {
  int valNew = digitalRead(port);
  if (valNew == 0) { // 0 = pressed
    delay(10); // Button pressed, entprellen
    valNew = digitalRead(port);

    // if button is currently pressed
    if (valNew == 0) {
      unsigned long millisStart = millis();
      while (valNew == 0) {
        valNew = digitalRead(port);
        delay(10);
      }
      unsigned long millisClickInterval = millis() - millisStart;
      if (millisClickInterval > MILLIS_LONG_BUTTON_PRESS) {
        return BUTTON_LONG_PRESSED;
      } else {
        return BUTTON_SHORT_PRESSED;
      }unsigned long currentMillis = millis();
    }
  }
  return BUTTON_NOT_PRESSED;
}*/
