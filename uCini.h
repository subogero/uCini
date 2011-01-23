/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
*******************************************************************************
*******************************************************************************
* A lightweight library to read and write ini files in embedded uC systems.
* Everything is case sensitive.
*
* Supported values:
* type internal            ini file value
* 0    string zero.term.   any char except = CR/LF terminated, no quoting
* 127  read/write function ..
* 1    char                (+/-) decimal number
* 2    short               ..
* 4    long                ..
*
* Section names: delimited by [], whitespace is allowed and significant
* Entry names  : whitespace and '=' not allowed
* Values       : delimited by '=' CR/LF, whitespace allowed and significant
******************************************************************************/
#ifndef __uCini
#define __uCini

enum eType {
  eType_SZ = 0,
  eType_CHAR  = 1,
  eType_SHORT = 2,
  eType_LONG  = 4,
  eType_FUNC = 0x7F
};

typedef void (*tIniFunc)(char* str, int rw); // rw 0=read 1=write

struct tEntry {
  const char* name;
  void* data;
  enum eType type;
};

struct tSection {
  const char* name;
  const struct tEntry* entries; // array of tEntry
  int nEntry;                   // size of array
};

struct tIni {
  const struct tSection* sections; // array of tSection
  int nSection;                    // size of array
};

/******************************************************************************
* Define the access of your internal data using the tIni data structure.
* Pass its address and a filename to the function.
* It'll update any internal value defined in the ini file
* and return the number of parsed values.
******************************************************************************/
int uCiniParse(const struct tIni* ini, char* fileName);

/******************************************************************************
* Define the access of your internal data using the tIni data structure.
* It creates an ini-file, and serializes your ENTIRE data structure into it.
******************************************************************************/
int uCiniDump(const struct tIni* ini, char* fileName);

/******************************************************************************
* Scan and print decimal numbers
******************************************************************************/
int  sscand(char* str, long* pNum);
void sprintd(char* str, long num);

#endif
