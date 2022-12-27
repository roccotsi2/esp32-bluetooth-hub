//#pragma pack(1)
typedef struct buttonData {
   uint8_t id;
   Rect_t area;
   bool drawBorder;
   char text[30];
} __attribute__ ((packed)) ButtonData;

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
} __attribute__ ((packed)) ListBoxData;
