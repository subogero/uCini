/******************************************************************************
* (c)      SZABO Gergely   szg@subogero.com
* License  LGPL v3         http://www.gnu.org/licenses/lgpl.html
******************************************************************************/
#ifndef __stdio_
#define __stdio_

typedef struct {
} FILE_;

FILE_* stdin, stdout, stderr;

FILE_* fopen_(char* filename, const char* mode); // filename is block ID for now
int    fclose_(FILE_* stream);
char*  fgets_(char* str, int num, FILE_* stream);
int    fputs_(char* str, FILE_* stream);
#endif
