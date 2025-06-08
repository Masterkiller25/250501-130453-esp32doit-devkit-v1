#include <Arduino.h>
#include <TFT_eSPI.h>
#include <File_essantial.h>
#include <JPEGDecoder.h> //include for Decoding JPEG file
#include <Graphics.h>

#define minimum(a, b) _min(a, b)
#define maximum(a, b) _max(a, b)

#define DEBUG

PROGMEM const unsigned char chr_flc_E286B0[16] = // 1 unsigned char per row
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x60, 0xf8, 0x68, 0x28, 0x08, // row 1 - 11
        0x08, 0x08, 0x08, 0x00, 0x00                                      // row 12 - 16
};
PROGMEM const unsigned char chr_flc_E286B1[16] = // 1 unsigned char per row
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x06, 0x1f, 0x16, 0x14, 0x10, // row 1 - 11
        0x10, 0x10, 0x10, 0x00, 0x00                                      // row 12 - 16
};
PROGMEM const unsigned char chr_flc_E286B2[16] = // 1 unsigned char per row
    {
        0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x28, 0x68, 0xf8, 0x60, 0x20, // row 1 - 11
        0x00, 0x00, 0x00, 0x00, 0x00                                      // row 12 - 16
};
PROGMEM const unsigned char chr_flc_E286B3[16] = // 1 unsigned char per row
    {
        0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x14, 0x16, 0x1f, 0x06, 0x04, // row 1 - 11
        0x00, 0x00, 0x00, 0x00, 0x00                                      // row 12 - 16
};

PROGMEM const unsigned char *const chrtbl_flc[4] =
    {
        chr_flc_E286B0, chr_flc_E286B1, chr_flc_E286B2, chr_flc_E286B3};

// Graphics graphics = Graphics();
// TFT_eSPI &tft = graphics.tft;
TFT_eSPI tft = TFT_eSPI();
uint8_t (*_coos)[2] = nullptr;
uint8_t (*_colors)[3] = nullptr;
File_essantial *_files = nullptr;

/**
 * @brief Setup variables
 *
 * @param coos Pointeur vers la matrice des coordonnées.
 * @param colors Pointeur vers la matrice des couleurs.
 * @param files Pointeur vers la liste des fichiers.
 * @return void
 */
void begin(uint8_t coos[16][2], uint8_t colors[0][3], File_essantial files[255])
{
#ifdef DEBUG
  if (coos && coos[0]) {
    Serial.println(coos[0][0]);
  } else {
    Serial.println(F("ERROR: coos is null"));
  }
#endif
  // _coos = (uint8_t *(*))&coos;
  // _colors = (uint8_t *(*))&colors;
  // _files = (File_essantial *)&files;
  _coos = coos;
  _colors = colors;
  _files = files;

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.println(F("Initaliser"));
}

bool test()
{
  Serial.print("_coos[0][0]=");
  Serial.println(_coos[0][0]);
  return true;
}

/**
 * @brief Remplit l'écran de noir.
 *
 * @return void
 */
void clear_screen()
{
  tft.fillScreen(0);
}

// add some tft function for the main scipt

size_t println(const char *str)
{
  return tft.println(str);
}

size_t println(const char *str, int16_t x, int16_t y)
{
  tft.setCursor(x, y);
  return tft.println(str);
}

void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  return tft.drawRect(x, y, w, h, color);
}

void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  return tft.fillRect(x, y, w, h, color);
}

/**
 * @brief Dessine une ligne entre les fichiers.
 *
 * @param i Index de l'endroit où il doit être.
 * @return void
 */
void drawsepar(const int &i)
{
#ifdef DEBUG
  Serial.print("[DEBUG] drawsepar(");
  Serial.print(i);
  Serial.println(")");
#endif
  if (!_coos)
  {
    Serial.println(F("ERROR: drawsepar: _coos is null"));
    return;
  }
  tft.drawFastHLine(5, 30 + i * 25, tft.width() - 10, TFT_WHITE);
}

/**
 * @brief Dessine le logo d'un dossier.
 *
 * @param i Index de l'endroit où il doit être.
 * @return void
 */
