#define COLOR_WHITE 0xF0

// internal state
ButtonData buttons[20];
ListBoxData listBox;
int countButtons = 0;
ButtonData lastPressedButton;
char lastPressedListBoxItem[30];
bool listBoxItemPressed = false;
bool buttonPressed = false;
int lastPressedButtonIndex = -1;

const byte WIDTH_LIST_BOX_UP_DOWN_BUTTON = 50;
const byte HEIGHT_LIST_BOX_UP_DOWN_BUTTON = 50;
const int HEIGHT_LIST_BOX_ELEMENT = 60;

boolean touchutilAddListBox(int id, int x, int y, int width, int height, char *text, uint8_t *frameBuffer, char elements[10][30], int elementCount) {
  bool success = touchutilRegisterListBox(id, x, y, width, height, text, elements, elementCount);
  //Serial.println("ListBox registered");
  if (!success) {
    return false;
  }

  touchutilDrawListBox(true, frameBuffer);
  //Serial.println("ListBox drawed");
  
  return true; // ListBox successfully created
}

bool touchutilRegisterListBox(int id, int x, int y, int width, int height, char *text, char elements[10][30], int elementCount) {
  //int elementsPerPage = min(height / HEIGHT_LIST_BOX_ELEMENT, elementCount);
  int elementsPerPage = height / HEIGHT_LIST_BOX_ELEMENT;
  int elementHeight = height / elementsPerPage;
  int adpatedHeight = elementsPerPage * elementHeight;
  
  Rect_t rect = {
    .x = x,
    .y = y,
    .width = width,
    .height = adpatedHeight // adapt the height to the number of elements
  };
  listBox.id = id;
  listBox.area = rect;
  listBox.text = text;
  for (int i = 0; i < 10; i++) {
    strcpy(listBox.elements[i], elements[i]);
  }
  
  listBox.elementCount = elementCount;
  listBox.elementsPerPage = elementsPerPage;
  listBox.elementHeight = elementHeight;
  listBox.currentPageNo = 1; // first page = 1
  if (elementCount % elementsPerPage == 0) {
    listBox.pageCount = elementCount / elementsPerPage;
  } else {
    listBox.pageCount = elementCount / elementsPerPage + 1;
  }
  memset(listBox.buttonIndexElements, -1, 10); // initialize buttonIndexElements with -1
  
  return true;
}

bool touchutilIsButtonIndexFromListBoxItem(int buttonIndex) {
  for (int i = 0; i < 10; i++) {
    if (listBox.buttonIndexElements[i] == buttonIndex) {
      return true;
    }
  }
  return false;
}

void touchutilCheckListBoxButtons(uint8_t *frameBuffer) {
  listBoxItemPressed = false;
  if (buttonPressed) {
    if (lastPressedButtonIndex == listBox.buttonIndexUp) {
      // button UP pressed
      //Serial.println("ListBox button pressed: UP");
      buttonPressed = false;
      touchutilChangePageList(listBox.currentPageNo - 1, frameBuffer);
    } else if (lastPressedButtonIndex == listBox.buttonIndexDown) {
      // button DOWN pressed
      //Serial.println("ListBox button pressed: DOWN");
      buttonPressed = false;
      touchutilChangePageList(listBox.currentPageNo + 1, frameBuffer);
    } else {
      bool listBoxItemButtonPressed = touchutilIsButtonIndexFromListBoxItem(lastPressedButtonIndex);
      if (listBoxItemButtonPressed) {
        // item button pressed
        if (lastPressedButton.text[0] != 0) {
          // assume that listbox item text is not empty
          listBoxItemPressed = true;
          strcpy(lastPressedListBoxItem, lastPressedButton.text);
        } else {
          // empty area of list box pressed
          listBoxItemPressed = false;
        }
        buttonPressed = false;
      }
    }
  }
}

void touchutilChangePageList(int newPageNo, uint8_t *frameBuffer) {
  if (newPageNo < 1 || listBox.pageCount < newPageNo) {
    // start or end reached -> nothing to do
    return;
  }
  // from here newPageNo is valid
  listBox.currentPageNo = newPageNo;

  // replace the texts of the buttons
  int startIndex = (listBox.currentPageNo - 1) * listBox.elementsPerPage; // 6
  int endIndex = min(startIndex + listBox.elementsPerPage - 1, listBox.elementCount - 1); // 9
  /*Serial.print("startIndex: ");
  Serial.println(startIndex);
  Serial.print("endIndex: ");
  Serial.println(endIndex);*/
  
  for (int i = startIndex; i <= endIndex; i++) {
    // set the item texts
    int buttonIndex = listBox.buttonIndexElements[i - startIndex];
    strncpy(buttons[buttonIndex].text, listBox.elements[i], 30);
  }
  for (int i = endIndex - startIndex + 1; i < listBox.elementsPerPage; i++) {
    // set empty text for last listbox items
    int buttonIndex = listBox.buttonIndexElements[i];
    memset(buttons[buttonIndex].text, 0, 30);
  }

  touchutilDrawListBox(false, frameBuffer);
}

