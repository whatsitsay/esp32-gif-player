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
#include <Arduino.h>
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

#define LCD_WIDTH  (320)
#define LCD_HEIGHT (240)
#define LCD_SIZE_B (LCD_WIDTH * LCD_HEIGHT * 2)

#define ERROR_WAIT for(;;)

// TFT module
TFT_eSPI tft = TFT_eSPI();
// SD SPI module
SPIClass sd_spi = SPIClass(VSPI);
// Gif decoder template. Hard code dimensions for now
// TODO: lzwMaxBits not defined, just part of constructor...
// is it meant to be the bitwidth of colors?
GifDecoder<LCD_WIDTH, LCD_HEIGHT, 16> decoder;
// Frame buffer
#define NUM_FRAME_BUFFS (2)
#define BUFF_HEIGHT (LCD_HEIGHT / NUM_FRAME_BUFFS)
#define BUFF_SIZE_B (LCD_WIDTH * BUFF_HEIGHT * 2)
uint16_t* frame_buff[NUM_FRAME_BUFFS];

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
  for (int i = 0; i < NUM_FRAME_BUFFS; i++) {
    tft.pushImage(0, i * BUFF_HEIGHT, LCD_WIDTH, BUFF_HEIGHT, frame_buff[i]);
  }
  delay(5);
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  // Set in frame buffer itself
  int buff_idx = y / BUFF_HEIGHT;
  int grid_idx = x + ((y % BUFF_HEIGHT) * LCD_WIDTH);
  frame_buff[buff_idx][grid_idx] = tft.color565(red, green, blue);
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

  // Initialize frame buffers
  for (int i = 0; i < NUM_FRAME_BUFFS; i++)
  {
    frame_buff[i] = (uint16_t *)calloc(BUFF_HEIGHT * LCD_WIDTH, 2);
    if (frame_buff[i] == NULL)
    {
      Serial.printf("Failed to allocate frame buffer %d\n", i);
      ERROR_WAIT;
    }

  }

  // Setup TFT
  tft.init();
  tft.setSwapBytes(true);

  // Enable backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Enable and check SD card
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sd_spi, 80000000))
  {
    Serial.println("Card mount failed, check pins!");
    ERROR_WAIT;
  }

  // Print SD card stats
  uint8_t cardType = SD.cardType();
 
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    ERROR_WAIT;
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
  tft.setRotation(3); // Double-check correct orientation
  tft.fillScreen(ILI9341_BLACK);

  // Only playing single gif, so load that as well
  int num_files = enumerateGIFFiles(gif_path, true);
  if (num_files <= 0) {
    Serial.printf("ERROR: No gif files present! Num files = %d", num_files);
    // Wait forever
    ERROR_WAIT;
  }

  // Open gif file
  if (openGifFilenameByIndex(gif_path, 0) < 0)
  {
    Serial.println("ERROR: Failed to open gif file!");
    // Wait forever
    ERROR_WAIT;
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