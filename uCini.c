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
  const struct tSection* sectionAct = NULL;
  FILE_* iniFile = stdin_;
  if (fileName) iniFile = fopen_(fileName, "r");
  while (1) {
    int i;
    char line[20];
    char *token1, *token2;
    if (!fgets_(line, 20, iniFile)) break;

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

    // ... or parse params within sections
    else if (sectionAct) {
      token1 = strtok(line, " \t=");
      token2 = strtok(NULL, "\r\n");
      for (i = 0; i < sectionAct->nEntry; ++i) {
        const struct tEntry* entryTmp = sectionAct->entries + i;
        if (!strncmp(token1, entryTmp->name, 20)) {
          switch (entryTmp->type) {
            case eType_SZ   :
              strcpy(entryTmp->data, token2); 
              break;
            case eType_CHAR :
            case eType_SHORT:
            case eType_LONG : {
              long number;
              if (!sscand(token2, &number)) continue;
              memcpy(entryTmp->data, &number, entryTmp->type);
              break;
            }
            case eType_FUNC :
              ((tIniFunc)entryTmp->data)(token2, 0);
              break;
            default:
              break;
          }
        }
      }
    }
  }
  fclose_(iniFile);
}

/******************************************************************************
* uCiniDump
******************************************************************************/
int uCiniDump(const struct tIni* ini, char* fileName) {
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
      long number;
      const struct tEntry* entryTmp = sectionAct->entries + iEntry;
      strcpy(line, entryTmp->name);
      strcat(line, "=");

      switch (entryTmp->type) {
        case eType_CHAR :
          sprintd(line, *(char*)entryTmp->data);
          break;
        case eType_SHORT:
          sprintd(line, *(short*)entryTmp->data);
          break;
        case eType_LONG :
          sprintd(line, *(long*)entryTmp->data);
          break;
        case eType_SZ   :
          strcat(line, entryTmp->data); 
          break;
        case eType_FUNC :
          ((tIniFunc)entryTmp->data)(line, 1);
          break;
      }
      strcat(line, "\n");
      fputs_(line, iniFile);
    }
  }
  if (fileName) fclose_(iniFile);
}

/******************************************************************************
* sscand
******************************************************************************/
int sscand(char* str, long* pNum) {
  long number = 0;
  if      (!strncmp(str, "y", 2)) *pNum = 1;
  else if (!strncmp(str, "n", 2)) *pNum = 0;
  else {
    int sign = 1;
    if (*str == '+')   str++;
    if (*str == '-') { str++; sign = -1; }
    if (*str < '0' || *str > '9') return 0;
    while (*str) {
      int digit = (*str - '0') * sign;
      number *= 10;
      number += digit;
      str++;
    }
  }
  *pNum = number;
  return 1;
}

/******************************************************************************
* sprintd
******************************************************************************/
void sprintd(char* str, long num) {
  long div; 
  int  mod;
  char digit[2];

  if (num == 0) {
    strcat(str, "0");
    return;
  }
  
  if (num < 0) { 
    strcat(str, "-");
    num *= -1;
  }

  div = num / 10;
  mod = num % 10;

  if (div) sprintd(str, div);

  digit[0] = '0' + mod;
  digit[1] = 0;

  strcat(str, digit);
}
