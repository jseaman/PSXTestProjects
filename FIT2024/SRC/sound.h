#ifndef SOUND_H
#define SOUND_H

#include "globals.h"
#include <libspu.h>

void SoundInit(void);
char *LoadVAGSound(char *filename, u_long *length);
void TransferVAGToSpu(char *data, u_long length, int voicechannel);

void PlayAudioTrack(u_short tracknum);

#endif
