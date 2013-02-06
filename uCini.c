/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
******************************************************************************/
#define __uCini_c

#include "uCini.h"
#include "stdio_.h"
#include <string.h>

// Bitfield operations.
static unsigned char BF_MASK(int pos, int size);
static unsigned char BF_RD(unsigned char *byte, int pos, int size);
static void BF_WR(unsigned char *byte, int pos, int size, unsigned char data);

/******************************************************************************
* uCiniParseLine
******************************************************************************/
void uCiniParseLine(char *line, char *words[])
{
    int i;
    words[TSEC] = words[TKEY] = words[TVAL] = NULL;
    // Add trailing LF to line if missing
    i = strlen(line);
    if (line[i - 1] != '\n') line[i] = '\n';
    // Look for section
    if (line[0] == '[') {
      words[TSEC] = strtok(line, "[]");
      return;
    }
    // Look for key-value pair
    words[TKEY] = strtok(line, "="); // name  no whitespace, end with =
    words[TVAL] = strtok(NULL, ";\r\n"); // value between = and CR/LF
}

/******************************************************************************
* uCiniParse
******************************************************************************/
int uCiniParse(const struct tIni *pIni, char *fileName)
{
  int values = 0;
  int ofsSection = 0;
  int ofsEntry = 0;
  const struct tSection *sectionAct = NULL;
  FILE_ *iniFile = stdin_;
  if (fileName) iniFile = fopen_(fileName, "r");

  // Line loop
  while (1) {
    int i;
    char line[MAX_LINE_LENGTH];
    char *words[3];
    if (!fgets_(line, MAX_LINE_LENGTH, iniFile)) break;
    uCiniParseLine(line, words);
    // Check for section changes...
    if (words[TSEC] != NULL) {
      sectionAct = NULL;
      for (i = 0; i < pIni->nSection; ++i) {
        int iSection = (i + ofsSection) % pIni->nSection;
        const struct tSection *sectionTmp = pIni->sections + iSection;
        if (!strncmp(words[TSEC], sectionTmp->name, MAX_LINE_LENGTH)) {
          sectionAct = sectionTmp;
          ofsSection = iSection;
          break;
        }
      }
    }

    // Skip all entry lines if section invalid,
    if (sectionAct) {
      // or parse params within section
      if (words[TKEY] == NULL || words[TVAL] == NULL) continue;
      for (i = 0; i < sectionAct->nEntry; ++i) {
        unsigned char size;
        long number;
        int iEntry = (i + ofsEntry) % sectionAct->nEntry;
        const struct tEntry *entryTmp = sectionAct->entries + iEntry;
        unsigned char type = entryTmp->type & (eType_MASK_TYPE|eType_MASK_ALTT);
        unsigned char indx = entryTmp->type & eType_MASK_NUM;
        if (strncmp(words[TKEY], entryTmp->name, MAX_LINE_LENGTH)) continue;

        ofsEntry = iEntry;
        switch (type) {
        // function
        case eType_FUNC:
          ((tIniFunc)entryTmp->data)(words[TVAL], 0);
          break;
        // flag in a byte
        case eType_FLAG:
          if (strcmp(words[TVAL], "y") == 0 || strcmp(words[TVAL], "1") == 0) {
            SET(*(char*)entryTmp->data, indx);
          }
          if (strcmp(words[TVAL], "n") == 0 || strcmp(words[TVAL], "0") == 0) {
            CLR(*(char*)entryTmp->data, indx);
          }
          break;
        // char short long
        case eType_INT:
          if (!sscand(words[TVAL], &number)) continue;
          switch (indx) {
          case 1: *(char *)entryTmp->data = number; break;
          case 2: *(short*)entryTmp->data = number; break;
          case 4: *(long *)entryTmp->data = number; break;
          }
          break;
        // raw string
        case eType_SZ:
          strcpy(entryTmp->data, words[TVAL]);
          break;
        // 1-7 bit fields
        case eType_MASK_ALTT + eType_BITF1:
        case eType_MASK_ALTT + eType_BITF2:
        case eType_MASK_ALTT + eType_BITF3:
        case eType_MASK_ALTT + eType_BITF4:
        case eType_MASK_ALTT + eType_BITF5:
        case eType_MASK_ALTT + eType_BITF6:
        case eType_MASK_ALTT + eType_BITF7:
          if (!sscand(words[TVAL], &number)) continue;
          size = type >> 5;
          BF_WR((unsigned char*)entryTmp->data, indx, size, number);
          break;
        }
        values++;
      }
    }
  }
  fclose_(iniFile);
  return values;
}

