#include <Arduino.h>

#include <stdarg.h>

#include "Encoder_Polling.h" //include for encoder

// include for SD with spi
#include <SPI.h>
#include <SPIFFS.h>
#include <SD.h>

#define TFT_PARALLEL_8_BIT
#include <TFT_eSPI.h> //include for tft screen

#include <JPEGDecoder.h> //include for Decoding JPEG file

// #include <String.h>

#define minimum(a, b) _min(a, b)
#define maximum(a, b) _max(a, b)

#define encoder_A_pin 35
#define encoder_B_pin 32
#define encoder_CLK_pin 33

#define SCAN 1
#define FS 0
#define OPTION 2

#define sck 12
#define miso 14
#define mosi 13
#define cs 15

struct File_essantial
{
  String name;
  bool type;
};

uint max_count = -1;
File_essantial files[255];
String directory = "/";
File config;
uint16_t chap = 1;
bool do_it = true;
uint8_t mode = FS;

TFT_eSPI tft = TFT_eSPI();
int count = 0;
int encoder_value;
bool last_state_CLK = HIGH;
bool current_state_CLK = HIGH;

// prototype

int calcdirectory(const char *root_name);
int createrootdir();

void refresh_screen_SCAN(String path, const int &start, const int &max);
void refresh_screen_FS(const int &selected, const int &n, const int &offset);

void calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print);
void jpegRender(const int &xpos, const int &ypos);

void start_timer();
uint32_t get_timer();

void drawdir(const int &i);
void drawfile(const int &i);
void writename(const int &i_screen, const int &i_file);
void drawsepar(const int &i);
void drawselect(const int &max);
void drawselected(const int &i);

void setup()
{
  Serial.begin(115200);

  SPI.begin(sck, miso, mosi, cs);
  if (SD.begin(cs))
  {
    Serial.println(F("Cart SD Mount"));
  }
  else
  {
    Serial.println(F("Erroro SD"));
  }

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.println(F("Initaliser"));

  // if (!SD.exists("test.jpg")) tft.println("No test.jpg");
  // if (!SD.exists("/test.jpg")) tft.println("No /test.jpg");
  // if (!SD.exists("foo.txt")) tft.println("No foo.txt");               //Debug
  // if (!SD.exists("/foo.txt")) tft.println("No /foo.txt");
  start_timer();

  delay(1000);

  tft.fillScreen(0);

  pinMode(encoder_CLK_pin, INPUT_PULLUP);
  encoder_begin(encoder_A_pin, encoder_B_pin);

  // drawJpeg("/test.jpg", tft.width() / 2 + 15, 0, 0);
  // encoder_value = 1;
}

void loop()
{

  encoder_value = encoder_data(); // return -1, 0 or 1
  if (encoder_value || do_it)     // handle scroll //do_it → to fake scroll
  {
    do_it = false;
    count -= encoder_value;
    if (count < 0)
    {
      count = 0;
    }
    else
    {
      if (get_timer() < 300 || do_it) // let time before refresh but add to the counter
      {
        start_timer();
      }
      else
      {
        if (count >= max_count)
        {
          count = max_count - 1;
        }
        if (mode == FS && calcdirectory(directory.c_str())) // handle scroll in file system mode
        {
          max_count = calcdirectory(directory.c_str());

          if (max_count <= 12)
          {
            refresh_screen_FS(count, max_count, 0);
          }
          else
          {

            if (max_count - count < 12)
            {
              refresh_screen_FS(12 - max_count + count, 12, max_count - 12);
            }
            else
            {
              refresh_screen_FS(0, 12, count);
            }
          }
        }
        else if (mode == SCAN) // handle scroll in scan mode
        {
          int i = 0;
          int y = 0;
          String im_path;
          while (true)
          {
            if (i + count >= max_count)
              break;
            im_path = directory + "/" + (i + count);
            im_path = im_path + ".jpg";
            calcJpeg(im_path.c_str(), 0, y, 0);
            i++;
            y += JpegDec.height;
            if (y >= 320)
              break;
          }
          refresh_screen_SCAN(directory + "/", count, i);
          config.seek(2);
          config.write((uint8_t)count);
          config.flush();
        }
      }
    }
  }

  current_state_CLK = digitalRead(encoder_CLK_pin);
  if (current_state_CLK == LOW && last_state_CLK == HIGH) // handle click
  {
    delay(20);
    if (digitalRead(encoder_CLK_pin) == LOW)
    {
      if (files[count].type)
      {
        do_it = true;
        if (count == 0)
        {
          uint8_t remove = 0;
          for (int i = directory.length() - 1; do_it; i--)
          {
            remove++;
            do_it = directory.charAt(i) != '/';
          }
          directory = directory.substring(0, directory.length() - remove);
          do_it = true;
        }
        else
        {
          if (!directory.endsWith("/"))
            directory += "/";
          directory += files[count].name;
          count = 0;
        }
      }
      Serial.println("Encoder pressed");
    }
  }
  last_state_CLK = current_state_CLK;

  /*
        int i = 0;
        int y = 0;
        String path = String("/"), im_path;
        path = path + chap;
        path = path + '/';
        while (true) {
          im_path = path + (i + count);
          im_path = im_path + ".jpg";
          if (calcJpeg(im_path.c_str(), 0, y, 0)) {
            max_count = count - 1;
            break;
          }
          i++;
          y += JpegDec.height;
          if (y >= 320) break;
        }
        refresh_screen_SCAN(path, count, i);
      }
    }
  }*/
}

