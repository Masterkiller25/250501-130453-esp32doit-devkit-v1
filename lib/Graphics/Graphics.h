#ifndef Graphics_H_ // Include guard
#define Graphics_H_

#include "Arduino.h"
#include "JPEGDecoder.h"
#include "TFT_eSPI.h"
#include "File_essantial.h"



void begin(uint8_t coos[16][2], uint8_t colors[0][3], File_essantial files[255]);
bool test();

void clear_screen();

size_t println(const char *str);
size_t println(const char *str, int16_t x, int16_t y);
void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void drawdir(const int &i);
void drawfile(const int &i);
void writename(const int &i_screen, const int &i_file);
void drawsepar(const int &i);
void drawselect(const int &max);
void drawselected(const int &i);

void printflc(const int &c);
void printflc(const int & c, int16_t x, int16_t y);

int calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print);
void jpegRender(const int &xpos, const int &ypos);

#endif