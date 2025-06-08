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
uint16_t OPTION_mode = 0x0;

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
void open_file(const int &i);

int refresh_screen_SCAN(String path, const int &start);
void refresh_screen_FS(const int &selected, const int &n, const int &offset);
void refresh_screen_OPTION(const int &type, const int16_t &selected);

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

#ifdef DEBUG
  test();
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
  scroll.start_timer();

  delay(1000);

  clear_screen();

  pinMode(encoder_CLK_pin, INPUT_PULLUP);
  encoder_begin(encoder_A_pin, encoder_B_pin);
}

void loop()
{
  if (do_it)
  {
    do_it = false;
    goto Fake_scroll; // go to the scroll handler
  }

  encoder_value = encoder_data(); // return -1, 0 or 1
  if (encoder_value)              // handle scroll
  {
    count -= encoder_value;
    {
      if (scroll.get_timer() < 300) // let time before refresh but add to the counter
      {
        scroll.start_timer();
      }
      else
      {
      Fake_scroll:
        if (count < 0)
        {
          count = 0;
        }
        if (count >= max_count)
        {
          count = max_count - 1;
        }
        if (mode == FILE_SYSTEM && calcdirectory(directory.c_str()) != -1) // handle scroll in file system mode
        {
          max_count = calcdirectory(directory.c_str());

          if (max_count <= 12) // moins de 12 fichiers
          {
            refresh_screen_FS(count, max_count, 0);
            goto Endloop;
          }
          clear_screen();
          if (max_count - count < 12) // plus de 12 fichiers mais on est en bas
          {
            refresh_screen_FS(12 - max_count + count, 12, max_count - 12);
            goto Endloop;
          }
          refresh_screen_FS(0, 12, count); // plus de 12 fichiers et on est en haut/milieu
          goto Endloop;
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
            count = 0;
            goto Fake_scroll;
          }
          config.seek(2);
          config.write((uint8_t)count);
          config.write((uint8_t)0);
          config.write((uint8_t)max_count);
          config.flush();
        }
        else if (mode == OPTION) // handle scroll in option mode
        {
          refresh_screen_OPTION(OPTION_mode, count);
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
        open_file(count);
        goto Fake_scroll;
      }
      if (mode == SCAN)
      {
        OPTION_mode = 0x0;
        OPTION_mode |= ((false) << 0);
        OPTION_mode |= ((false) << 1);
        OPTION_mode |= ((true) << 2);
        OPTION_mode |= ((mode) << 3);
        OPTION_mode |= ((count) << 5);
#ifdef DEBUG
        Serial.print("OPTION_mode");
        Serial.println(OPTION_mode);
#endif
        count = 0;
        max_count = 5;
        mode = OPTION;
        goto Fake_scroll;
      }
      if (mode == OPTION)
      {
        if (count == 0 && (OPTION_mode & (1 << 0)) >> 0)
        {
          count = OPTION_mode >> 5;
          OPTION_mode = 0x0;
          OPTION_mode |= ((count != 0) << 0);
          OPTION_mode |= ((canopen(directory)) << 1);
          OPTION_mode |= (((count - 1) != max_count) << 2);
          OPTION_mode |= ((mode) << 3);
          OPTION_mode |= ((count) << 5);
#ifdef DEBUG
          Serial.print("OPTION_mode");
          Serial.println(OPTION_mode);
#endif
          count = 0;
          max_count = 5;
          mode = OPTION;
          goto Fake_scroll;
        }

        if (count == 1) // OPEN
        {
          open_file(OPTION_mode >> 5);
          goto Fake_scroll;
        }
      }
    }
    else // handle slow release
    {
      if (mode == FILE_SYSTEM)
      {
        OPTION_mode = 0x0;
        OPTION_mode |= ((count != 0) << 0);
        OPTION_mode |= ((canopen(directory)) << 1);
        OPTION_mode |= (((count - 1) != max_count) << 2);
        OPTION_mode |= ((mode) << 3);
        OPTION_mode |= ((count) << 5);
#ifdef DEBUG
        Serial.print("OPTION_mode");
        Serial.println(OPTION_mode);
#endif
        count = 0;
        max_count = 5;
        mode = OPTION;
        goto Fake_scroll;
      }
    }
  }
Endloop:
  last_state_CLK = current_state_CLK;
}
String last_root;
int last_i;
/**
 * @brief Détermine les dossiers et sous-dossiers d'un dossier donné en entrée et en stocke l'essentiel.
 *
 * @param root_name Nom du dossier (commence par "/")
 * @return int Le nombre de fichiers/dossiers présents. Si le dossier est configuré, renvoie -1.
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
    return -1;
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

/**
 * @brief Crée le dossier parent(..).
 *
 * @return int 1
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

/**
 * @brief Récupère le répertoire parent d'un dossier.
 *
 * @param path Chemin à évaluer
 * @return String Le répertoire parent
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

/**
 * @brief Récupère le répertoire parent d'un dossier et extrait le nom du fichier.
 *
 * @param[in, out] path Chemin à évaluer (sera modifié)
 * @param[out] filename Nom du fichier modifié par la fonction
 * @return String Le répertoire parent
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

/**
 * @brief Détermine si le fichier est ouvrable.
 *
 * @param filename Chemin d'accès incluant le nom de fichier
 * @return bool Ouvrable ou non
 */
