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

#define DEBUG

struct File_essantial
{
  String name;
  bool type;
};
struct Timer
{

  uint32_t start_time;

  void start_timer()
  {
    start_time = millis();
  }
  uint32_t get_timer()
  {
    return millis() - start_time;
  }
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
Timer scroll;
Timer click;
bool last_state_CLK = HIGH;
bool current_state_CLK = HIGH;

uint8_t coos[16][2];
uint8_t colors[0][3];

// prototype

int calcdirectory(const char *root_name);
int createrootdir();
String getparentdir(const String &path);
String dividepath(String &path, String &filename);

int refresh_screen_SCAN(String path, const int &start);
void refresh_screen_FS(const int &selected, const int &n, const int &offset);
void refresh_screen_OPTION(const int &type, const int &selected);

void calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print);
void jpegRender(const int &xpos, const int &ypos);

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
#ifdef DEBUG
    Serial.println(F("Cart SD Mount"));
  }
  else
  {
    Serial.println(F("Erroro SD"));
#endif
  }
  File configs = SD.open("/configs", FILE_READ);
  configs.readBytes((char *)&coos, sizeof(coos));
  configs.readBytes((char *)&colors, sizeof(colors));

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.println(F("Initaliser"));

#ifdef DEBUG
  if (!SD.exists("test.jpg"))
    tft.println("No test.jpg");
  if (!SD.exists("/test.jpg"))
    tft.println("No /test.jpg");
  if (!SD.exists("foo.txt"))
    tft.println("No foo.txt");
  if (!SD.exists("/foo.txt"))
    tft.println("No /foo.txt");
#endif
  scroll.start_timer();

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
      if (scroll.get_timer() < 300) // let time before refresh but add to the counter
      {
        scroll.start_timer();
      }
      else
      {
        // do_it = false;
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
          if (refresh_screen_SCAN(directory + "/", count) >= max_count)
          {
            String filename;
            dividepath(directory, filename);
            directory = directory + (filename.toInt() + 1);
#ifdef DEBUG
            Serial.print(F("directory: "));
            Serial.print(directory);
            Serial.print(F(" filename: "));
            Serial.print(filename);
            Serial.print(F(" full: "));
            Serial.println(directory);
#endif
            do_it = true;
            count = 0;
          }
          config.seek(2);
          config.write((uint8_t)count);
          config.flush();
        }
      }
    }
  }

  current_state_CLK = digitalRead(encoder_CLK_pin);
  if (current_state_CLK == LOW && last_state_CLK == HIGH) // handle press
  {
#ifdef DEBUG
    Serial.println("Encoder pressed");
#endif
    click.start_timer();
  }
  if (current_state_CLK == HIGH && last_state_CLK == LOW) // handle release
  {
#ifdef DEBUG
    Serial.println(click.get_timer());
#endif
    if (click.get_timer() < 1000) // handle quick release
    {
      if (files[count].type)
      {
        do_it = true;
        if (count == 0 && directory != "/")
        {
          uint8_t remove = 0;
          for (int i = directory.length() - 1; do_it; i--)
          {
            remove++;
            do_it = directory.charAt(i) != '/';
          }
          directory = directory.substring(0, maximum(directory.length() - remove, 1));
          do_it = true;
        }
        else
        {
          if (!directory.endsWith("/"))
            directory += "/";
          directory += files[count].name;
          count = 0;
          if (calcdirectory(directory.c_str())) {
            do_it = true;
          }
        }
      }
    }
    else // handle slow release
    {
    }
  }
  last_state_CLK = current_state_CLK;
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
#ifdef DEBUG
  Serial.print("[DEBUG] calcdirectory(");
  Serial.print(root_name);
  Serial.println(")");
#endif
  if (String(root_name) == last_root)
    return last_i;
  last_root = String(root_name);
  File root = SD.open(root_name);
  if (SD.exists(last_root + "/cfg_sc"))
  {
    config = SD.open(last_root + "/cfg_sc", "w+");
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
    }
    root.close();
    return false;
  }
  File entry = root.openNextFile();
  int i;
  for (i = ((root_name[1] == '\0') ? (0) : (createrootdir())); entry; i++)
  {
    files[i].name = String(entry.name());
    files[i].type = entry.isDirectory();
#ifdef DEBUG
    Serial.println(entry.path());
#endif
    entry = root.openNextFile();
  }
  entry.close();
  root.close();
  last_i = i;
  return i;
}

/*
 *Function:   createrootdir
 *--------------------------
 *Crée le dossier racine
 *
 *returns: 1
 */
int createrootdir()
{
#ifdef DEBUG
  Serial.println("[DEBUG] createrootdir()");
#endif
  files[0].name = String("..");
  files[0].type = true;
  return 1;
}

/*
 *Function:   getparentdir
 *--------------------------
 *Récupère le répertoire parent d'un dossier
 *
 * path: chemin à évaluer
 *
 * returns: le répertoire parent
 */
