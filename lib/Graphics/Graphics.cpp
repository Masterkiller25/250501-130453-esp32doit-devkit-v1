#include <Arduino.h>
#include <TFT_eSPI.h>
#include <File_essantial.h>


TFT_eSPI tft = TFT_eSPI();
uint8_t **coos;
uint8_t **colors;
File_essantial *files;


/*
 *Function: begin
 *--------------------------
 *Setup variable
 *
 * coos_: pointeur de la matrix coo points
 * 
 * colors_: pointeur de la matrix des couleurs
 * 
 * files_: pointeur de la liste des fichiers
 * 
 * returns: void
*/
void begin(uint8_t **coos_, uint8_t **colors_, File_essantial *files_)
{
    coos = coos_;
    colors = colors_;
    files = files_;
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