void drawdir(const int &i)
{
#ifdef DEBUG
  Serial.print("[DEBUG] drawdir(");
  Serial.print(i);
  Serial.println(")");
#endif
  if (!_coos)
  {
    Serial.println(F("ERROR: drawdir: _coos is null"));
    return;
  }
  tft.fillRoundRect(_coos[0][0], _coos[0][1] + i * 25, _coos[1][0], _coos[1][1], 3, tft.color565(200, 200, 50));
  tft.fillRoundRect(_coos[2][0], _coos[2][1] + i * 25, _coos[3][0], _coos[3][1], 3, tft.color565(200, 200, 50));
}

/**
 * @brief Dessine le logo d'un fichier.
 *
 * @param i Index de l'endroit où il doit être.
 * @return void
 */
void drawfile(const int &i)
{
#ifdef DEBUG
  Serial.print("[DEBUG] drawfile(");
  Serial.print(i);
  Serial.println(")");
#endif
  if (!_coos)
  {
    Serial.println(F("ERROR: drawfile: _coos is null"));
    return;
  }
  tft.fillRect(_coos[4][0], _coos[4][1] + i * 25, _coos[5][0], _coos[5][1], TFT_WHITE);
  tft.fillRect(_coos[6][0], _coos[6][1] + i * 25, _coos[7][0], _coos[7][1], TFT_WHITE);
  tft.fillTriangle(19, 9 + i * 25, 19, 14 + i * 25, 24, 14 + i * 25, TFT_WHITE);
}

/**
 * @brief Écrit le nom d'un fichier.
 *
 * @param i_screen Index de l'endroit où il doit être.
 * @param i_file Index du fichier.
 * @return void
 */
void writename(const int &i_screen, const int &i_file)
{
#ifdef DEBUG
  Serial.print("[DEBUG] writename(");
  Serial.print(i_screen);
  Serial.print(", ");
  Serial.print(i_file);
  Serial.println(")");
#endif
  if (!_files)
  {
    Serial.println(F("ERROR: writename: _files is null"));
    return;
  }
  tft.setCursor(40, 11 + i_screen * 25);
  if (_files[i_file].name.length() > 14)
  {
    for (int i = 0; i < 11; i++)
      tft.print(_files[i_file].name.charAt(i));
    tft.print(F("..."));
  }
  else
  {
    tft.print(_files[i_file].name.c_str());
  }
  drawsepar(i_screen);
}

/**
 * @brief Dessine une boîte à cocher.
 *
 * @param max Index maximum des endroits où elles doivent être.
 * @return void
 */
void drawselect(const int &max)
{
#ifdef DEBUG
  Serial.print("[DEBUG] drawselect(");
  Serial.print(max);
  Serial.println(")");
#endif
  if (!_coos)
  {
    Serial.println(F("ERROR: drawselect: _coos is null"));
    return;
  }
  for (int i = 0; i < max; i++)
  {
    tft.drawRect(210, 8 + i * 25, 20, 20, TFT_WHITE);
    tft.drawRect(211, 9 + i * 25, 18, 18, 0);
  }
}

/**
 * @brief Dessine un ✔ dans la boîte.
 *
 * @param i Index de l'endroit où il doit être.
 * @return void
 */
void drawselected(const int &i)
{
#ifdef DEBUG
  Serial.print("[DEBUG] drawselected(");
  Serial.print(i);
  Serial.println(")");
#endif
  if (!_coos)
  {
    Serial.println(F("ERROR: drawselected: _coos is null"));
    return;
  }
  tft.drawLine(215, 20 + i * 25, 219, 24 + i * 25, TFT_WHITE);
  tft.drawLine(219, 24 + i * 25, 224, 15 + i * 25, TFT_WHITE);
}

/**
 * @brief Affiche une flèche (↰↱↲↳).
 *
 * @param c Caractère de la flèche.
 * @return void
 */
void printflc(const int &c)
{
#ifdef DEBUG
  Serial.print("[DEBUG] printflc(");
  Serial.print(c);
  Serial.println(")");
#endif
  if (c < 0xE286B0 || c > 0xE286B3)
  {
    Serial.println(F("ERROR: printflc: Invalid character code"));
    return;
  }
  const unsigned char *chr = chrtbl_flc[c - 0xE286B0];
  int16_t x = tft.getCursorX();
  int16_t y = tft.getCursorY();
  uint8_t s = tft.textsize;
  for (int row = 0; row < 16; row++)
  {
    unsigned char data = pgm_read_byte(chr + row);
    for (uint8_t col = 0; col < 8; col++)
    {
      if (data & (0x80 >> col))
      {
        for (uint8_t i = 0; i < s; i++)
        {
          for (uint8_t j = 0; j < s; j++)
          {
            tft.drawPixel(x + col * s + i, y + row * s + j, TFT_WHITE);
          }
        }
      }
    }
  }
  tft.setCursor(x + 8 * s, y);
}