String getparentdir(const String &path)
{
#ifdef DEBUG
  Serial.print("[DEBUG] getparentdir(");
  Serial.print(path);
  Serial.println(")");
#endif
  uint8_t remove;

  for (remove = 0; path.charAt(path.length() - 1 - remove) != '/'; remove++)
    ;

  return path.substring(0, maximum(path.length() - remove, 1));
}

/*
 *Function:   dividepath
 *--------------------------
 *Récupère le répertoire parent d'un dossier
 *
 * path: chemin à évaluer
 *
 * filename: nom du fichier modifier par la fonction
 *
 * returns: le répertoire parent
 */
String dividepath(String &path, String &filename)
{
#ifdef DEBUG
  Serial.print("[DEBUG] dividepath(");
  Serial.print(path);
  Serial.print(", ");
  Serial.print(filename);
  Serial.println(")");
#endif
  uint8_t remove;

  for (remove = 0; path.charAt(path.length() - 1 - remove) != '/'; remove++)
    ;

  filename = path.substring(path.length() - remove, path.length());

  path = path.substring(0, maximum(path.length() - remove, 1));

  return path;
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
int refresh_screen_SCAN(String path, const int &start)
{
#ifdef DEBUG
  Serial.print("[DEBUG] refresh_screen_SCAN(");
  Serial.print(path);
  Serial.print(", ");
  Serial.print(start);
  Serial.println(")");
#endif
  String im_path;
  int y = 0;
  int i;
  for (i = 0; y < 320; i++)
  {
    im_path = path + (i + start);
    im_path = im_path + ".jpg";
    calcJpeg(im_path.c_str(), 0, y, 1);
    y += JpegDec.height;
  }
  return i + start;
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
#ifdef DEBUG
  Serial.print("[DEBUG] refresh_screen_FS(");
  Serial.print(selected);
  Serial.print(", ");
  Serial.print(n);
  Serial.print(", ");
  Serial.print(offset);
  Serial.println(")");
#endif
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
/*
 *Function:   refresh_screen_OPTION
 *--------------------------
 *Raffraichi l'écran ;mode:OPTION à l'aide de mes fonctions
 *
 *type: troi bit contenat les options afficher
 *
 *selected: index de l'option sélectionner
 *
 *returns: void
 */
void refresh_screen_OPTION(const int &type, const int &selected)
{
#ifdef DEBUG
  Serial.print("[DEBUG] refresh_screen_OPTION(");
  Serial.print(type);
  Serial.print(", ");
  Serial.print(selected);
  Serial.println(")");
#endif
  // claer
  //  tft.drawRect
}

//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
void calcJpeg(const char *filename, const int &xpos, const int &ypos, const bool &print)
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
  }
  else
  {
#ifdef DEBUG
    Serial.println(F("Jpeg file format not supported!"));
#endif
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
#ifdef DEBUG
  Serial.print("[DEBUG] drawdir(");
  Serial.print(i);
  Serial.println(")");
#endif
  tft.fillRoundRect(coos[0][0], coos[0][1] + i * 25, coos[1][0], coos[1][1], 3, tft.color565(200, 200, 50));
  tft.fillRoundRect(coos[2][0], coos[2][1] + i * 25, coos[3][0], coos[3][1], 3, tft.color565(200, 200, 50));
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
#ifdef DEBUG
  Serial.print("[DEBUG] drawfile(");
  Serial.print(i);
  Serial.println(")");
#endif
  tft.fillRect(coos[4][0], coos[4][1] + i * 25, coos[5][0], coos[5][1], TFT_WHITE);
  tft.fillRect(coos[6][0], coos[6][1] + i * 25, coos[7][0], coos[7][1], TFT_WHITE);
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
#ifdef DEBUG
  Serial.print("[DEBUG] writename(");
  Serial.print(i_screen);
  Serial.print(", ");
  Serial.print(i_file);
  Serial.println(")");
#endif
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
#ifdef DEBUG
  Serial.print("[DEBUG] drawsepar(");
  Serial.print(i);
  Serial.println(")");
#endif
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
#ifdef DEBUG
  Serial.print("[DEBUG] drawselect(");
  Serial.print(max);
  Serial.println(")");
#endif
  for (int i = 0; i < max; i++)
  {
    tft.drawRect(210, 8 + i * 25, 20, 20, TFT_WHITE);
    // tft.drawRect(211, 9 + i * 25, 18, 18, 0);
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
#ifdef DEBUG
  Serial.print("[DEBUG] drawselected(");
  Serial.print(i);
  Serial.println(")");
#endif
  tft.drawLine(215, 20 + i * 25, 219, 24 + i * 25, TFT_WHITE);
  tft.drawLine(219, 24 + i * 25, 224, 15 + i * 25, TFT_WHITE);
}
