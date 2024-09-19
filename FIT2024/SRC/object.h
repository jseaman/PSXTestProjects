#ifndef OBJECT_H
#define OBJECT_H

#include <sys/types.h>
#include "globals.h"

typedef struct Object {
  SVECTOR rotation;
  VECTOR position;
  VECTOR scale;

  short numverts;
  SVECTOR *vertices;

  short numfaces;
  short *faces;

  short numcolors;
  CVECTOR *colors;
} Object;

#endif