void touchutilDrawListBox(bool initialDraw, uint8_t *frameBuffer) { 
  if (!initialDraw) {
    // first clear the area of the list box if this is not the first time it gets drawed
    epd_fill_rect(listBox.area.x, listBox.area.y, listBox.area.width, listBox.area.height, COLOR_WHITE, frameBuffer);
  }
  
  // add border, up/down buttons
  epd_draw_rect(listBox.area.x, listBox.area.y, listBox.area.width, listBox.area.height, 0, frameBuffer); // border of list box
  //Serial.println("ListBox border drawed");
  epd_draw_rect(listBox.area.x + listBox.area.width - WIDTH_LIST_BOX_UP_DOWN_BUTTON, listBox.area.y, WIDTH_LIST_BOX_UP_DOWN_BUTTON, listBox.area.height, 0, frameBuffer); // border of scroll
  //Serial.println("ListBox scroll border drawed");
  
  if (initialDraw) {
    // add 1 button per displayed element
    int count = min(listBox.elementsPerPage, listBox.elementCount);
    for (int i = 0; i < count; i++) {
      listBox.buttonIndexElements[i] = touchutilAddButton(listBox.area.x, listBox.area.y + i*listBox.elementHeight, listBox.area.width - WIDTH_LIST_BOX_UP_DOWN_BUTTON, listBox.elementHeight, listBox.elements[i], true, frameBuffer);
    }
    //Serial.println("ListBox buttons added");
    // add UP / DOWN buttons
    listBox.buttonIndexUp = touchutilAddButton(listBox.area.x + listBox.area.width - WIDTH_LIST_BOX_UP_DOWN_BUTTON, listBox.area.y, WIDTH_LIST_BOX_UP_DOWN_BUTTON, HEIGHT_LIST_BOX_UP_DOWN_BUTTON, "^", true, frameBuffer);
    listBox.buttonIndexDown = touchutilAddButton(listBox.area.x + listBox.area.width - WIDTH_LIST_BOX_UP_DOWN_BUTTON, listBox.area.y + listBox.area.height - HEIGHT_LIST_BOX_UP_DOWN_BUTTON, WIDTH_LIST_BOX_UP_DOWN_BUTTON, HEIGHT_LIST_BOX_UP_DOWN_BUTTON, "v", true, frameBuffer);
    //Serial.println("Up/Down buttons added");
  } else {    
    // draw only the buttons
    for (int i = 0; i < listBox.elementsPerPage; i++) {
      ButtonData button = buttons[listBox.buttonIndexElements[i]];
      if (button.text[0] != 0) {
        // draw only button with text
        touchutilDrawButton(button, frameBuffer);
      }
    }
    touchutilDrawButton(buttons[listBox.buttonIndexUp], frameBuffer);
    touchutilDrawButton(buttons[listBox.buttonIndexDown], frameBuffer);
    touchutilDrawScreen();
  }
}

void touchutilDrawButton(ButtonData button, uint8_t *frameBuffer) {
  if (button.drawBorder) {
    epd_draw_rect(button.area.x, button.area.y, button.area.width, button.area.height, 0, frameBuffer);
  }
  if (strlen(button.text) > 0) {
    int text_x = button.area.x + 15;
    int text_y = button.area.y + 40;
    write_string((GFXfont *)&FiraSans, button.text, &text_x, &text_y, frameBuffer);
  }
  //Serial.println(button.text);
}

int touchutilAddButton(int x, int y, int width, int height, char *text, bool drawBorder, uint8_t *frameBuffer) {  
  // register button
  int buttonIndex = touchutilRegisterButton(x, y, width, height, text, drawBorder);

  // draw button
  touchutilDrawButton(buttons[buttonIndex], frameBuffer);
  
  return buttonIndex;
}

int touchutilRegisterButton(int x, int y, int width, int height, char *text, bool drawBorder) {
  if (countButtons >= sizeof(buttons)) {
    Serial.println("Could not register button, max size exceeded");
    return -1;
  }

  Rect_t rect = {
    .x = x,
    .y = y,
    .width = width,
    .height = height
  };
  ButtonData buttonData = {
    .id = countButtons + 1, // ID starts with 1
    .area = rect,
    .drawBorder = drawBorder
  };
  strcpy(buttonData.text, text);
  int buttonIndex = countButtons;
  buttons[buttonIndex] = buttonData;
  countButtons++;
  return buttonIndex;
}

