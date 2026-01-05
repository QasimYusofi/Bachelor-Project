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

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// Message storage
#define MAX_MESSAGES 50
String messages[MAX_MESSAGES];           // Display messages (on TFT)
bool messageFromSerial[MAX_MESSAGES];    // True if from serial monitor (false = from keyboard)
unsigned long messageTimestamps[MAX_MESSAGES];
int messageCount = 0;

// Typing buffer
String typingBuffer = "";
int cursorPosition = 0;

// Display parameters
#define MAX_DISPLAY_LINES 8
#define CHARS_PER_LINE 25
#define LINE_HEIGHT 20
#define CHAT_AREA_X 10
#define CHAT_AREA_Y 40
#define TYPING_AREA_X 10
#define TYPING_AREA_Y 200
#define AREA_WIDTH 300
#define CHAT_AREA_HEIGHT 150
#define TYPING_AREA_HEIGHT 30

// UI Colors
uint16_t bgColor = TFT_BLACK;
uint16_t headerColor = TFT_CYAN;
uint16_t borderColor = TFT_WHITE;
uint16_t youColor = TFT_GREEN;
uint16_t serialColor = TFT_MAGENTA;
uint16_t typingColor = TFT_YELLOW;
uint16_t cursorColor = TFT_YELLOW;
uint16_t infoColor = TFT_LIGHTGREY;

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

// Convert HID keycode to ASCII
char keycodeToChar(uint8_t keycode, uint8_t modifiers) {
    bool isShift = modifiers & 0x02;
    
    shiftPressed = (modifiers & 0x02) || (modifiers & 0x20);
    
    // Letters A-Z
    if (keycode >= 4 && keycode <= 29) {
        char c = 'a' + (keycode - 4);
        return isShift ? (c - 32) : c;
    }
    
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
    
    // Special characters
    if (keycode == 45) return isShift ? '_' : '-';
    if (keycode == 46) return isShift ? '+' : '=';
    if (keycode == 47) return isShift ? '{' : '[';
    if (keycode == 48) return isShift ? '}' : ']';
    if (keycode == 49) return isShift ? '|' : '\\';
    if (keycode == 51) return isShift ? ':' : ';';
    if (keycode == 52) return isShift ? '"' : '\'';
    if (keycode == 53) return isShift ? '~' : '`';
    if (keycode == 54) return isShift ? '<' : ',';
    if (keycode == 55) return isShift ? '>' : '.';
    if (keycode == 56) return isShift ? '?' : '/';
    
    // Function keys
    if (keycode == 58) return 0xF1; // F1 - Clear chat
    if (keycode == 59) return 0xF2; // F2 - Clear typing
    
    return 0;
}

// Add message to chat (for display on TFT)
void addMessage(const String& text, bool fromSerial) {
    if (text.length() == 0) return;
    
    // Add to messages array
    if (messageCount < MAX_MESSAGES) {
        messages[messageCount] = text;
        messageFromSerial[messageCount] = fromSerial;
        messageTimestamps[messageCount] = millis();
        messageCount++;
    } else {
        // Shift old messages out
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            messages[i] = messages[i + 1];
            messageFromSerial[i] = messageFromSerial[i + 1];
            messageTimestamps[i] = messageTimestamps[i + 1];
        }
        messages[MAX_MESSAGES - 1] = text;
        messageFromSerial[MAX_MESSAGES - 1] = fromSerial;
        messageTimestamps[MAX_MESSAGES - 1] = millis();
    }
    
    needsDisplayUpdate = true;
 
}

// Send typing buffer as message
void sendTypingBuffer() {
    if (typingBuffer.length() > 0) {
        // Encrypt the message
        String encrypted = encryptMessage(typingBuffer);
        
        // Add to display
        addMessage(typingBuffer, false);
        
        // Print encrypted version to Serial
        Serial.println(encrypted);
        
        // Clear typing buffer
        typingBuffer = "";
        cursorPosition = 0;
        needsDisplayUpdate = true;
    }
}

