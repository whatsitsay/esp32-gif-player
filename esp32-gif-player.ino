/**
 * @brief GIF player using ILI9340c TFT
 * 
 * Code based on XTronical JPEG player example and GifDecoder
 * SmartMatrixGifPlayer example
 * XTronical example: https://www.xtronical.com/esp32ili9341/
 * GifPlayer example: https://github.com/pixelmatix/GifDecoder/blob/master/examples/SmartMatrixGifPlayer/SmartMatrixGifPlayer.ino
 * 
 */
#include <string.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <User_Config.h>
#include <GifDecoder.h>

// Local filename functions
#include "FilenameFunctions.h"

#define SD_SCK    (TFT_SCLK)
#define SD_MOSI   (TFT_MOSI)
#define SD_MISO   (TFT_MISO)
#define SD_CS     (4)

// TFT module
TFT_eSPI tft = TFT_eSPI();
// SD SPI module
SPIClass sd_spi = SPIClass(VSPI);
// Gif decoder template. Hard code dimensions for now
// TODO: lzwMaxBits not defined, just part of constructor...
// is it meant to be the bitwidth of colors?
GifDecoder<320, 240, 12, true> decoder;
static unsigned update_screen_call = 0;

// Gif path
File gif_dir;
const char* gif_path = "/gif";
//####################################################################################################
// Callback functions
//####################################################################################################
void screenClearCallback(void)
{
  // Fillscreen with black
  tft.fillScreen(ILI9341_BLACK);
}

void updateScreenCallback(void)
{
  // Do nothing but increment counter for now
  update_screen_call++;
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  tft.drawPixel(x, y, tft.color565(red, green, blue));
}

//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);

  // Set callbacks for Gif decoder
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);

  decoder.setFileSizeCallback(fileSizeCallback);

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
  tft.setRotation(1); // Double-check correct orientation
  tft.fillScreen(ILI9341_BLACK);

  // Only playing single gif, so load that as well
  int num_files = enumerateGIFFiles(gif_path, true);
  if (num_files <= 0) {
    Serial.printf("ERROR: No gif files present! Num files = %d", num_files);
    // Wait forever
    while(1);
  }

  // Open gif file
  if (openGifFilenameByIndex(gif_path, 0) < 0)
  {
    Serial.println("ERROR: Failed to open gif file!");
    // Wait forever
    while(1);
  }
}
//####################################################################################################
// Main loop
//####################################################################################################
void loop() {

  // Just keep decoding the same file
  if (decoder.startDecoding() >= 0) {
    // Decode frame
    int result;
    do {
      result = decoder.decodeFrame(false);
    } while (result >= 0);
  }
  else {
    Serial.println("WARNING: Error decoding gif");
  }
}