String last_root;
int last_i;
/*
 *Function:   calcdirectory
 *--------------------------
 *Détermine les dossier et sous dossier d'un dossier donner en entrer et en stock l'éssentielle
 *
 *root_name: Nom de doosier(commence par "/")
 *
 *returns: le nombre de fichier/dossier présent. Si le dossier est configurer renvoie faux
 */
int calcdirectory(const char *root_name)
{
  if (String(root_name) == last_root)
    return last_i;
  last_root = String(root_name);
  File root = SD.open(root_name);
  if (SD.exists(last_root + "/config"))
  {
    File pre_config = SD.open(last_root + "/config", FILE_READ);
    char config_data[5];
    config.readBytes(config_data, 5);
    if (config_data[0] == SCAN)
    {
      chap = (uint16_t)config_data[1];
      count = config_data[2];
      encoder_value = config_data[3];
      max_count = config_data[4];
      do_it = true;
      mode = SCAN;
      config = SD.open(pre_config.path(), FILE_WRITE);
    }
    pre_config.close();
    root.close();
    return false;
  }
  File entry = root.openNextFile();
  int i;
  for (i = ((root_name[1] == '\0') ? (0) : (createrootdir())); entry; i++)
  {
    files[i].name = String(entry.name());
    files[i].type = entry.isDirectory();
    Serial.println(entry.path());
    entry = root.openNextFile();
  }
  entry.close();
  root.close();
  last_i = i;
  return i;
}

int createrootdir()
{
  files[0].name = String("..");
  files[0].type = true;
  return 1;
}

/*
 *Function:   refresh_screen_SCAN
 *--------------------------
 *Raffraichi l'écran ;mode:SCAN à l'aide de la librairies Jpegdec
 *
 *path: Répèretoire parent des image a afficher
 *
 *start: index de la première image
 *
 *max: index de la dernière image
 *
 *returns: void
 */
void refresh_screen_SCAN(String path, const int &start, const int &stop)
{
  String im_path;
  Serial.print(start);    // ↰
  Serial.print(F(" | ")); // } Debug
  Serial.println(stop);   // ↲
  int y = 0;
  for (int i = 0; i < stop; i++)
  {
    im_path = path + (i + start);
    im_path = im_path + ".jpg";
    calcJpeg(im_path.c_str(), 0, y, 1);
    y += JpegDec.height;
  }
}
/*
 *Function:   refresh_screen_SCAN
 *--------------------------
 *Raffraichi l'écran ;mode:FS à l'aide de mes fonctions
 *
 *selected: index de l'élement sélectionner
 *
 *n: nombre de fichier à afficher
 *
 *offset: index du début des informations des fichier
 *
 *returns: void
 */
