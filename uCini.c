/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
******************************************************************************/
#define __uCini_c

#include "uCini.h"
#include "stdio_.h"
#include <string.h>

/******************************************************************************
* uCiniParse
******************************************************************************/
int uCiniParse(const struct tIni* ini, char* fileName) {
  int values = 0;
  const struct tSection* sectionAct = NULL;
  FILE_* iniFile = stdin_;
  if (fileName) iniFile = fopen_(fileName, "r");

  // Line loop
  while (1) {
    int i;
    char line[20];
    char *token1, *token2;
    if (!fgets_(line, MAX_LINE_LENGTH, iniFile)) break; // EOF, leave line loop
    // Check for section changes...
    if (line[0] == '[') {
      sectionAct = NULL;
      token1 = strtok(line, "[]");
      for (i = 0; i < ini->nSection; ++i) {
        const struct tSection* sectionTmp = ini->sections + i;
        if (!strncmp(token1, sectionTmp->name, 20)) {
          sectionAct = sectionTmp;
          break;
        }
      }
    }

    // Skip all entry lines if section invalid,
    else if (sectionAct) {
      // or parse params within section
      token1 = strtok(line, " \t="); // name  no whitespace, end with =
      token2 = strtok(NULL, "\r\n"); // value between = and CR/LF
      for (i = 0; i < sectionAct->nEntry; ++i) {
        const struct tEntry* entryTmp = sectionAct->entries + i;
        char type = entryTmp->type & eType_MASK_TYPE;
        char indx = entryTmp->type & eType_MASK_NUM;
        if (strncmp(token1, entryTmp->name, 20)) continue;

        switch (type) {
          // function
          case eType_FUNC:
            ((tIniFunc)entryTmp->data)(token2, 0);
            break;
          // flag in a byte
          case eType_FLAG:
            if (!strcmp(token2, "y")) SET(*(char*)entryTmp->data, indx);
            if (!strcmp(token2, "n")) CLR(*(char*)entryTmp->data, indx);
            break;
          // char short long
          case eType_INT: {
            long number;
            if (!sscand(token2, &number)) continue;
            memcpy(entryTmp->data, &number, indx);
            break;
          }
          // raw string
          case eType_SZ:
            strcpy(entryTmp->data, token2);
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
int uCiniDump(const struct tIni* ini, char* fileName) {
  int values = 0;
  int iSection;
  char line[20];
  const struct tSection* sectionAct = NULL;
  FILE_* iniFile = stdout_;
  if (fileName) iniFile = fopen_(fileName, "w");

  // Section loop
  for (iSection = 0; iSection < ini->nSection; ++iSection) {
    const struct tSection* sectionAct = ini->sections + iSection;
    int iEntry;
    strcpy(line, "[");
    strcat(line, sectionAct->name);
    strcat(line, "]\n");
    fputs_(line, iniFile);

    // Entry loop for each section
    for (iEntry = 0; iEntry < sectionAct->nEntry; ++iEntry) {
      const struct tEntry* entryTmp = sectionAct->entries + iEntry;
      char type = entryTmp->type & eType_MASK_TYPE;
      char sgnd = entryTmp->type & eType_SGND;
      char indx = entryTmp->type & eType_MASK_NUM;
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
            case 1:   scatd(line, *(unsigned char* )entryTmp->data); break;
            case 2:   scatd(line, *(unsigned short*)entryTmp->data); break;
            case 4:   scatd(line, *(unsigned long* )entryTmp->data); break;
            case 1+eType_SGND: scatd(line, *(char* )entryTmp->data); break;
            case 2+eType_SGND: scatd(line, *(short*)entryTmp->data); break;
            case 4+eType_SGND: scatd(line, *(long* )entryTmp->data); break;
          }
          break;
        // raw string
        case eType_SZ:
          strcat(line, entryTmp->data); 
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
int sscand(char* str, long* pNum) {
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
void scatd(char* str, long num) {
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
