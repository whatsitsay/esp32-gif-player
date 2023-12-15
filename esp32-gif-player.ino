/**
 * @brief GIF player using ILI9340c TFT
 * 
 * Pulled code from XTronical example: https://www.xtronical.com/esp32ili9341/
 * 
 */
#include <string.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <User_Config.h>
#include <JPEGDecoder.h>

#define SD_SCK    (TFT_SCLK)
#define SD_MOSI   (TFT_MOSI)
#define SD_MISO   (TFT_MISO)
#define SD_CS     (4)

#define GIF_FRAME_TIME_MS (33) // For 30 FPS

// TFT module
TFT_eSPI tft = TFT_eSPI();
// SD SPI module
SPIClass sd_spi = SPIClass(VSPI);

// Root path
File root;
const char* root_path = "/split_gif";

//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);

  // Setup TFT
  tft.init();

  // Enable backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Enable and check SD card
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sd_spi, 40000000))
  {
    Serial.println("Card mount failed, check pins!");
    return;
  }

  // Print SD card stats
  uint8_t cardType = SD.cardType();
 
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
 
  Serial.print("SD Card Type: ");
  switch (cardType) {
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SDSC");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("UNKNOWN");
  }
 
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.println("Completed initialisation");

  // Rotate and set to black
  tft.setRotation(0); // Double-check correct orientation
  tft.fillScreen(ILI9341_BLACK);

  // Load root path
  root = SD.open(root_path);
  if(!root){
      Serial.println("Failed to open root directory");
      return;
  }
}
//####################################################################################################
// Main loop
//####################################################################################################
void loop() {
  File file = root.openNextFile();  // Opens next file in root
  char full_path[50];
  while(file)
  {
    if(!file.isDirectory())
    {
      unsigned long start_time_ms = millis();
      // Clear full path buffer
      memset(full_path, 0, sizeof(full_path));
      // Concatenate file name with full path
      strcpy(full_path, root_path);
      strcat(full_path, "/"); // Path separator
      strcat(full_path, file.name());
      drawSdJpeg(full_path, 0, 0);     // This draws a jpeg pulled off the SD Card
      int draw_time = (int)(millis() - start_time_ms);

      // If any time left in frame, delay for that amount
      if (draw_time < GIF_FRAME_TIME_MS)
      delay(GIF_FRAME_TIME_MS - draw_time);
    }
    file = root.openNextFile();  // Opens next file in root
  };

  // Rewind path for next iteration
  root.rewindDirectory();
}
 
//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {
 
  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }
 
  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");
 
  // Use one of the following methods to initialise the decoder:
  boolean decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //boolean decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)
 
  if (decoded) {
    // print information about the image to the serial port
    // jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("ERROR: Jpeg file format not supported!");
  }
}
 
//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {
 
  //jpegInfo(); // Print information from the JPEG file (could comment this line out)
 
  uint16_t *pImg;
  uint32_t mcu_w = JpegDec.MCUWidth;
  uint32_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;
 
  // Set byte swap
  bool bytesSwapped = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = min(mcu_w, max_x % mcu_w);
  uint32_t min_h = min(mcu_h, max_y % mcu_h);
 
  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
 
  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;
 
  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
 
    // Calculate coordinates of top left corner of current MCU
    uint32_t mcu_x = JpegDec.MCUx * mcu_w + xpos;
    uint32_t mcu_y = JpegDec.MCUy * mcu_h + ypos;
 
    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
 
    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;
 
    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      uint32_t p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }
 
    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;
 
    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // Reset byte swap
  tft.setSwapBytes(bytesSwapped);
}
 
//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {
 
  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}
 
//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16 bit address pointers are used in some libraries.
 
// The Due will work fine with the HX8357_Due library.
 
void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}