void refresh_screen_FS(const int &selected, const int &n, const int &offset)
{
  tft.fillScreen(0);
  for (int i = 0; i < n; i++)
  {
    if (files[i + offset].type)
      drawdir(i);
    else
      drawfile(i);
    writename(i, i + offset);
    if (i == selected)
      drawselected(i);
  }
  drawselect(n);
}

uint32_t start_time;

void start_timer()
{
  start_time = millis();
}
uint32_t get_timer()
{
  return millis() - start_time;
}

//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
void calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print)
{

  Serial.println("===========================");
  Serial.print("Drawing file: ");
  Serial.println(filename);
  Serial.println("===========================");

  // Open the named file (the Jpeg decoder library will close it after rendering image)
  // fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  File jpegFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

  // ESP32 always seems to return 1 for jpegFile so this null trap does not work
  if (!jpegFile)
  {
    Serial.print("ERROR: File \"");
    Serial.print(filename);
    Serial.println("\" not found!");
  }

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
  }
  else
  {
    Serial.println("Jpeg file format not supported!");
  }
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

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

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

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print("Total render time was    : ");
  Serial.print(drawTime);
  Serial.println(" ms");
  Serial.println("=====================================");
}

/*
 *Function:   drawdir
 *--------------------------
 *Dessine le logo d'un dossier
 *
 *i: index de l'endroit où il doit être
 *
 *returns: void
 */
void drawdir(const int &i)
{
  tft.fillRoundRect(10, 10 + i * 25, 17, 15, 3, tft.color565(200, 200, 50));
  tft.fillRoundRect(10, 15 + i * 25, 19, 10, 3, tft.color565(200, 200, 50));
}
/*
 *Function:   drawfile
 *--------------------------
 *Dessine le logo d'un fichier
 *
 *i: index de l'endroit où il doit être
 *
 *returns: void
 */
void drawfile(const int &i)
{
  tft.fillRect(10, 9 + i * 25, 10, 17, TFT_WHITE);
  tft.fillRect(10, 14 + i * 25, 15, 15, TFT_WHITE);
  tft.fillTriangle(19, 9 + i * 25, 19, 14 + i * 25, 24, 14 + i * 25, TFT_WHITE);
}

/*
 *Function:   writename
 *--------------------------
 *Ecrit le nom d'un fichier
 *
 *i_screen: index de l'endroit où il doit être
 *
 *i_file: index du fichier
 *
 *returns: void
 */
void writename(const int &i_screen, const int &i_file)
{
  tft.setCursor(40, 11 + i_screen * 25);
  if (files[i_file].name.length() > 14)
  {
    for (int i = 0; i < 11; i++)
      tft.print(files[i_file].name.charAt(i));
    tft.print(F("..."));
  }
  else
  {
    tft.print(files[i_file].name.c_str());
  }
  drawsepar(i_screen);
}
/*
 *Function:   drawsepar
 *--------------------------
 *Dessine une ligne entre les fichier
 *
 *i: index de l'endroit où il doit être
 *
 *returns: void
 */
void drawsepar(const int &i)
{
  tft.drawFastHLine(5, 30 + i * 25, tft.width() - 10, TFT_WHITE);
}

/*
 *Function:   drawselect
 *--------------------------
 *Dessine une boite à cocher
 *
 *max: index maximum des endroits où elles doivent être
 *
 *returns: void
 */
void drawselect(const int &max)
{
  for (int i = 0; i < max; i++)
  {
    tft.drawRect(210, 8 + i * 25, 20, 20, TFT_WHITE);
    tft.drawRect(211, 9 + i * 25, 18, 18, 0);
  }
}
/*
 *Function:   drawselected
 *--------------------------
 *Dessine un ✔ dans la boite
 *
 *i: index de l'endroit où il doit être
 *
 *returns: void
 */
void drawselected(const int &i)
{
  tft.drawLine(215, 20 + i * 25, 219, 24 + i * 25, TFT_WHITE);
  tft.drawLine(219, 24 + i * 25, 224, 15 + i * 25, TFT_WHITE);
}
