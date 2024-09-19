#include "sound.h"
#include <libcd.h>
#include <stdlib.h>
#include "utils.h"

#define SOUND_MALLOC_MAX 10

static SpuCommonAttr spucommonattr;
static SpuVoiceAttr spuvoiceattr;

static u_long vagspuaddr;
static char spumallocrec[SPU_MALLOC_RECSIZ * (SOUND_MALLOC_MAX + 1)];

static CdlLOC loc[100];
static int numtoc;

void SoundInit(void) {
  SpuInit();

  SpuInitMalloc(SOUND_MALLOC_MAX, spumallocrec);

  // Set common attributes (master volume left/right, CD volume, etc.)
  spucommonattr.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR | SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR | SPU_COMMON_CDMIX);

  spucommonattr.mvol.left  = 0x3FFF;	    // Master volume (left) - should be between 0x0000 and 0x3FFF
  spucommonattr.mvol.right = 0x3FFF;	    // Master volume (right)

  spucommonattr.cd.volume.left  = 0x7FFF; // CD volume (left)     -> should be between 0x0000 and 0x7FFF
  spucommonattr.cd.volume.right = 0x7FFF; // CD volume (right)

  spucommonattr.cd.mix = SPU_ON;          // Enable CD input on

  SpuSetCommonAttr(&spucommonattr);
}

char *LoadVAGSound(char *filename, u_long *length) {
  u_long i;
  u_char *bytes;

  bytes = (u_char*) FileRead(filename, length);

  if (bytes == NULL) {
    printf("Error reading %s from the CD.\n", filename);
    return NULL;
  }

  return (char *)bytes;
}

void TransferVAGToSpu(char *data, u_long length, int voicechannel) {
  SpuSetTransferMode(SpuTransByDMA);         // Transfer can be done by IO or by DMA
  vagspuaddr = SpuMalloc(length);            // Allocate SPU memory for first sound
  SpuSetTransferStartAddr(vagspuaddr);       // Set transfer starting address to allocated area
  SpuWrite(data, length);                    // Perform the actual transfer of sound data to SPU
  SpuIsTransferCompleted(SPU_TRANSFER_WAIT); // Wait for transfer to complete
  spuvoiceattr.mask = (
    SPU_VOICE_VOLR |
    SPU_VOICE_PITCH |
    SPU_VOICE_WDSA |
    SPU_VOICE_ADSR_AMODE |
    SPU_VOICE_ADSR_SMODE |
    SPU_VOICE_ADSR_RMODE |
    SPU_VOICE_ADSR_AR |
    SPU_VOICE_ADSR_DR |
    SPU_VOICE_ADSR_SR |
    SPU_VOICE_ADSR_RR |
    SPU_VOICE_ADSR_SL
  );
  spuvoiceattr.voice = voicechannel;
  spuvoiceattr.volume.left  = 0x1FFF;         // Left volume
  spuvoiceattr.volume.right = 0x1FFF;         // Right volume
  spuvoiceattr.pitch = 0x800;                 // Pitch
  spuvoiceattr.addr = vagspuaddr;             // Waveform data start address
  spuvoiceattr.a_mode = SPU_VOICE_LINEARIncN; // Attack curve
  spuvoiceattr.s_mode = SPU_VOICE_LINEARIncN; // Sustain curve
  spuvoiceattr.r_mode = SPU_VOICE_LINEARIncN; // Release curve
  spuvoiceattr.ar = 0x00;                     // Attack rate
  spuvoiceattr.dr = 0x00;                     // Decay rate
  spuvoiceattr.sr = 0x00;                     // Sustain rate
  spuvoiceattr.rr = 0x00;                     // Release rate
  spuvoiceattr.sl = 0x00;                     // Sustain level
  SpuSetVoiceAttr(&spuvoiceattr);             // Set the SPU voice attributes
}

void AudioPlay(int voicechannel) {
  SpuSetKey(SpuOn, voicechannel);
}

void AudioFree(unsigned long address) {
  SpuFree(address);
}

void PlayAudioTrack(u_short tracknum) {
  u_int i;
  u_char param[4];
  u_char result[8];

  // Transfer can be done by IO or by DMA
  SpuSetTransferMode(SpuTransByDMA);

  // Get CD TOC (table of contents)
  while ((numtoc = CdGetToc(loc)) == 0) {
    printf("No TOC found: Please use CD-DA disc...\n");
  }

  printf("NUMTOC = %d\n", numtoc);

  // Prevent out of bounds pos
  for (i = 1; i < numtoc; i++) {
    CdIntToPos(CdPosToInt(&loc[i]) - 74, &loc[i]);
    printf("TOC[%d] ---> Track=%d --> Minute=%d --> Second=%d\n", i, loc[i].track, loc[i].minute, loc[i].second);
  }

  // Set CD parameters: Report mode ON, CD-DA ON, see LibOvr.pdf pg. 188
  param[0] = CdlModeRept | CdlModeDA;

  // Set mode
  CdControlB(CdlSetmode, param, 0);

  // Wait three VSyncs
  VSync(3);

  // Play the track in TOC array
  CdControlB(CdlPlay, (u_char*) &loc[tracknum], 0);
}