bool canopen(const String &filename)
{
  return filename.endsWith(".txt") || filename.endsWith("/");
}
/**
 * @brief Raffraichi l'écran ;mode:SCAN à l'aide de la librairie Jpegdec.
 *
 * @param path Répertoire parent des images à afficher
 * @param start Index de la première image
 * @return int Dernier index affiché
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
/**
 * @brief Raffraichi l'écran ;mode:FS à l'aide de mes fonctions.
 *
 * @param selected Index de l'élément sélectionné
 * @param n Nombre de fichiers à afficher
 * @param offset Index du début des informations des fichiers
 * @return void
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

/**
 * @brief Raffraichi l'écran ;mode:OPTION à l'aide de mes fonctions.
 *
 * @param type Trois bits contenant les options à afficher
 * @param selected Index de l'option sélectionnée
 * @return void
 */
void refresh_screen_OPTION(const int &type, const int16_t &selected)
{
#ifdef DEBUG
  Serial.print("[DEBUG] refresh_screen_OPTION(");
  Serial.print(type);
  Serial.print(", ");
  Serial.print(selected);
  Serial.println(")");
  Serial.print("Previous count: ");
  Serial.println(type >> 5);
  Serial.print("Can open: ");
  Serial.println((type & (1 << 1)) >> 1);
  Serial.print("flc: ");
  Serial.print((type & (1 << 0)) >> 0);
  Serial.println((type & (1 << 2)) >> 2);
  Serial.print("Selected: ");
  Serial.println(selected);
#endif

  fillRect(0, 0, 240, 45, TFT_BLACK);
  fillRect(0, 240, 240, 80, TFT_BLACK);
  if ((type & (1 << 0)) >> 0)
  {
    printflc('↲', 20, 10);
  }
  if ((type & (1 << 1)) >> 1)
  {
    println("Open", 90, 15);
  }
  if ((type & (1 << 2)) >> 2)
  {
    printflc('↳', 200, 10);
  }
  println("Delete", 10, 255);
  println("Back", 10, 290);

  switch (selected)
  {
  case 0:
    drawRect(0, 0, 58, 45, TFT_WHITE);
    break;
  case 1:
    drawRect(59, 0, 122, 45, TFT_WHITE);
    break;
  case 2:
    drawRect(182, 0, 58, 45, TFT_WHITE);
    break;
  case 3:
    drawRect(0, 240, 240, 39, TFT_WHITE);
    break;
  case 4:
    drawRect(0, 280, 240, 40, TFT_WHITE);
    break;
  default:
    drawRect(0, 280, 240, 40, TFT_WHITE);
    break;
  }
}

/**
 * @brief Ouvre un fichier ou un dossier.
 *
 * @param i Index du fichier ou du dossier à ouvrir
 *
 * @return void
 */
void open_file(const int &i)
{
#ifdef DEBUG
  Serial.print("[DEBUG] open_file(");
  Serial.print(i);
  Serial.println(")");
#endif
  mode = FILE_SYSTEM;
  if (files[i].type)
  {
    clear_screen();
    if ((i) == 0 && directory != "/")
    {
      uint8_t i;
      for (i = directory.length() - 1; i >= 0 && directory.charAt(i) != '/'; i--)
        ;
      directory = directory.substring(0, maximum(i, 1));
      return;
    }
    else
    {
      if (!directory.endsWith("/"))
        directory += "/";
      directory += files[i].name;
      count = 0;
      if (calcdirectory(directory.c_str()) == -1)
      {
        mode = SCAN;
        return;
      }
      else
      {
        mode = FILE_SYSTEM;
        return;
      }
    }
  }
  else
  {
    clear_screen();
    File file = SD.open(directory + files[i].name);
    if (file)
    {
      char buffer[64];
      file.readBytes(buffer, 64);
      println(buffer);
    }
    file.close();
    delay(2000);
  }
}