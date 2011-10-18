/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
*******************************************************************************
* Template of stdio_.h to connect uCini to your system.
* It's based on a small subset of stdio.h.
*
* If you don't have a file-system, you can pass anything to fopen_ as filename,
* like the address of a buffer, RAM/EEPROM memory block ID, or whatever.
*
* MAX_LINE_LENGTH is the max allowed line length in your ini files.
******************************************************************************/
#ifndef __stdio_
#define __stdio_

typedef struct {
} FILE_;

FILE_ *stdin_, *stdout_, *stderr_;

FILE_ *fopen_(char *filename, const char *mode);
int    fclose_(FILE_ *stream);
char  *fgets_(char *str, int num, FILE_ *stream);
int    fputs_(char *str, FILE_ *stream);

#define MAX_LINE_LENGTH 80

#endif
