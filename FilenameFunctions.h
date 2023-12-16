/*
 * Animated GIFs Display jeader
 * Originally for 32x32 smart matrix display
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 * Source: https://github.com/pixelmatix/GifDecoder/blob/master/examples/SmartMatrixGifPlayer/FilenameFunctions.h
 */

#ifndef FILENAME_FUNCTIONS_H
#define FILENAME_FUNCTIONS_H

int enumerateGIFFiles(const char *directoryName, bool displayFilenames);
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
int openGifFilenameByIndex(const char *directoryName, int index);
int initFileSystem(int chipSelectPin);

bool fileSeekCallback(unsigned long position);
unsigned long filePositionCallback(void);
int fileReadCallback(void);
int fileReadBlockCallback(void * buffer, int numberOfBytes);
int fileSizeCallback(void);

#endif