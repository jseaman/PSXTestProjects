#include "joypad.h"
#include <sys/types.h>
#include <libetc.h>
#include <libpad.h>

#define MultiTap	1
#define CtrlTypeMtap	8
#define PortPerMtap	4

#define BtnM0		1
#define BtnM1		2
#define BtnMode		4
#define BtnStart	8
#define BtnEnable	0x10

typedef struct
{
	/* button state at last V-Sync*/
	unsigned char Button;

	/* lock/unlock mode for setting terminal type alternation button
 */
	unsigned char Lock;

	/* value for setting actuators*/
	unsigned char Motor0,Motor1;

	/* flag for "already called PadSetActAlign()"
 */
	unsigned char Send;

	/* target coordinate of gun (which use interrupt)
 */
	int X,Y;
} HISTORY;

static u_long padstate;
static HISTORY history[2][4];
static unsigned char padbuff[2][34];

int JoyPadCheck(int p) {
  return padstate & p;
}

void JoyPadInit(void) {
  PadInit(0);
  PadInitMtap(padbuff[0],padbuff[1]);
  PadStartCom();
}

void JoyPadReset(void) {
  padstate = 0;
}

void JoyPadUpdate(void) {
  u_long pad = PadRead(0);
  padstate = pad;
}

void JoyPadGetAnalogState(unsigned char *A, unsigned char *B, unsigned char *C, unsigned char *D)
{
  *A = padbuff[0][6];
  *B = padbuff[0][7];
  *C = padbuff[0][4];
  *D = padbuff[0][5];
}

/*#setPad : analyze button state to act expanded-protocol controller functions
 */
int setPad(int port, unsigned char *rxbuf)
{
	HISTORY *hist;
	int button,count;

	if(rxbuf[1]>>4 == CtrlTypeMtap)
	{
		for(count=0;count<PortPerMtap;++count)
		{
			if (setPad(port+count, rxbuf + 2 +count*8) == 1){
				VSync(2);
				return 1;
			}
		}
		return 0;
	}

	/* ignore received data when communication failed
 */
	if(*rxbuf)
	{
		return 0;
	}

	button = ~((rxbuf[2]<<8) | rxbuf[3]);
	hist = &history[port>>4][port & 3];
	if (button & PADselect ){
		if (PadInfoMode(port,2,0) != 0){
                	PadSetMainMode(port,0,2);
                	VSync(2);
                	while (PadGetState(port) != 6){
                	}
		}
                return 1;
        }

#if 0
	/* suspend/resume opposite port of communication
 */
	if(!(hist->Button & BtnEnable) && button & PADRleft)
	{
		padEnable ^= (1 << (!(port>>4)));
		PadEnableCom(padEnable);
	}
#endif

	/* Confirmation for reloading controller information by calling
         PadStopCom(), PadStartCom()
 */
	if(!(hist->Button & BtnStart) && button & PADstart)
	{
		PadStopCom();
		PadStartCom();
	}

	if(PadInfoMode(port,InfoModeCurExID,0))
	{
		/* set actuator 0 level of expanded-protocol controller
 */
		if(!(hist->Button & BtnM0))
		{
			if(button & PADL1 && hist->Motor0)
			{
				hist->Motor0 -= 1;
			}
			else if(button & PADL2 && hist->Motor0 < 255)
			{
				hist->Motor0 += 1;
			}
		}

		/* set actuator 1 level of expanded-protocol controller
 */
		if(!(hist->Button & BtnM1))
		{
			if(button & PADR1 && hist->Motor1)
			{
				hist->Motor1 -= 10;
			}
			else if(button & PADR2 && hist->Motor1 < 246)
			{
				hist->Motor1 += 10;
			}
		}
		/* alternate terminal type and lock/unlock switch state
 */
		if(!(hist->Button & BtnMode))
		{
			if(button & PADLleft)
			{
				PadSetMainMode(port,0,hist->Lock);
			}
			else if(button & PADLright)
			{
				PadSetMainMode(port,1,hist->Lock);
			}
			else if(button & PADRright)
			{
				switch(hist->Lock)
				{
					case 0:
					case 2:
						hist->Lock = 3;
						break;
					case 3:
						hist->Lock = 2;
						break;
				}
				PadSetMainMode(port,
					PadInfoMode(port,InfoModeCurExOffs,0),
					hist->Lock);
			}
		}
	}
	else
	{
		/* set actuator level of SCPH-1150
 */
		if(!(hist->Button & BtnM0))
		{
			if(button & PADL1 && hist->Motor0)
			{
				hist->Motor0 = 0x40;
				hist->Motor1 -= 1;
			}
			else if(button & PADL2 && hist->Motor0 < 255)
			{
				hist->Motor0 = 0x40;
				hist->Motor1 += 1;
			}
		}
	}

	/*store button state of this V-Sync period*/
	if(button & (PADLright|PADLleft|PADRright))
		hist->Button |= BtnMode; else hist->Button &= ~BtnMode;
	if(button & (PADL1 | PADL2))
		hist->Button |= BtnM0; else hist->Button &= ~BtnM0;
	if(button & (PADR1 | PADR2))
		hist->Button |= BtnM1; else hist->Button &= ~BtnM1;
	if(button & PADRleft)
		hist->Button |= BtnEnable; else hist->Button &= ~BtnEnable;
	if(button & PADstart)
		hist->Button |= BtnStart; else hist->Button &= ~BtnStart;
	return 0;
}

int JoyPadCheckAnalog(int port)
{
  return setPad(port,padbuff[port]);
}

char IsJoyPadAnalog(int port)
{
	return PadInfoMode(port,InfoModeCurID,0) == 7;
}

