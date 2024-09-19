#include "utils.h"
#include "globals.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <libcd.h>

char *FileRead(char *filename, u_long *length) {
  CdlFILE filepos;
  int numsectors;
  char *buffer;

  buffer = NULL;

  if (CdSearchFile(&filepos, filename) == NULL) {
    printf("%s file not found in the CD.", filename);
  } else {
    printf("Found %s in the CD.", filename);
    numsectors = (filepos.size + 2047) / 2048;              // compute the number of sectors to read from the file
    buffer = (char*) malloc3(2048 * numsectors);             // allocate buffer for the file
    if (!buffer) {
      printf("Error allocating %d sectors!\n", numsectors);
    }
    CdControl(CdlSetloc, (u_char*) &filepos.pos, 0);        // set read target to the file
    CdRead(numsectors, (u_long*) buffer, CdlModeSpeed);     // start reading from the CD
    CdReadSync(0, 0);                                       // wait until the read is complete
  }

  *length = filepos.size;

  return buffer;
}

char GetChar(u_char *bytes, u_long *b) {
  return bytes[(*b)++];
}

short GetShortLE(u_char *bytes, u_long *b) {
  u_short value = 0;
  value |= bytes[(*b)++] << 0;
  value |= bytes[(*b)++] << 8;
  return (short) value;
}

short GetShortBE(u_char *bytes, u_long *b) {
  u_short value = 0;
  value |= bytes[(*b)++] << 8;
  value |= bytes[(*b)++] << 0;
  return (short) value;
}

long GetLongLE(u_char *bytes, u_long *b) {
  u_long value = 0;
  value |= bytes[(*b)++] <<  0;
  value |= bytes[(*b)++] <<  8;
  value |= bytes[(*b)++] << 16;
  value |= bytes[(*b)++] << 24;
  return (long) value;
}

long GetLongBE(u_char *bytes, u_long *b) {
  u_long value = 0;
  value |= bytes[(*b)++] << 24;
  value |= bytes[(*b)++] << 16;
  value |= bytes[(*b)++] <<  8;
  value |= bytes[(*b)++] <<  0;
  return (long) value;
}