// Encrypt a message using AES-256
String encryptMessage(const String &plaintext) {
    if (plaintext.length() == 0) return "";
    
    // Pad plaintext to multiple of 16 bytes
    int paddedLen = ((plaintext.length() + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    uint8_t *paddedText = new uint8_t[paddedLen];
    uint8_t *ciphertext = new uint8_t[paddedLen];
    
    // Copy plaintext and add PKCS7 padding
    memcpy(paddedText, plaintext.c_str(), plaintext.length());
    uint8_t padValue = BLOCK_SIZE - (plaintext.length() % BLOCK_SIZE);
    for (int i = plaintext.length(); i < paddedLen; i++) {
        paddedText[i] = padValue;
    }
    
    // Encrypt each block
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

// Decrypt a hex string using AES-256
String decryptMessage(const String &encryptedHex) {
    if (encryptedHex.length() == 0) return "";
    
    // Convert hex string to bytes
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
    
    // Decrypt each block
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
            s = decryptMessage(s);
        }
        addMessage(s,true);
    }
    
}

// Draw the UI
void drawUI() {
    tft.fillScreen(bgColor);
    
    // Header
    tft.setTextColor(headerColor);
    tft.setTextSize(2);
    tft.setCursor(10, 5);
    tft.println("ESP32 ENCRYPTED CHAT");
    
    // Separator
    tft.drawFastHLine(5, 30, 310, borderColor);
    
    // Chat area border
    tft.drawRect(5, 35, 310, CHAT_AREA_HEIGHT, TFT_WHITE);
    
    // Typing area border
    tft.drawRect(5, 195, 310, TYPING_AREA_HEIGHT, TFT_YELLOW);
    
    // Footer
    tft.setTextColor(infoColor);
    tft.setTextSize(1);
    tft.setCursor(10, 240 - 15);
    tft.print("Shift+Enter:Send & Encrypt  Serial:115200");
}

// Display chat messages on TFT
void displayChat() {
    // Clear chat area
    tft.fillRect(CHAT_AREA_X, CHAT_AREA_Y, AREA_WIDTH, CHAT_AREA_HEIGHT, bgColor);
    
    // Show most recent messages
    int startIdx = max(0, messageCount - MAX_DISPLAY_LINES);
    int displayLine = 0;
    
    for (int i = startIdx; i < messageCount && displayLine < MAX_DISPLAY_LINES; i++) {
        int yPos = CHAT_AREA_Y + (displayLine * LINE_HEIGHT);
        
        // Set color based on source
        if (messageFromSerial[i]) {
            tft.setTextColor(serialColor);
            tft.setCursor(CHAT_AREA_X, yPos);
            tft.print("serial: ");
        } else {
            tft.setTextColor(youColor);
            tft.setCursor(CHAT_AREA_X, yPos);
            tft.print("you: ");
        }
        
        // Display message (first line only)
        tft.setTextSize(2);
        String displayText = messages[i];
        
        // Handle long messages - show first line only
        int newlinePos = displayText.indexOf('\n');
        if (newlinePos != -1) {
            displayText = displayText.substring(0, newlinePos);
        }
        
        // Truncate if too long
        if (displayText.length() > CHARS_PER_LINE - 8) {
            displayText = displayText.substring(0, CHARS_PER_LINE - 11);
            displayText += "...";
        }
        
        tft.print(displayText);
        displayLine++;
    }
}

// Display typing area
void displayTyping() {
    // Clear typing area
    tft.fillRect(TYPING_AREA_X, TYPING_AREA_Y, AREA_WIDTH, TYPING_AREA_HEIGHT, bgColor);
    
    // Typing prompt
    tft.setTextColor(typingColor);
    tft.setTextSize(2);
    tft.setCursor(TYPING_AREA_X, TYPING_AREA_Y);
    tft.print("> ");
    
    // Typing buffer
    tft.print(typingBuffer);
    
    // Blinking cursor
    if (cursorVisible) {
        int cursorX = TYPING_AREA_X + 14 + (cursorPosition * 12);
        int cursorY = TYPING_AREA_Y + 15;
        tft.fillRect(cursorX, cursorY, 8, 3, cursorColor);
    }
}

// Handle function keys
void handleFunctionKey(char funcKey) {
    switch (funcKey) {
        case 0xF1: // Clear all messages
            messageCount = 0;
            needsDisplayUpdate = true;
            Serial.println("All messages cleared");
            break;
            
        case 0xF2: // Clear typing buffer
            typingBuffer = "";
            cursorPosition = 0;
            needsDisplayUpdate = true;
            Serial.println("Typing buffer cleared");
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
        
        // Update shift state
        bool currentShiftState = (modifiers & 0x02) || (modifiers & 0x20);
        if (currentShiftState != lastShiftState) {
            shiftPressed = currentShiftState;
            lastShiftState = currentShiftState;
        }
        
        // Debounce
        if (millis() - lastKeyTime < 50) return;
        
        for (int i = 2; i < 8; i++) {
            if (p[i] != 0) {
                char c = keycodeToChar(p[i], modifiers);
                
                if (c != 0) {
                    // Shift+Enter = send & encrypt message
                    if (c == 0xFE) {
                        sendTypingBuffer();
                    }
                    // Function keys
                    else if (c == 0xF1 || c == 0xF2) {
                        handleFunctionKey(c);
                    }
                    // Backspace
                    else if (c == '\b') {
                        if (cursorPosition > 0 && typingBuffer.length() > 0) {
                            typingBuffer.remove(cursorPosition - 1, 1);
                            cursorPosition--;
                            needsDisplayUpdate = true;
                        }
                    }
                    // Enter (new line in typing)
                    else if (c == '\n') {
                        typingBuffer += '\n';
                        cursorPosition = typingBuffer.length();
                        needsDisplayUpdate = true;
                    }
                    // Tab
                    else if (c == '\t') {
                        typingBuffer += "    ";
                        cursorPosition = typingBuffer.length();
                        needsDisplayUpdate = true;
                    }
                    // Printable characters
                    else {
                        typingBuffer += c;
                        cursorPosition = typingBuffer.length();
                        needsDisplayUpdate = true;
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
    
    // Initialize TFT
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(bgColor);
    
    // Show connecting message
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(50, 100);
    tft.print("Starting System...");
    
    // Initialize USB
    usbHost.begin();
    Serial.println("USB Host ready");
    
    // Add welcome messages
    addMessage("Shift+Enter to send", false);
    
    // Draw initial UI
    drawUI();
    displayChat();
    displayTyping();
    
    delay(1000);
    
    Serial.println("\n=== SYSTEM READY ===");
    Serial.println("1. Type on USB keyboard");
    Serial.println("2. Shift+Enter to encrypt & send");
    Serial.println("3. Encrypted message appears in Serial");
    Serial.println("4. Send plain text or encrypted hex via Serial");
    Serial.println("5. TFT shows decrypted messages");
}

void loop() {
    // Handle USB
    usbHost.task();
    
    // Handle Serial input
    processSerialInput();
    
    // Update display if needed
    if (needsDisplayUpdate) {
        displayChat();
        displayTyping();
        needsDisplayUpdate = false;
    }
    
    // Blinking cursor
    if (millis() - lastCursorBlink > 500) {
        cursorVisible = !cursorVisible;
        displayTyping();
        lastCursorBlink = millis();
    }
    
    delay(10);
}