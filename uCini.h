/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
*******************************************************************************
* A lightweight library to read and write ini files in embedded uC systems.
*
* Section names: delimited by [], whitespace is allowed and significant
* Entry names  : whitespace and '=' not allowed
* Values       : delimited by '=' CR/LF, whitespace remains part of value
* Everything is case sensitive.
*
* eType_         internal type             ini format / notes
* -----------------------------------------------------------------------------
* SZ             string zero.term.         all chars between '=' and CR/LF
* INT+1/2/4      unsigned char/short/long  NOTE: unsigned long Dumped as signed
* INT+1/2/4+SGND signed   char/short/long  optional '-' before decimal number
* FLAG + bitpos. 1-bit flags in char       'y' or 'n'
* FUNC           read/write function       all chars between '=' and CR/LF
*
* Notes:
* -----------------------------------------------------------------------------
* Dumping unsigned long is done in signed format. Thus 4294967295 becomes -1.
* But reading it from ini-file is fine. 4294967295 remains 4294967295.
*
* Flags can be stored in chars only.
******************************************************************************/
#ifndef __uCini
#define __uCini

// Values of tEntry::type
enum eType {
  eType_MASK_NUM  = 0x0F, // Bit 0-3: size or flag position
  eType_SGND      = 0x10, // Bit 4  : signed
  eType_MASK_TYPE = 0xE0, // Bit 5-7: type
  eType_SZ        = 0x00, // Type string
  eType_INT       = 0x20, // Type char/short/long integer
  eType_FLAG      = 0x40, // Type flag in char
  eType_FUNC      = 0x60  // Type read/write function
};

// Flag operations. You're better off to #define flag positions, not masks
#define MASK(pos)     (1u << (pos))
#define GET(data,pos) ((data) & MASK(pos))
#define SET(data,pos) ((data) |= MASK(pos))
#define CLR(data,pos) ((data) &= ~MASK(pos))

// Data structure types for mapping ini-to-data
typedef void (*tIniFunc)(char *str, int write); // read/write func.prototype

struct tEntry {
  const char *name; // name of value before =
  void *data;       // address of data of access function
  enum eType type;  // see above
};
struct tSection {
  const char *name;             // section name, the one between []
  const struct tEntry *entries; // array of tEntry
  int nEntry;                   // size of array
};
struct tIni {
  const struct tSection *sections; // array of tSection
  int nSection;                    // size of array
};

/******************************************************************************
* Returns    : number of values read, excluding sections
* - ini      : the mapping to your internal data
* - fileName : filename, or address of a memory/EEPROM block number, whatever
* Side effect: update any internal value defined in the ini file
******************************************************************************/
int uCiniParse(const struct tIni *ini, char *fileName);

/******************************************************************************
* Returns    : number of values printed, excluding sections
* - ini      : the mapping to your internal data
* - fileName : filename, or address of a memory/EEPROM block number, whatever
* Side effect: serialize your ENTIRE data structure into ini-file
******************************************************************************/
int uCiniDump(const struct tIni *ini, char *fileName);

/******************************************************************************
* Returns    : 1 if str holds a valid decimal number
* - str      : string holding decimal number
* - pNum     : address of variable number is scanned into
* Side effect: update *pNum only if str is a valid decimal number
******************************************************************************/
int sscand(char *str, long *pNum);

/******************************************************************************
* - str      : string decimal number is CONCATENATED to
* - num      : number to print
******************************************************************************/
void scatd(char *str, long num);

#endif