void touchutilCheckTouch(uint8_t *frameBuffer) {
  buttonPressed = false; // reset

  // check for pressed button
  if (touch.scanPoint()) {
    uint16_t  x, y;    //pressedButtonData->text = lastPressedButton.text;
    touch.getPoint(x, y, 0);
    y = EPD_HEIGHT - y;
    for (int i = 0; i < countButtons; i++) {
      ButtonData buttonData = buttons[i];
      if (x >= buttonData.area.x && x <= buttonData.area.x + buttonData.area.width && y >= buttonData.area.y && y <= buttonData.area.y + buttonData.area.height) {
        // button found, wait until it is released
        touchutilWaitUntilNoPress();

        // fill the found pressed button
        lastPressedButton.id = buttonData.id;
        lastPressedButton.area = buttonData.area;
        if (buttonData.text[0] > 0) {
          strcpy(lastPressedButton.text, buttonData.text);
        } else {
          // text of button is empty
          memset(lastPressedButton.text, 0, 30);
        }
        lastPressedButtonIndex = i;
        buttonPressed = true;
        break;
      }
    }
  }

  touchutilCheckListBoxButtons(frameBuffer);
}

bool touchutilGetPressedButton(ButtonData *pressedButtonData) {
  if (buttonPressed) {
    pressedButtonData->id = lastPressedButton.id;
    pressedButtonData->area = lastPressedButton.area;
    strcpy(pressedButtonData->text, lastPressedButton.text);
    buttonPressed = false; // reset
    return true; // button found
  }
  return false; // no button found
}

bool touchutilGetPressedListBoxItem(char *buf, int count) {
  if (!listBoxItemPressed) {
    return false;
  }
  if (count < sizeof(lastPressedListBoxItem)) {
    Serial.print("Size of buf too small, needed size: ");
    Serial.println(sizeof(lastPressedListBoxItem));
    return false;
  }
  strcpy(buf, lastPressedListBoxItem);
  return true;
}

void touchutilWaitUntilNoPress() {
  int count = 0;
  while (count < 10) {
    if (touch.scanPoint()) {
      count = 0;
    } else {
      count++;
    }
    delay(10);
  }
}

int touchutilGetButtonIdByIndex(int index) {
  /*Serial.print("touchutilGetButtonIdByIndex: index: ");
  Serial.println(index);*/
  if (index < 0 || index >= sizeof(buttons)) {
    // invalid index
    return -1;
  }
  return buttons[index].id;
}

void touchutilDrawScreen() {
  epd_poweron();
  epd_clear();
  epd_draw_grayscale_image(epd_full_screen(), frameBuffer);
  epd_poweroff();
}

void touchutilInitializeRect(Rect_t rect) {
  rect.x = 0;
  rect.y = 0;
  rect.width = 0;
  rect.height = 0;
}

void touchutilInitializeButton(ButtonData button) {
  button.id = 0;
  touchutilInitializeRect(button.area);
  button.drawBorder = false;
  memset(button.text, 0, sizeof(button.text));
}

void touchutilInitializeListBox(ListBoxData listBox) {
  listBox.id = 0;
  touchutilInitializeRect(listBox.area);
  listBox.text = NULL;
  memset(listBox.elements, 0, sizeof(listBox.elements));
  listBox.elementCount = 0;
  listBox.elementsPerPage = 0;
  listBox.elementHeight = 0;
  listBox.currentPageNo = 0;
  listBox.pageCount = 0;
  memset(listBox.buttonIndexElements, 0, sizeof(listBox.buttonIndexElements));
  listBox.buttonIndexUp = 0;
  listBox.buttonIndexDown = 0;
}

/**
 * Initializes all internal variables and clears prvious state
 */
void touchutilInitialize() {
  for (int i = 0; i < sizeof(buttons); i++) {
    touchutilInitializeButton(buttons[i]);
  }
  touchutilInitializeListBox(listBox);
  countButtons = 0;
  touchutilInitializeButton(lastPressedButton);
  memset(lastPressedListBoxItem, 0, sizeof(lastPressedListBoxItem));
  listBoxItemPressed = false;
  buttonPressed = false;
  lastPressedButtonIndex = -1;

  //Initialize listbox special values (that differs from 0)
  memset(listBox.buttonIndexElements, -1, sizeof(listBox.buttonIndexElements)); 
  listBox.buttonIndexUp = -1;
  listBox.buttonIndexDown = -1;
}
