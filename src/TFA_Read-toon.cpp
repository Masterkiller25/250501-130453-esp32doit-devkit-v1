#include <Arduino.h>

#include <stdarg.h>

// include for SD with spi
#include <SPI.h>
#include <SPIFFS.h>
#include <SD.h>

#define TFT_PARALLEL_8_BIT
#include <TFT_eSPI.h> //include for tft screen

// #include <String.h>

#define minimum(a, b) _min(a, b)
#define maximum(a, b) _max(a, b)

#define encoder_A_pin 35
#define encoder_B_pin 32
#define encoder_CLK_pin 33

#define SCAN 1
#define FILE_SYSTEM 0
#define OPTION 2

#define sck 12
#define miso 14
#define mosi 13
#define cs 15

#define DEBUG

#include "Encoder_Polling.h" //include for encoder
#include "File_essantial.h"  //include for file essential
#include "Graphics.h"        //include for graphics

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
uint8_t mode = FILE_SYSTEM;
uint8_t OPTION_mode = 0x0;

// TFT_eSPI tft = TFT_eSPI();
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
bool canopen(const String &filename);

int refresh_screen_SCAN(String path, const int &start);
void refresh_screen_FS(const int &selected, const int &n, const int &offset);
void refresh_screen_OPTION(const int &type, const int &selected);

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

  configs.close();

  begin(coos, colors, files);
  test();

#ifdef DEBUG
  if (!SD.exists("test.jpg"))
    println("No test.jpg");
  if (!SD.exists("/test.jpg"))
    println("No /test.jpg");
  if (!SD.exists("foo.txt"))
    println("No foo.txt");
  if (!SD.exists("/foo.txt"))
    println("No /foo.txt");

  printflc('↲');
  printflc('↳');
  delay(4000);
#endif
  delay(1000);

  scroll.start_timer();

  clear_screen();

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
        if (mode == FILE_SYSTEM && calcdirectory(directory.c_str())) // handle scroll in file system mode
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
      if (mode == FILE_SYSTEM)
      {
        if (files[count].type)
        {
          do_it = true;
          if (count == 0 && directory != "/") // parent directory
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
            if (calcdirectory(directory.c_str()))
            {
              do_it = true;
            }
          }
        }
        else
        {
          OPTION_mode = 0x0;
          OPTION_mode |= ((count != 0) << 0);
          OPTION_mode |= ((canopen(directory)) << 1);
          OPTION_mode |= (((count - 1) != max_count) << 2);
#ifdef DEBUG
          Serial.print("OPTION_mode");
          Serial.println(OPTION_mode);
#endif
          count = 0;
          mode = OPTION;
        }
      }
      if (mode == OPTION)
      {
        refresh_screen_OPTION(OPTION_mode, count);
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
 *Function:   canopen
 *--------------------------
 *Détirmine si le fichier est ouverable
 *
 * filename: chemin d'accés incluant le nom de fichier
 *
 * returns: ouverbable ou nom
 */
bool canopen(const String &filename)
{
  return filename.endsWith(".txt");
}
/*
 *Function:   refresh_screen_SCAN
 *--------------------------
 *Raffraichi l'écran ;mode:SCAN à l'aide de la librairies Jpegdec
 *
 *path: Répèretoire parent des image à afficher
 *
 *start: index de la première image
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
    y += calcJpeg(im_path.c_str(), 0, y, 1);
  }
  return i + start;
}
/*
 *Function:   refresh_screen_FS
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
  clear_screen();
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
