//#pragma pack(1)
typedef struct buttonData {
   uint8_t id;
   Rect_t area;
   bool drawBorder;
   char text[30];
} ButtonData; // __attribute__ ((packed)) ButtonData;

//#pragma pack(1)
typedef struct listBoxData {
   uint8_t id;
   Rect_t area;
   char *text;
   char elements[10][30];
   int elementCount;
   int elementsPerPage;
   int elementHeight;
   int currentPageNo; // first page = 1
   int pageCount;
   int8_t buttonIndexElements[10];
   int buttonIndexUp;
   int buttonIndexDown;
} ListBoxData; // __attribute__ ((packed)) ListBoxData;

//#pragma pack(1)
typedef struct numberEntryData {
   uint8_t id;
   Rect_t area;
   char text[10];
   int currentCursorPos;
   int value;
   int8_t buttonIndex[12]; // 0-9, clear, ok
} NumberEntryData; // __attribute__ ((packed)) NumberEntryData;