/******************************************************************************
* uCiniDump
******************************************************************************/
int uCiniDump(const struct tIni *pIni, char *fileName)
{
  int values = 0;
  int iSection;
  char line[MAX_LINE_LENGTH];
  FILE_ *iniFile = stdout_;
  if (fileName) iniFile = fopen_(fileName, "w");

  // Section loop
  for (iSection = 0; iSection < pIni->nSection; ++iSection) {
    const struct tSection *sectionAct = pIni->sections + iSection;
    int iEntry;
    strcpy(line, "[");
    strcat(line, sectionAct->name);
    strcat(line, "]\n");
    fputs_(line, iniFile);

    // Entry loop for each section
    for (iEntry = 0; iEntry < sectionAct->nEntry; ++iEntry) {
      const struct tEntry *entryTmp = sectionAct->entries + iEntry;
      unsigned char type = entryTmp->type & (eType_MASK_TYPE|eType_MASK_ALTT);
      unsigned char indx = entryTmp->type & eType_MASK_NUM;
      char sgnd = entryTmp->type & eType_SGND;
      unsigned char size;
      strcpy(line, entryTmp->name);
      strcat(line, "=");

      switch (type) {
      // function
      case eType_FUNC:
        ((tIniFunc)entryTmp->data)(line, 1);
        break;
      // flag in a byte
      case eType_FLAG:
        strcat(line, GET(*(char*)entryTmp->data, indx) ? "y" : "n");
        break;
      // char short long
      case eType_INT:
        switch (indx + sgnd) {
        case 1:   scatd(line, *(unsigned char *)entryTmp->data); break;
        case 2:   scatd(line, *(unsigned short*)entryTmp->data); break;
        case 4:   scatd(line, *(unsigned long *)entryTmp->data); break;
        case 1+eType_SGND: scatd(line, *(char *)entryTmp->data); break;
        case 2+eType_SGND: scatd(line, *(short*)entryTmp->data); break;
        case 4+eType_SGND: scatd(line, *(long *)entryTmp->data); break;
        }
        break;
      // raw string
      case eType_SZ:
        strcat(line, entryTmp->data);
        break;
      // 1-7 bit fields
      case eType_MASK_ALTT + eType_BITF1:
      case eType_MASK_ALTT + eType_BITF2:
      case eType_MASK_ALTT + eType_BITF3:
      case eType_MASK_ALTT + eType_BITF4:
      case eType_MASK_ALTT + eType_BITF5:
      case eType_MASK_ALTT + eType_BITF6:
      case eType_MASK_ALTT + eType_BITF7:
        size = type >> 5;
        scatd(line, BF_RD((unsigned char*)entryTmp->data, indx, size));
        break;
      }
      values++;
      strcat(line, "\n");
      fputs_(line, iniFile);
    }
  }
  if (fileName) fclose_(iniFile);
  return values;
}

/******************************************************************************
* sscand
******************************************************************************/
int sscand(char *str, long *pNum)
{
  long number = 0;
  int sign = 1;
  if (*str == '-') { str++; sign = -1; }
  while (*str) {
    int digit = *str++; // read 1st char, then step to next char
    if (digit < '0' || digit > '9') return 0;
    digit -= '0';
    number *= 10;
    number += digit;
  }
  number *= sign;
  *pNum = number;
  return 1;
}

/******************************************************************************
* scatd - calls itself recursively to print MSdigit first
******************************************************************************/
void scatd(char *str, long num)
{
  long div; 
  int  mod;
  char digit[2]; // zero term. string to hold one decimal digit

  // Special cases: zero and negative numbers (print neg.sign)
  if (num == 0) {
    strcat(str, "0");
    return;
  }
  if (num < 0) { 
    strcat(str, "-");
    num *= -1;
  }

  // If num >= 10, print more significant digits first by recursive call
  // then we print least significatn digit (mod) ourselves.
  div = num / 10; // nonzero if num >= 10
  mod = num % 10;
  if (div) scatd(str, div);
  digit[0] = '0' + mod;
  digit[1] = 0;

  // Concatenate digit to target string
  strcat(str, digit);
}

// Bitfield operations.
static unsigned char BF_MASK(int pos, int size)
{
  unsigned char mask = (1u << pos+size) - 1;
  mask &= ~((1 << pos) - 1);
  return mask;
}

static unsigned char BF_RD(unsigned char *byte, int pos, int size)
{
  return (*byte & BF_MASK(pos, size)) >> pos;
}

static void BF_WR(unsigned char *byte, int pos, int size, unsigned char data)
{
  unsigned char mask = BF_MASK(pos, size);
  *byte &= ~mask;
  *byte |= (data << pos) & mask;
}
