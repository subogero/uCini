/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  WTFPL v2        http://www.wtfpl.net
*******************************************************************************
* A lightweight library to read and write ini files in embedded uC systems.
*
* Parsing rules for ini formatted files:
* - Section names: between [] on the beginning of line
* - Entry names  : before 1st '=' of line
* - Values       : between 1st '=' and linefeed (CR/LF)
* Everything is case sensitive. Whitespace is part of each token.
*
* Mapping to internal data:
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
  eType_MASK_NUM  = 0x07, // Bit 0-2: size or flag position
  eType_MASK_ALTT = 0x08, // Bit 3  : alternate types
  eType_SGND      = 0x10, // Bit 4  : signed
  eType_MASK_TYPE = 0xE0, // Bit 5-7: type
  eType_SZ        = 0x00, // Type string
  eType_INT       = 0x20, // Type char/short/long integer
  eType_FLAG      = 0x40, // Type flag in char
  eType_FUNC      = 0x60, // Type read/write function
  eType_BITF1     = 0x20, // Alternate Type 2-bit field
  eType_BITF2     = 0x40, // Alternate Type 2-bit field
  eType_BITF3     = 0x60, // Alternate Type 3-bit field
  eType_BITF4     = 0x80, // Alternate Type 4-bit field
  eType_BITF5     = 0xA0, // Alternate Type 5-bit field
  eType_BITF6     = 0xC0, // Alternate Type 6-bit field
  eType_BITF7     = 0xE0  // Alternate Type 7-bit field
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
* Returns: zero if EOF found
* - line : buffer to read line into
* - words: address of an array of strings for section, key and value
* Side effect: words filled up with sectionname, key and value,
*              if found, otherwise NULL
******************************************************************************/
enum { TSEC, TKEY, TVAL };
void uCiniParseLine(char *line, char *words[]);

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
