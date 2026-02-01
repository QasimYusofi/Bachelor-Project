#include "EspUsbHostKeybord.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Crypto.h>
#include <AES.h>
#include <string.h>

// AES-256 uses 32-byte key
#define KEY_SIZE 32
#define BLOCK_SIZE 16

// Your secret key
uint8_t key[KEY_SIZE] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

AES256 aes256;

// TFT Display - SINGLE GLOBAL OBJECT
TFT_eSPI tft = TFT_eSPI();

// ==============================================
// PERSIAN HELPER SECTION
// ==============================================

// Glyph structure: stores dimensions + bitmap data
struct Glyph {
  uint8_t width;   // Actual character width in pixels (1-8)
  uint8_t height;  // Character height (typically 7-8)
  const uint8_t* bitmap; // Row-wise bitmap (MSB=left, LSB=right)
};

const int GLYPH_WIDTH = 6;
const int GLYPH_HEIGHT = 8;

// Persian alphabet glyphs
const uint8_t aleph_data[] = {0b00000001,0b00000001,0b00000001,0b00000001,0b00000001,0b00000000,0b00000000,0b00000000};
const uint8_t be_data[] = {0b00000000,0b00000000,0b00000000,0b00000001,0b00111111,0b00000000,0b00000100,0b00000000};
const uint8_t pe_data[] = {0b00000000,0b00000000,0b00000000,0b00000001,0b00111111,0b00000000,0b00001010,0b00000100};
const uint8_t te_data[] = {0b00000000,0b00010100,0b00000000,0b00000001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t se_data[] = {0b00001000,0b00010100,0b00000000,0b00000001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t jim_data[] = {0b00000000,0b00011000,0b00010100,0b00000010,0b00111111,0b00000000,0b00000100,0b00000000};
const uint8_t che_data[] = {0b00000000,0b00011000,0b00010100,0b00000010,0b00111111,0b00000000,0b00001010,0b00000100};
const uint8_t he_data[] = {0b00000000,0b00011000,0b00010100,0b00000010,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t khe_data[] = {0b00000010,0b00001000,0b00010100,0b00000010,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t dall_data[] = {0b00000000,0b00001000,0b00000100,0b00000010,0b00011111,0b00000000,0b00000000,0b00000000};
const uint8_t zall_data[] = {0b00000010,0b00001000,0b00000100,0b00000010,0b00011111,0b00000000,0b00000000,0b00000000};
const uint8_t re_data[] = {0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000011,0b00000110,0b00001100};
const uint8_t ze_data[] = {0b00000000,0b00000000,0b00000001,0b00000000,0b00000001,0b00000011,0b00000110,0b00001100};
const uint8_t zhe_data[] = {0b00000000,0b00000010,0b00000101,0b00000000,0b00000001,0b00000011,0b00000110,0b00001100};
const uint8_t sen_data[] = {0b00000000,0b00000000,0b00000000,0b00000101,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t shen_data[] = {0b00000100,0b00001010,0b00000000,0b00000101,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t ain_data[] = {0b00000000,0b00000000,0b00000011,0b00000100,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t ghain_data[] = {0b00000010,0b00000000,0b00000011,0b00000100,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t fe_data[] = {0b00000010,0b00000000,0b00000111,0b00000101,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t qe_data[] = {0b00000101,0b00000000,0b00000111,0b00000101,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t swat_data[] = {0b00000000,0b00000000,0b00000111,0b00001001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t zwat_data[] = {0b00000010,0b00000000,0b00000111,0b00001001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t kaf_data[] = {0b00000001,
                            0b00000010,
                            0b00001100,
                            0b00000010,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t gaf_data[] = {0b00000101,
                            0b00001010,
                            0b00010100,
                            0b00000010,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t toy_data[] = {0b00010000,0b00010000,0b00010110,0b00011001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t zoy_data[] = {0b00010000,0b00010000,0b00010111,0b00011001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t lam_data[] = {0b00000001,0b00000001,0b00000001,0b00000001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t mem_data[] = {0b00000000,0b00000000,0b00000000,0b00000111,0b00111101,0b00000111,0b00000000,0b00000000};
const uint8_t non_data[] = {0b00000000,0b00000100,0b00000000,0b00000001,0b00111111,0b00000000,0b00000000,0b00000000};
const uint8_t wow_data[] = {0b00000000,0b00000000,0b00000000,0b00000111,0b00000101,0b00000011,0b00001110,0b00000000};
const uint8_t ha_data[] = {0b00000000,0b00000000,0b00011100,0b00010100,0b00001111,0b00000000,0b00000000,0b00000000};
const uint8_t ya_data[] = {0b00000000,0b00000000,0b00000000,0b00000001,0b00111111,0b00000000,0b00001010,0b00000000};

const Glyph aleph = {2, GLYPH_HEIGHT, aleph_data};
const Glyph be = {GLYPH_WIDTH, GLYPH_HEIGHT, be_data};
const Glyph pe = {GLYPH_WIDTH, GLYPH_HEIGHT, pe_data};
const Glyph te = {GLYPH_WIDTH, GLYPH_HEIGHT, te_data};
const Glyph se = {GLYPH_WIDTH, GLYPH_HEIGHT, se_data};
const Glyph jim = {GLYPH_WIDTH, GLYPH_HEIGHT, jim_data};
const Glyph che = {GLYPH_WIDTH, GLYPH_HEIGHT, che_data};
const Glyph he = {GLYPH_WIDTH, GLYPH_HEIGHT, he_data};
const Glyph khe = {GLYPH_WIDTH, GLYPH_HEIGHT, khe_data};
const Glyph dall = {GLYPH_WIDTH, GLYPH_HEIGHT, dall_data};
const Glyph zall = {GLYPH_WIDTH, GLYPH_HEIGHT, zall_data};
const Glyph re = {GLYPH_WIDTH, GLYPH_HEIGHT, re_data};
const Glyph ze = {GLYPH_WIDTH, GLYPH_HEIGHT, ze_data};
const Glyph zhe = {GLYPH_WIDTH, GLYPH_HEIGHT, zhe_data};
const Glyph sen = {GLYPH_WIDTH, GLYPH_HEIGHT, sen_data};
const Glyph shen = {GLYPH_WIDTH, GLYPH_HEIGHT, shen_data};
const Glyph ain = {GLYPH_WIDTH, GLYPH_HEIGHT, ain_data};
const Glyph ghain = {GLYPH_WIDTH, GLYPH_HEIGHT, ghain_data};
const Glyph fe = {GLYPH_WIDTH, GLYPH_HEIGHT, fe_data};
const Glyph qe = {GLYPH_WIDTH, GLYPH_HEIGHT, qe_data};
const Glyph swat = {GLYPH_WIDTH, GLYPH_HEIGHT, swat_data};
const Glyph zwat = {GLYPH_WIDTH, GLYPH_HEIGHT, zwat_data};
const Glyph kaf = {GLYPH_WIDTH, GLYPH_HEIGHT, kaf_data};
const Glyph gaf = {GLYPH_WIDTH, GLYPH_HEIGHT, gaf_data};
const Glyph toy = {GLYPH_WIDTH, GLYPH_HEIGHT, toy_data};
const Glyph zoy = {GLYPH_WIDTH, GLYPH_HEIGHT, zoy_data};
const Glyph lam = {GLYPH_WIDTH, GLYPH_HEIGHT, lam_data};
const Glyph mem = {GLYPH_WIDTH, GLYPH_HEIGHT, mem_data};
const Glyph non = {GLYPH_WIDTH, GLYPH_HEIGHT, non_data};
const Glyph wow = {GLYPH_WIDTH, GLYPH_HEIGHT, wow_data};
const Glyph ha = {GLYPH_WIDTH, GLYPH_HEIGHT, ha_data};
const Glyph ya = {GLYPH_WIDTH, GLYPH_HEIGHT, ya_data};

// Persian Helper Functions
const Glyph* getGlyphForChar(char latinChar) {
  switch(latinChar) {
    case 'a': return &shen; // ش
    case 'b': return &zall; // ذ
    case 'c': return &ze;   // ز
    case 'd': return &ya;   // ی
    case 'e': return &se;   // ث
    case 'f': return &be;   // ب
    case 'g': return &lam;  // ل
    case 'h': return &aleph;// ا
    case 'i': return &ha;   // ه
    case 'j': return &te;   // ت
    case 'k': return &non;  // ن
    case 'l': return &mem;  // م
    case 'm': return &kaf;  // ک
    case 'n': return &dall; // د
    case 'o': return &khe;  // خ
    case 'p': return &he;   // ح
    case 'q': return &zwat; // ض
    case 'r': return &qe;   // ق
    case 's': return &sen;  // س
    case 't': return &fe;   // ف
    case 'u': return &ain;  // ع
    case 'v': return &re;   // ر
    case 'w': return &swat; // ص
    case 'x': return &toy;  // ط
    case 'y': return &ghain;// غ
    case 'z': return &zoy;  // ظ
    case '[': return &jim;  // ژ
    case ']': return &che;  // چ
    case '\\': return &pe;  // پ
    case ';': return &kaf;  // گ  
    case '\'': return &gaf; // ک  
    case 'C': return &zhe;  // ژ  
    case ',': return &wow;  // و
    default: return nullptr;
  }
}

void drawGlyph(const Glyph* glyph, int x, int y, int fontSize, 
               uint16_t color, uint16_t bgColor = 0xFFFF) {
  if (!glyph || fontSize < 1) return;
  
  for (int row = 0; row < glyph->height; row++) {
    uint8_t rowData = glyph->bitmap[row];
    
    for (int col = 0; col < glyph->width; col++) {
      bool pixelSet = rowData & (1 << col); // col 0 = LSB = rightmost
      
      int drawX = x + (glyph->width - 1 - col) * fontSize;
      int drawY = y + row * fontSize;
      
      if (pixelSet) {
        if (fontSize == 1) {
          tft.drawPixel(drawX, drawY, color);
        } else {
          tft.fillRect(drawX, drawY, fontSize, fontSize, color);
        }
      } else if (bgColor != 0xFFFF) {
        if (fontSize == 1) {
          tft.drawPixel(drawX, drawY, bgColor);
        } else {
          tft.fillRect(drawX, drawY, fontSize, fontSize, bgColor);
        }
      }
    }
  }
}

// Draw single character (Persian glyph or standard font)
void drawPersianChar(char c, int &currentX, int y, int fontSize, 
                     uint16_t color, uint16_t bgColor) {
  const Glyph* glyph = getGlyphForChar(c);
  
  if (glyph) {
    // Draw Persian glyph
    drawGlyph(glyph, currentX - (glyph->width * fontSize) + fontSize, y, 
              fontSize, color, bgColor);
    
    // Move left by actual scaled width
    currentX -= (glyph->width * fontSize);
  } else if (c == ' ') {
    // Handle space: move left by space width
    currentX -= (3 * fontSize);
  } else {
    // Regular character (number, symbol, etc.) - draw with standard font
    // Save current text settings
    uint16_t oldTextColor = tft.textcolor;
    uint16_t oldTextBgColor = tft.textbgcolor;
    uint8_t oldTextSize = tft.textsize;
    
    // Set up for drawing the character
    tft.setTextColor(color, bgColor);
    tft.setTextSize(fontSize);
    
    // Draw the character at the right position
    int charWidth = 6 * fontSize; // Standard font width
    int drawX = currentX - charWidth + fontSize;
    
    tft.setCursor(drawX, y);
    tft.print(c);
    
    // Move left by character width
    currentX -= charWidth;
    
    // Restore text settings
    tft.setTextColor(oldTextColor, oldTextBgColor);
    tft.setTextSize(oldTextSize);
  }
}

// Improved drawPersianText that handles numbers LTR within RTL text
void drawPersianText(const char* text, int x, int y, int fontSize, 
                     uint16_t color, uint16_t bgColor = 0xFFFF) {
  if (!text || fontSize < 1) return;
  
  int currentX = x; // Start from right edge
  
  // Process string from left to right (in buffer) but draw RTL
  // We need to handle numbers and symbols as LTR within RTL context
  
  int i = 0;
  while (text[i] != '\0') {
    char c = text[i];
    const Glyph* glyph = getGlyphForChar(c);
    
    if (glyph) {
      // Draw Persian glyph
      drawGlyph(glyph, currentX - (glyph->width * fontSize) + fontSize, y, 
                fontSize, color, bgColor);
      
      // Move left by actual scaled width
      currentX -= (glyph->width * fontSize);
      i++;
    } else if (c == ' ') {
      // Handle space: move left by space width
      currentX -= (3 * fontSize);
      i++;
    } else {
      // Regular character (number, symbol, etc.) - check if it's part of a sequence
      bool isNumberOrSymbol = (c >= '0' && c <= '9') || 
                             (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || 
                              c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                              c == '_' || c == '-' || c == '+' || c == '=' ||
                              c == ':' || c == '"' || c == '<' ||
                              c == '>' || c == '?' || c == '`' || c == '~' || c == '|');
      
      if (isNumberOrSymbol) {
        // Collect the sequence of numbers/symbols
        String sequence = "";
        while (text[i] != '\0') {
          char seqChar = text[i];
          bool isSeqNumberOrSymbol = (seqChar >= '0' && seqChar <= '9') || 
                                    (seqChar == '!' || seqChar == '@' || seqChar == '#' || 
                                     seqChar == '$' || seqChar == '%' || seqChar == '^' || 
                                     seqChar == '&' || seqChar == '*' || seqChar == '(' || 
                                     seqChar == ')' || seqChar == '_' || seqChar == '-' || 
                                     seqChar == '+' || seqChar == '=' ||
                                     seqChar == ':' || seqChar == '"' || seqChar == '<' || 
                                     seqChar == '>' || seqChar == '?' || seqChar == '`' || 
                                     seqChar == '~' || seqChar == '|');
          
          if (isSeqNumberOrSymbol) {
            sequence += seqChar;
            i++;
          } else {
            break;
          }
        }
        
        // Draw the sequence LTR (from left to right)
        int seqWidth = sequence.length() * (6 * fontSize);
        
        // Move to the start position for the sequence
        currentX -= seqWidth;
        int seqStartX = currentX;
        
        // Save current text settings
        uint16_t oldTextColor = tft.textcolor;
        uint16_t oldTextBgColor = tft.textbgcolor;
        uint8_t oldTextSize = tft.textsize;
        
        // Set up for drawing
        tft.setTextColor(color, bgColor);
        tft.setTextSize(fontSize);
        
        // Draw each character in the sequence LTR
        for (int j = 0; j < sequence.length(); j++) {
          tft.setCursor(seqStartX + (j * (6 * fontSize)), y);
          tft.print(sequence[j]);
        }
        
        // Restore text settings
        tft.setTextColor(oldTextColor, oldTextBgColor);
        tft.setTextSize(oldTextSize);
      } else {
        // Other character (like English letter in Persian mode) - draw as is
        drawPersianChar(c, currentX, y, fontSize, color, bgColor);
        i++;
      }
    }
  }
}

// ==============================================
// MAIN APPLICATION CODE
// ==============================================

// Message structure for display
struct DisplayMessage {
    String text;
    bool fromSerial;
    bool isContinued;
    bool isPersian;
    unsigned long timestamp;
};

#define MAX_MESSAGES 200
DisplayMessage displayMessages[MAX_MESSAGES];
int displayMessageCount = 0;
int scrollOffset = 0;
int maxScrollOffset = 0;
bool autoScroll = true;

// Typing buffer with horizontal scrolling
String typingBuffer = "";
int cursorPosition = 0;
int typingScrollOffset = 0;

// Current message being composed (for display)
String currentMessageForDisplay = "";
bool isComposingMessage = false;

// Language mode
bool persianMode = false;

// Display parameters for 180° rotation
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Chat area
#define CHAT_AREA_X 5
#define CHAT_AREA_Y 30
#define CHAT_AREA_WIDTH 310
#define CHAT_AREA_HEIGHT 140
#define MAX_DISPLAY_LINES 7
#define LINE_HEIGHT 20
#define CHARS_PER_LINE 21

// Typing area
#define TYPING_AREA_X 5
#define TYPING_AREA_Y 175
#define TYPING_AREA_WIDTH 310
#define TYPING_VISIBLE_WIDTH 270
#define TYPING_AREA_HEIGHT 25
#define TYPING_MAX_CHARS 500
#define CHAR_WIDTH 12
#define PROMPT_WIDTH 20

// Scroll buttons (vertical)
#define SCROLL_UP_X 295
#define SCROLL_UP_Y 35
#define SCROLL_DOWN_X 295
#define SCROLL_DOWN_Y 155
#define SCROLL_BTN_SIZE 10

// Typing scroll indicators (horizontal)
#define TYPING_SCROLL_LEFT_X 5
#define TYPING_SCROLL_LEFT_Y 183
#define TYPING_SCROLL_RIGHT_X 303
#define TYPING_SCROLL_RIGHT_Y 183
#define TYPING_SCROLL_SIZE 6

// UI Colors
uint16_t bgColor = TFT_BLACK;
uint16_t headerColor = TFT_CYAN;
uint16_t borderColor = TFT_WHITE;
uint16_t youColor = TFT_GREEN;
uint16_t serialColor = TFT_MAGENTA;
uint16_t typingColor = TFT_YELLOW;
uint16_t cursorColor = TFT_YELLOW;
uint16_t infoColor = TFT_LIGHTGREY;
uint16_t scrollBtnColor = TFT_DARKGREY;
uint16_t scrollBtnActive = TFT_WHITE;
uint16_t scrollBtnInactive = 0x4228;
uint16_t modeIndicatorColor = TFT_ORANGE;

// State tracking
bool shiftPressed = false;
bool lastShiftState = false;
bool needsDisplayUpdate = true;
unsigned long lastCursorBlink = 0;
bool cursorVisible = true;

// Serial input buffer
String serialInputBuffer = "";

// Function prototypes
String encryptMessage(const String &plaintext);
String decryptMessage(const String &encryptedHex);
void addMessageToDisplay(const String& text, bool fromSerial, bool isPersian = false);
bool containsPersianTokens(const String& str);

// Convert HID keycode to ASCII or Persian token - FIXED VERSION
char keycodeToChar(uint8_t keycode, uint8_t modifiers) {
    bool isShift = modifiers & 0x02;
    
    shiftPressed = (modifiers & 0x02) || (modifiers & 0x20);
    
    // Arrow keys
    if (keycode == 0x50) return 0x81; // Left arrow
    if (keycode == 0x4F) return 0x82; // Right arrow
    if (keycode == 0x52) return 0x83; // Up arrow
    if (keycode == 0x51) return 0x84; // Down arrow
    
    // Function keys
    if (keycode == 58) return 0xF1; // F1 - Clear chat
    if (keycode == 59) return 0xF2; // F2 - Clear typing
    if (keycode == 60) return 0xF3; // F3 - Toggle Persian mode
    
    // Handle Persian special characters FIRST
    // if (persianMode) {
    //     // Special Persian characters that need specific handling
    //     if (keycode == 47) { // [ key
    //         return isShift ? '[' : '['; // Keep as [ for Persian mapping
    //     }
    //     if (keycode == 48) { // ] key
    //         return isShift ? ']' : ']'; // Keep as ] for Persian mapping
    //     }
    //     if (keycode == 49) { // \ key
    //         return isShift ? '\\' : '\\'; // Keep as \ for Persian mapping
    //     }
    //     if (keycode == 51) { // ; key
    //         return isShift ? ';' : ';'; // Keep as ; for Persian mapping
    //     }
    //     if (keycode == 52) { // ' key
    //         return isShift ? '\'' : '\''; // Keep as ' for Persian mapping
    //     }
    //     if (keycode == 54) { // , key
    //         return isShift ? ',' : ','; // Keep as , for Persian mapping
    //     }
    // }
    
    // COMMON KEYS - SHARED BY BOTH MODES
    // Numbers 1-9
    if (keycode >= 30 && keycode <= 38) {
        const char* shifted = "!@#$%^&*(";
        return isShift ? shifted[keycode - 30] : ('1' + (keycode - 30));
    }
    
    // Number 0
    if (keycode == 39) return isShift ? ')' : '0';
    
    // Space
    if (keycode == 44) return ' ';
    
    // Enter
    if (keycode == 40) {
        if (shiftPressed) {
            return 0xFE;  // Shift+Enter = send
        }
        return '\n';
    }
    
    // Backspace
    if (keycode == 42) return '\b';
    
    // Tab
    if (keycode == 43) return '\t';
    
    // Special characters - For English mode, handle normally
    // if (!persianMode) {
        if (keycode == 49) return isShift ? '|' : '\\';
        if (keycode == 47) return isShift ? '{' : '[';
        if (keycode == 48) return isShift ? '}' : ']';
        
        if (keycode == 54) return isShift ? '<' : ',';
        if (keycode == 52) return isShift ? '"' : '\'';
        if (keycode == 51) return isShift ? ':' : ';';
    // }
    
    // Other symbols that work in both modes
    if (keycode == 45) return isShift ? '_' : '-';
    if (keycode == 46) return isShift ? '+' : '=';
    if (keycode == 53) return isShift ? '~' : '`';
    if (keycode == 55) return isShift ? '>' : '.';
    if (keycode == 56) return isShift ? '?' : '/';
    // Now handle Persian vs English mode for letters
    if (keycode >= 4 && keycode <= 29) {
        if (persianMode) {
            // Persian mode mapping - use letters as tokens for Persian glyphs
            char baseChar = 'a' + (keycode - 4);
            if (isShift) {
                // Map specific capital letters for Persian characters
                switch(keycode) {
                    case 6: return 'C';  // Shift+c -> C for ژ
                    // Other capital letters can be handled here if needed
                    default: return baseChar; // Keep same char for others
                }
            }
            return baseChar;
        } else {
            // English mode (original mapping)
            char c = 'a' + (keycode - 4);
            return isShift ? (c - 32) : c;
        }
    }
    
    return 0;
}

// Helper function to detect if a string contains Persian tokens
bool containsPersianTokens(const String& str) {
    // Check if string contains Persian letter tokens (a-z for Persian mapping)
    // Persian messages will have sequences of these tokens
    int persianTokenCount = 0;
    int totalCharCount = 0;
    
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        // Skip whitespace and punctuation for detection
        if (c != ' ' && c != '\n' && c != '.' && c != ',' && c != '!' && c != '?') {
            totalCharCount++;
            
            // Check if character is a Persian token
            if ((c >= 'a' && c <= 'z') || c == 'A' || c == 'C') {
                persianTokenCount++;
            }
        }
    }
    
    // If we have at least 2 characters and more than 70% are Persian tokens, likely Persian
    if (totalCharCount >= 2) {
        if (persianTokenCount * 100 / totalCharCount > 70) {
            return true;
        }
    }
    
    return false;
}

// Add COMPLETE message to display
void addMessageToDisplay(const String& text, bool fromSerial, bool isPersian) {
    if (text.length() == 0) return;
    
    int startPos = 0;
    int newlinePos;
    
    do {
        newlinePos = text.indexOf('\n', startPos);
        String line;
        if (newlinePos == -1) {
            line = text.substring(startPos);
        } else {
            line = text.substring(startPos, newlinePos);
            startPos = newlinePos + 1;
        }
        
        if (line.length() == 0 && newlinePos == -1) break;
        
        // Handle long lines by splitting them for display
        while (line.length() > 0) {
            if (displayMessageCount < MAX_MESSAGES) {
                DisplayMessage msg;
                
                if (isPersian) {
                    // For Persian, split based on visible Persian characters
                    // Each Persian token maps to 1 Persian character
                    // Approx 21 Persian characters per line
                    int takeLen = min((int)line.length(), CHARS_PER_LINE);
                    msg.text = line.substring(0, takeLen);
                    msg.isPersian = true;
                } else {
                    // For English
                    int takeLen = min((int)line.length(), CHARS_PER_LINE);
                    msg.text = line.substring(0, takeLen);
                    msg.isPersian = false;
                }
                
                msg.fromSerial = fromSerial;
                msg.isContinued = false; // Always start new message as first line
                msg.timestamp = millis();
                
                displayMessages[displayMessageCount] = msg;
                displayMessageCount++;
                
                // Handle continuation lines
                if (line.length() > msg.text.length()) {
                    line = line.substring(msg.text.length());
                    
                    // Mark continuation lines
                    while (line.length() > 0 && displayMessageCount < MAX_MESSAGES) {
                        DisplayMessage contMsg;
                        
                        if (isPersian) {
                            int contTakeLen = min((int)line.length(), CHARS_PER_LINE - 1);
                            contMsg.text = line.substring(0, contTakeLen);
                        } else {
                            int contTakeLen = min((int)line.length(), CHARS_PER_LINE - 1);
                            contMsg.text = line.substring(0, contTakeLen);
                        }
                        
                        contMsg.fromSerial = fromSerial;
                        contMsg.isContinued = true; // Mark as continuation
                        contMsg.isPersian = isPersian;
                        contMsg.timestamp = millis();
                        
                        displayMessages[displayMessageCount] = contMsg;
                        displayMessageCount++;
                        
                        if (line.length() > contMsg.text.length()) {
                            line = line.substring(contMsg.text.length());
                        } else {
                            break;
                        }
                    }
                    break;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        if (newlinePos == -1) break;
    } while (true);
    
    // Update scroll limits
    maxScrollOffset = max(0, displayMessageCount - MAX_DISPLAY_LINES);
    
    // Auto-scroll to show newest message
    if (autoScroll) {
        scrollOffset = maxScrollOffset;
    }
    
    needsDisplayUpdate = true;
}

// Send typing buffer as ONE COMPLETE ENCRYPTED MESSAGE
void sendTypingBuffer() {
    if (typingBuffer.length() > 0) {
        // ENTIRE MESSAGE encrypted as ONE
        String encrypted = encryptMessage(typingBuffer);
        
        // Add COMPLETE message to display
        addMessageToDisplay(typingBuffer, false, persianMode);
        
        // Send SINGLE encrypted string via Serial
        Serial.println(encrypted);
        
        // RESET everything for next message
        typingBuffer = "";
        cursorPosition = 0;
        typingScrollOffset = 0;
        currentMessageForDisplay = "";
        isComposingMessage = false;
        
        needsDisplayUpdate = true;
    }
}

// Encrypt ENTIRE MESSAGE as ONE
String encryptMessage(const String &plaintext) {
    if (plaintext.length() == 0) return "";
    
    // Calculate padded length
    int paddedLen = ((plaintext.length() + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    uint8_t *paddedText = new uint8_t[paddedLen];
    uint8_t *ciphertext = new uint8_t[paddedLen];
    
    // Copy plaintext
    memcpy(paddedText, plaintext.c_str(), plaintext.length());
    
    // Add PKCS7 padding
    uint8_t padValue = BLOCK_SIZE - (plaintext.length() % BLOCK_SIZE);
    for (int i = plaintext.length(); i < paddedLen; i++) {
        paddedText[i] = padValue;
    }
    
    // Encrypt ENTIRE message
    for (int i = 0; i < paddedLen; i += BLOCK_SIZE) {
        aes256.encryptBlock(ciphertext + i, paddedText + i);
    }
    
    // Convert to hex string
    String hexString = "";
    for (int i = 0; i < paddedLen; i++) {
        if (ciphertext[i] < 0x10) hexString += "0";
        hexString += String(ciphertext[i], HEX);
    }
    
    delete[] paddedText;
    delete[] ciphertext;
    
    return hexString;
}

// Decrypt ENTIRE MESSAGE
String decryptMessage(const String &encryptedHex) {
    if (encryptedHex.length() == 0) return "";
    
    // Convert hex to bytes
    int hexLen = encryptedHex.length();
    int byteLen = hexLen / 2;
    uint8_t *ciphertext = new uint8_t[byteLen];
    uint8_t *plaintext = new uint8_t[byteLen];
    
    for (int i = 0; i < byteLen; i++) {
        char hex[3];
        hex[0] = encryptedHex.charAt(i * 2);
        hex[1] = encryptedHex.charAt(i * 2 + 1);
        hex[2] = '\0';
        ciphertext[i] = strtoul(hex, NULL, 16);
    }
    
    // Decrypt ENTIRE message
    for (int i = 0; i < byteLen; i += BLOCK_SIZE) {
        aes256.decryptBlock(plaintext + i, ciphertext + i);
    }
    
    // Remove PKCS7 padding
    uint8_t padValue = plaintext[byteLen - 1];
    if (padValue > 0 && padValue <= BLOCK_SIZE) {
        byteLen -= padValue;
    }
    
    // Convert to string
    String result = "";
    for (int i = 0; i < byteLen; i++) {
        result += (char)plaintext[i];
    }
    
    delete[] ciphertext;
    delete[] plaintext;
    
    return result;
}

// Check if string is valid hex
bool isHexString(const String &str) {
    for (size_t i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (!((c >= '0' && c <= '9') || 
              (c >= 'a' && c <= 'f') || 
              (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

// Process serial input
void processSerialInput() {
    String s = "";
    char c;
    while (Serial.available()) {
        c = Serial.read();
        s.concat(c);
    }
    if(s != ""){
        if(isHexString(s)){
            String decrypted = decryptMessage(s);
            
            // Use the current persianMode setting for received messages
            addMessageToDisplay(decrypted, true, persianMode);
        } else {
            // Not encrypted hex, display as-is but use current mode
            addMessageToDisplay(s, true, persianMode);
        }
    }
}

// Draw the UI
void drawUI() {
    tft.fillScreen(bgColor);
    
    // Title
    tft.setTextColor(headerColor);
    tft.setTextSize(2);
    tft.setCursor(80, 5);
    tft.println("ENCRYPTED CHAT");
    
    // Language mode indicator
    tft.setTextColor(modeIndicatorColor);
    tft.setTextSize(1);
    tft.setCursor(280, 8);
    tft.print(persianMode ? "FA" : "EN");
    
    // Separator
    tft.drawFastHLine(5, 25, 310, borderColor);
    
    // Chat area border
    tft.drawRect(5, 30, 310, 140, TFT_WHITE);
    
    // Typing area border
    tft.drawRect(5, 175, 310, 25, TFT_YELLOW);
    
    // Footer info
    tft.setTextColor(infoColor);
    tft.setTextSize(1);
    tft.setCursor(10, 220);
    tft.print("Shift+Enter:Send  F3:Toggle Lang  Arrows:Navigate");
}

// Display chat messages
void displayChat() {
    // Clear chat area
    tft.fillRect(CHAT_AREA_X + 1, CHAT_AREA_Y + 1, CHAT_AREA_WIDTH - 2, CHAT_AREA_HEIGHT - 2, bgColor);
    
    // Show messages based on scroll offset
    int startIdx = scrollOffset;
    int displayLine = 0;
    
    for (int i = startIdx; i < displayMessageCount && displayLine < MAX_DISPLAY_LINES; i++) {
        int yPos = CHAT_AREA_Y + 3 + (displayLine * LINE_HEIGHT);
        
        uint16_t textColor;
        if (displayMessages[i].fromSerial) {
            textColor = serialColor;
        } else {
            textColor = youColor;
        }
        
        // Draw prefix
        tft.setTextColor(textColor);
        tft.setTextSize(2);
        
        if (!displayMessages[i].isContinued) {
            tft.setCursor(CHAT_AREA_X + 8, yPos);
            if (displayMessages[i].fromSerial) {
                tft.print("S: ");
            } else {
                tft.print("Y: ");
            }
        } else {
            tft.setCursor(CHAT_AREA_X + 8 + 60, yPos);
        }
        
        String displayText = displayMessages[i].text;
        
        // Draw message text
        if (displayMessages[i].isPersian) {
            // Draw Persian text using the improved drawPersianText function
            // Calculate starting X position (right-aligned for RTL)
            int textX = CHAT_AREA_X + CHAT_AREA_WIDTH - 10; // 10px from right edge
            
            // For non-continued lines, adjust for prefix
            if (!displayMessages[i].isContinued) {
                textX -= 40; // Make room for "S: " or "Y: "
            }
            
            // Draw the Persian text
            drawPersianText(displayText.c_str(), textX, yPos, 2, textColor, bgColor);
        } else {
            // Draw English text
            if (displayMessages[i].isContinued) {
                tft.setCursor(CHAT_AREA_X + 8 + 60, yPos);
            }
            tft.print(displayText);
        }
        
        displayLine++;
    }
    
    // Draw scroll buttons
    uint16_t upColor = scrollOffset > 0 ? scrollBtnActive : scrollBtnInactive;
    tft.fillTriangle(
        SCROLL_UP_X, SCROLL_UP_Y + SCROLL_BTN_SIZE,
        SCROLL_UP_X + SCROLL_BTN_SIZE/2, SCROLL_UP_Y,
        SCROLL_UP_X + SCROLL_BTN_SIZE, SCROLL_UP_Y + SCROLL_BTN_SIZE,
        upColor
    );
    
    uint16_t downColor = scrollOffset < maxScrollOffset ? scrollBtnActive : scrollBtnInactive;
    tft.fillTriangle(
        SCROLL_DOWN_X, SCROLL_DOWN_Y,
        SCROLL_DOWN_X + SCROLL_BTN_SIZE/2, SCROLL_DOWN_Y + SCROLL_BTN_SIZE,
        SCROLL_DOWN_X + SCROLL_BTN_SIZE, SCROLL_DOWN_Y,
        downColor
    );
}

// Display typing area with proper RTL/LTR handling
void displayTyping() {
    // Clear typing area
    tft.fillRect(TYPING_AREA_X + 1, TYPING_AREA_Y + 1, TYPING_AREA_WIDTH - 2, TYPING_AREA_HEIGHT - 2, bgColor);
    
    // Typing prompt
    tft.setTextColor(typingColor);
    tft.setTextSize(2);
    tft.setCursor(TYPING_AREA_X + 10, TYPING_AREA_Y + 5);
    tft.print("> ");
    
    if (persianMode) {
        // Draw Persian text in typing area using the improved function
        // Start from right edge of typing area for RTL
        int textX = TYPING_AREA_X + TYPING_AREA_WIDTH - 5; // 5px from right edge
        
        // Draw the Persian text (including numbers in LTR)
        drawPersianText(typingBuffer.c_str(), textX, TYPING_AREA_Y + 5, 2, typingColor, bgColor);
        
        // Calculate cursor position
        if (cursorVisible) {
            int cursorX = TYPING_AREA_X + 30; // Default after prompt
            
            // Calculate width of text before cursor position
            int totalWidth = 0;
            for (int i = 0; i < cursorPosition; i++) {
                char c = typingBuffer.charAt(i);
                const Glyph* glyph = getGlyphForChar(c);
                
                if (glyph) {
                    totalWidth += (glyph->width * 2); // Persian glyph width
                } else if (c == ' ') {
                    totalWidth += 6; // Space width
                } else {
                    totalWidth += 12; // Regular character width (numbers, symbols)
                }
            }
            
            cursorX = TYPING_AREA_X + 10 + 24 + totalWidth; // 24 = width of "> " prompt
            
            int cursorY = TYPING_AREA_Y + 20;
            tft.fillRect(cursorX, cursorY, 8, 3, cursorColor);
        }
    } else {
        // Display English text (original logic)
        int maxVisibleChars = (TYPING_VISIBLE_WIDTH - PROMPT_WIDTH) / CHAR_WIDTH;
        int bufferLength = typingBuffer.length();
        
        // Adjust scroll offset to keep cursor visible
        if (cursorPosition - typingScrollOffset >= maxVisibleChars - 1) {
            typingScrollOffset = cursorPosition - (maxVisibleChars - 2);
        } else if (cursorPosition < typingScrollOffset) {
            typingScrollOffset = cursorPosition;
        }
        
        // Ensure scroll offset stays within bounds
        if (typingScrollOffset < 0) typingScrollOffset = 0;
        
        // FIXED: Cast to int for max function
        int maxTypingScroll = max(0, (int)(bufferLength - maxVisibleChars + 1));
        
        if (typingScrollOffset > maxTypingScroll) {
            typingScrollOffset = maxTypingScroll;
        }
        
        // Display visible portion of typing buffer
        int displayStart = typingScrollOffset;
        int displayEnd = min(displayStart + maxVisibleChars, bufferLength);
        
        if (displayStart < displayEnd) {
            String visibleText = typingBuffer.substring(displayStart, displayEnd);
            tft.print(visibleText);
        }
        
        // Blinking cursor (only for English mode)
        if (cursorVisible) {
            int cursorX = TYPING_AREA_X + 10 + 14 + ((cursorPosition - typingScrollOffset) * CHAR_WIDTH);
            
            if (cursorPosition - typingScrollOffset < 0) {
                cursorX = TYPING_AREA_X + 10 + 14;
            }
            
            int cursorY = TYPING_AREA_Y + 20;
            tft.fillRect(cursorX, cursorY, 8, 3, cursorColor);
        }
        
        // Draw horizontal scroll indicators (only for English mode)
        int bufferLength2 = typingBuffer.length();
        int maxVisibleChars2 = (TYPING_VISIBLE_WIDTH - PROMPT_WIDTH) / CHAR_WIDTH;
        int maxTypingScroll2 = max(0, (int)(bufferLength2 - maxVisibleChars2 + 1));
        
        uint16_t leftColor = typingScrollOffset > 0 ? scrollBtnActive : scrollBtnInactive;
        tft.fillTriangle(
            TYPING_SCROLL_LEFT_X + TYPING_SCROLL_SIZE, TYPING_SCROLL_LEFT_Y + TYPING_SCROLL_SIZE/2,
            TYPING_SCROLL_LEFT_X, TYPING_SCROLL_LEFT_Y,
            TYPING_SCROLL_LEFT_X, TYPING_SCROLL_LEFT_Y + TYPING_SCROLL_SIZE,
            leftColor
        );
        
        uint16_t rightColor = typingScrollOffset < maxTypingScroll2 ? scrollBtnActive : scrollBtnInactive;
        tft.fillTriangle(
            TYPING_SCROLL_RIGHT_X, TYPING_SCROLL_RIGHT_Y + TYPING_SCROLL_SIZE/2,
            TYPING_SCROLL_RIGHT_X + TYPING_SCROLL_SIZE, TYPING_SCROLL_RIGHT_Y,
            TYPING_SCROLL_RIGHT_X + TYPING_SCROLL_SIZE, TYPING_SCROLL_RIGHT_Y + TYPING_SCROLL_SIZE,
            rightColor
        );
    }
}

// Handle scroll actions
void scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        autoScroll = false;
        needsDisplayUpdate = true;
    }
}

void scrollDown() {
    if (scrollOffset < maxScrollOffset) {
        scrollOffset++;
        if (scrollOffset == maxScrollOffset) {
            autoScroll = true;
        }
        needsDisplayUpdate = true;
    }
}

// Handle function keys
void handleFunctionKey(char funcKey) {
    switch (funcKey) {
        case 0xF1:
            displayMessageCount = 0;
            scrollOffset = 0;
            maxScrollOffset = 0;
            autoScroll = true;
            typingBuffer = "";
            cursorPosition = 0;
            typingScrollOffset = 0;
            currentMessageForDisplay = "";
            isComposingMessage = false;
            needsDisplayUpdate = true;
            Serial.println("All messages cleared");
            break;
            
        case 0xF2:
            typingBuffer = "";
            cursorPosition = 0;
            typingScrollOffset = 0;
            currentMessageForDisplay = "";
            isComposingMessage = false;
            needsDisplayUpdate = true;
            Serial.println("Typing buffer cleared");
            break;
            
        case 0xF3:
            persianMode = !persianMode;
            needsDisplayUpdate = true;
            Serial.print("Language mode: ");
            Serial.println(persianMode ? "Persian" : "English");
            break;
    }
}

// USB Keyboard Handler
class MyEspUsbHostKeybord : public EspUsbHostKeybord {
private:
    unsigned long lastKeyTime = 0;
    
public:
    void onKey(usb_transfer_t *transfer) {
        uint8_t *p = transfer->data_buffer;
        uint8_t modifiers = p[0];
        
        bool currentShiftState = (modifiers & 0x02) || (modifiers & 0x20);
        if (currentShiftState != lastShiftState) {
            shiftPressed = currentShiftState;
            lastShiftState = currentShiftState;
        }
        
        if (millis() - lastKeyTime < 50) return;
        
        for (int i = 2; i < 8; i++) {
            if (p[i] != 0) {
                char c = keycodeToChar(p[i], modifiers);
                
                if (c != 0) {
                    switch (c) {
                        case 0x81:
                            if (cursorPosition > 0) {
                                cursorPosition--;
                                needsDisplayUpdate = true;
                            }
                            break;
                            
                        case 0x82:
                            if (cursorPosition < typingBuffer.length()) {
                                cursorPosition++;
                                needsDisplayUpdate = true;
                            }
                            break;
                            
                        case 0x83:
                            scrollUp();
                            break;
                            
                        case 0x84:
                            scrollDown();
                            break;
                            
                        case 0xFE:
                            sendTypingBuffer();
                            autoScroll = true;
                            break;
                            
                        case 0xF1:
                        case 0xF2:
                        case 0xF3:
                            handleFunctionKey(c);
                            break;
                            
                        case '\b':
                            if (cursorPosition > 0 && typingBuffer.length() > 0) {
                                typingBuffer.remove(cursorPosition - 1, 1);
                                cursorPosition--;
                                needsDisplayUpdate = true;
                            }
                            break;
                            
                        case '\n':
                            typingBuffer += '\n';
                            cursorPosition = typingBuffer.length();
                            needsDisplayUpdate = true;
                            break;
                            
                        case '\t':
                            typingBuffer += "    ";
                            cursorPosition = typingBuffer.length();
                            needsDisplayUpdate = true;
                            break;
                            
                        default:
                            if (typingBuffer.length() < TYPING_MAX_CHARS) {
                                typingBuffer += c;
                                cursorPosition = typingBuffer.length();
                                needsDisplayUpdate = true;
                            }
                            break;
                    }
                    
                    lastKeyTime = millis();
                }
            }
        }
    }
};

MyEspUsbHostKeybord usbHost;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 Encrypted Chat...");
    
    // Initialize AES
    aes256.setKey(key, KEY_SIZE);
    Serial.println("AES-256 initialized");
    
    // Initialize TFT with 180° rotation
    tft.init();
    tft.setRotation(3);  // 180° rotation
    tft.fillScreen(bgColor);
    
    // Initialize USB
    usbHost.begin();
    Serial.println("USB Host ready");
    
    // Add welcome messages
    addMessageToDisplay("System Ready", false, false);
    addMessageToDisplay("Type message & press Shift+Enter", false, false);
    addMessageToDisplay("Entire message encrypted as ONE", false, false);
    addMessageToDisplay("Press F3 to toggle Persian/English", false, false);
    
    // Draw initial UI
    drawUI();
    displayChat();
    displayTyping();
    
    delay(500);
    
    Serial.println("\n=== SYSTEM READY ===");
    Serial.println("Features:");
    Serial.println("1. ENTIRE message encrypted as ONE string");
    Serial.println("2. Only ONE encrypted hex string sent via Serial");
    Serial.println("3. New messages always start fresh (no continuation)");
    Serial.println("4. After sending, typing area resets completely");
    Serial.println("5. Press F3 to toggle between English/Persian");
    Serial.println("6. Persian letters mapped to English keyboard:");
    Serial.println("   a=ش, b=ذ, c=ز, d=ی, e=ث, f=ب, g=ل, h=ا, i=ه, j=ت");
    Serial.println("   k=ن, l=م, m=ک, n=د, o=خ, p=ح, q=ض, r=ق, s=س, t=ف");
    Serial.println("   u=ع, v=ر, w=ص, x=ط, y=غ, z=ظ");
    Serial.println("7. Special Persian characters:");
    Serial.println("   \\ = پ (backslash)");
    Serial.println("   [ = ژ (left bracket)");
    Serial.println("   ] = چ (right bracket)");
    Serial.println("   ; = گ (semicolon)");
    Serial.println("   ' = ک (apostrophe)");
    Serial.println("   , = و (comma)");
    Serial.println("   Shift+C = ژ (capital C)");
    Serial.println("8. NUMBERS AND SYMBOLS WORK IN BOTH MODES");
    Serial.println("9. In Persian mode: Numbers display LTR (12345) within RTL text");
}

void loop() {
    usbHost.task();
    processSerialInput();
    
    if (needsDisplayUpdate) {
        drawUI();
        displayChat();
        displayTyping();
        needsDisplayUpdate = false;
    }
    
    // Cursor blink for both modes
    if (millis() - lastCursorBlink > 500) {
        cursorVisible = !cursorVisible;
        displayTyping();
        lastCursorBlink = millis();
    }
    
    delay(10);
}