/**
 * @brief Affiche une flèche (↰↱↲↳) à une position spécifique.
 * 
 * @overload
 */
void printflc(const int & c, int16_t x, int16_t y)
{
#ifdef DEBUG
  Serial.print("[DEBUG] printflc(");
  Serial.print(c);
  Serial.println(")");
#endif
  if (c < 0xE286B0 || c > 0xE286B3)
  {
    Serial.println(F("ERROR: printflc: Invalid character code"));
    return;
  }
  const unsigned char *chr = chrtbl_flc[c - 0xE286B0];
  uint8_t s = tft.textsize;
  for (int row = 0; row < 16; row++)
  {
    unsigned char data = pgm_read_byte(chr + row);
    for (uint8_t col = 0; col < 8; col++)
    {
      if (data & (0x80 >> col))
      {
        for (uint8_t i = 0; i < s; i++)
        {
          for (uint8_t j = 0; j < s; j++)
          {
            tft.drawPixel(x + col * s + i, y + row * s + j, TFT_WHITE);
          }
        }
      }
    }
  }
  tft.setCursor(x + 8 * s, y);
}

//====================================================================================
//   Decode and render the Jpeg image onto the TFT screen
//====================================================================================
void jpegRender(const int &xpos, const int &ypos)
{

  // retrieve infomration about the image
  uint16_t *pImg;
  int16_t mcu_w = JpegDec.MCUWidth;
  int16_t mcu_h = JpegDec.MCUHeight;
  int32_t max_x = JpegDec.width;
  int32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  int32_t min_w = minimum(mcu_w, max_x % mcu_w);
  int32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  int32_t win_w = mcu_w;
  int32_t win_h = mcu_h;

#ifdef DEBUG
  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();
#endif

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes())
  { // Swapped byte order read

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos; // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x)
      win_w = mcu_w;
    else
      win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y)
      win_h = mcu_h;
    else
      win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      for (int h = 1; h < win_h - 1; h++)
      {
        memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
      }
    }

    // draw image MCU block only if it will fit on the screen
    if (mcu_x < tft.width() && mcu_y < tft.height())
    {
      // Now push the image block to the screen
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }

    else if ((mcu_y + win_h) >= tft.height())
      JpegDec.abort();
  }

#ifdef DEBUG
  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print("Total render time was    : ");
  Serial.print(drawTime);
  Serial.println(" ms");
  Serial.println("=====================================");
#endif
}

//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
int calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print)
{
#ifdef DEBUG
  Serial.println("===========================");
  Serial.print("Drawing file: ");
  Serial.println(filename);
  Serial.println("===========================");
#endif

  // Open the named file (the Jpeg decoder library will close it after rendering image)
  // fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  File jpegFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

// ESP32 always seems to return 1 for jpegFile so this null trap does not work
#ifdef DEBUG
  if (!jpegFile)
  {
    Serial.print(F("ERROR: File \""));
    Serial.print(filename);
    Serial.println(F("\" not found!"));
  }
#endif

  // Use one of the three following methods to initialise the decoder,
  // the filename can be a String or character array type:

  // boolean decoded = JpegDec.decodeFsFile(jpegFile); // Pass a SPIFFS file handle to the decoder,
  boolean decoded = JpegDec.decodeSdFile(jpegFile); // or pass the SD file handle to the decoder,
  // boolean decoded = JpegDec.decodeFsFile(filename);  // or pass the filename (leading / distinguishes SPIFFS files)

  if (decoded)
  {
    // print information about the image to the serial port
    // jpegInfo();

    // render the image onto the screen at given coordinates
    if (print)
      jpegRender(xpos, ypos);
    return JpegDec.height;
  }
  else
  {
#ifdef DEBUG
    Serial.println(F("Jpeg file format not supported!"));
#endif
  }
  return -1;
}
