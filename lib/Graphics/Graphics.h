#ifndef Graphics_H_		// Include guard
#define Graphics_H_

#include "Arduino.h"

void begin(uint8_t **coos_, uint8_t **colors_, class File_essantial *files_);
void drawdir(const int &i);
void drawfile(const int &i);
void writename(const int &i_screen, const int &i_file);
void drawsepar(const int &i);
void drawselect(const int &max);
void drawselected(const int &i);

#endif