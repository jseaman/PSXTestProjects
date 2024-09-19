#include "globals.h"

static u_long ot[2][OT_LEN];            // Ordering table holding pointers to sorted primitives
static char primbuff[2][PRIMBUFF_LEN];  // Primitive buffer that holds the actual data for each primitive
static char *nextprim;                  // Pointer to the next primitive in the primitive buffer

void EmptyOT(u_short currbuff) {
  ClearOTagR(ot[currbuff], OT_LEN);
}

void SetOTAt(u_short currbuff, u_int i, u_long value) {
  ot[currbuff][i] = value;
}

u_long *GetOTAt(u_short currbuff, u_int i) {
  return &ot[currbuff][i];
}

void IncrementNextPrim(u_int size) {
  nextprim += size;
}

void ResetNextPrim(u_short currbuff) {
  nextprim = primbuff[currbuff];
}

void SetNextPrim(char *value) {
  nextprim = value;
}

char *GetNextPrim(void) {
  return nextprim;
}
