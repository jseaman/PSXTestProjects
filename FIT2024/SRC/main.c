#include <stdlib.h>
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "inline_n.h"
#include "globals.h"
#include "display.h"
#include "joypad.h"
#include "camera.h"
#include "object.h"
#include "utils.h"
#include "sound.h"
#include "libcd.h"
#include "trig_lut.h"

extern char __heap_start, __sp;

POLY_FT4 *polyft4;

MATRIX worldmat = {0};
MATRIX viewmat = {0};

Camera camera;
short camera_yaw = 0, camera_pitch = 0, camera_roll = 0;

Object object;

u_long timmode;  /* Pixel mode of the TIM */
RECT   timprect; /* Stores the X,Y location of the texture */
RECT   timcrect; /* Stores the X,Y location of the CLUT */

///////////////////////////////////////////////////////////////////////////////
// Load texture data from the CD (TIM file)
///////////////////////////////////////////////////////////////////////////////
void LoadTexture(char *filename) {
  u_long i, b, length;
  u_long *bytes;
  TIM_IMAGE tim;

  // Start reading bytes from the CD-ROM
  bytes = (u_long*) FileRead(filename, &length);

  OpenTIM(bytes);
  ReadTIM(&tim);

  // Load texture into VRAM and wait for copy to complete
  LoadImage(tim.prect, tim.paddr);
  DrawSync(0);

  // Load CLUT into VRAM (if present) and wait for copy to complete
  if (tim.mode & 0x8) {
    LoadImage(tim.crect, tim.caddr);
    DrawSync(0);
  }

  // Saving a backup of the rectangle and mode values before we deallocate
  timmode = tim.mode;
  timprect = *tim.prect;
  timcrect = *tim.crect;

  // Deallocate the file buffer
  free(bytes);
}

///////////////////////////////////////////////////////////////////////////////
// Load object data from the CD (vertices, faces, and colors)
///////////////////////////////////////////////////////////////////////////////
void LoadModel(char *filename) {
  u_long i, b, length;
  char *bytes;

  // Start reading bytes from the CD-ROM
  bytes = FileRead(filename, &length);

  b = 0;

  // Read the vertices from the file
  object.numverts = GetShortBE(bytes, &b);
  object.vertices = malloc3(object.numverts * sizeof(SVECTOR));
  for (i = 0; i < object.numverts; i++) {
    object.vertices[i].vx = GetShortBE(bytes, &b);
    object.vertices[i].vy = GetShortBE(bytes, &b);
    object.vertices[i].vz = GetShortBE(bytes, &b);
    printf("VERTICES[%d] X=%d, Y=%d, Z=%d\n", i, object.vertices[i].vx, object.vertices[i].vy, object.vertices[i].vz);
  }

  // Read the face indices from the file
  object.numfaces = GetShortBE(bytes, &b) * 4; // 4 indices per quad
  object.faces = malloc3(object.numfaces * sizeof(short));
  for (i = 0; i < object.numfaces; i++) {
    object.faces[i] = GetShortBE(bytes, &b);
    printf("FACES[%d] %d\n", i, object.faces[i]);
  }

  // Read the color values from the file
  object.numcolors = GetChar(bytes, &b);
  object.colors = malloc3(object.numcolors * sizeof(CVECTOR));
  for (i = 0; i < object.numcolors; i++) {
    object.colors[i].r  = GetChar(bytes, &b);
    object.colors[i].g  = GetChar(bytes, &b);
    object.colors[i].b  = GetChar(bytes, &b);
    object.colors[i].cd = GetChar(bytes, &b);
    printf("COLORS[%d] R=%d, G=%d, B=%d\n", i, object.colors[i].r, object.colors[i].g, object.colors[i].b);
  }

  // Deallocate the file buffer after reading
  free(bytes);
}

///////////////////////////////////////////////////////////////////////////////
// Setup function that is called once at the beginning of the execution
///////////////////////////////////////////////////////////////////////////////
void Setup(void) {
  // Init Heap
  
  InitHeap3((unsigned long *)&__heap_start, (&__sp - 0x5000) - &__heap_start);

  InitTrigTables();

  // Initialize Sound
  SoundInit();

  // Setup the display environment
  ScreenInit();

  // Initialize the CD subsystem
  CdInit();

  // Initializes the joypad
  JoyPadInit();

  // Reset next primitive pointer to the start of the primitive buffer
  ResetNextPrim(GetCurrBuff());

  // Initializes the camera object
  setVector(&camera.position, 0, 0, -1200);
  camera.lookat = (MATRIX){0};

  // Initialize the object initial values
  setVector(&object.position, 0, 0, 0);
  setVector(&object.rotation, 0, 0, 0);
  setVector(&object.scale, ONE, ONE, ONE);

  LoadModel("\\MODEL.BIN;1");
  LoadTexture("\\FIT64_4.TIM;1");

  PlayAudioTrack(2);
}

short AdjustRotation (short rot)
{
  if (rot > ONE)
    {
      rot -= ONE;
    }
    else if (rot < 0)
    {
      rot += ONE;
    }

  return rot;
}

void MoveCamera(VECTOR *fwd)
{
  if (!IsJoyPadAnalog(0))
  {
    fwd->vx = fwd->vy = 0;
    fwd->vz = ONE;
    return;
  }

  unsigned char Left_Horizontal, Left_Vertical, Right_Horizontal, Right_Vertical;
  JoyPadGetAnalogState(&Left_Horizontal, &Left_Vertical, &Right_Horizontal, &Right_Vertical);

  short MoveYaw = (Right_Horizontal - 128) >> 3;

  if (abs(MoveYaw) >= 8)
  {
    camera_yaw += MoveYaw;
    camera_yaw = AdjustRotation(camera_yaw);
  }

  short MovePitch = (Right_Vertical - 128) >> 3;

  if (abs(MovePitch) >= 8)
  {
    camera_pitch -= MovePitch;
    camera_pitch = AdjustRotation(camera_pitch);
  }

  short sinx = sin_lut(camera_pitch);
  short cosx = cos_lut(camera_pitch);
  short siny = sin_lut(camera_yaw);
  short cosy = cos_lut(camera_yaw);
  short sinz = sin_lut(camera_roll);
  short cosz = cos_lut(camera_roll);

  fwd->vx = (siny * cosx) >> 12;
  fwd->vy = (-sinx);
  fwd->vz = (cosy * cosx) >> 12;
  fwd->pad = 0;

  //printf("Camera Yaw: %d, Forward: (%d, %d, %d)\n", camera_yaw, fwd.vx, fwd.vy, fwd.vz);

  VECTOR right;
  right.vx = ((cosx * cosz) >> 12) + ((((siny * sinx) >> 12) * sinz) >> 12);
  right.vy = (cosx * sinz) >> 12;
  right.vz = ((-siny * cosz) >> 12) + ((((cosy * sinx) >> 12) * sinz) >> 12);

  short MoveX = (Left_Horizontal - 128) >> 2;
  short MoveZ = (Left_Vertical - 128) >> 2;
  
  if (abs(MoveX) >= 8)
  {
    camera.position.vx += (right.vx * MoveX) >> 12;
    camera.position.vy += (right.vy * MoveX) >> 12;
    camera.position.vz += (right.vz * MoveX) >> 12;
  }

  if (abs(MoveZ) >= 8)
  {
    camera.position.vx -= (fwd->vx * MoveZ) >> 12;
    camera.position.vy -= (fwd->vy * MoveZ) >> 12;
    camera.position.vz -= (fwd->vz * MoveZ) >> 12;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Setup function that is called once at the beginning of the execution
///////////////////////////////////////////////////////////////////////////////
void Update(void) {
  int i, q, nclip;
  long otz, p, flg;
  u_long pad;

  // Empty the Ordering Table
  EmptyOT(GetCurrBuff());

  // Update the state of the controller
  JoyPadUpdate();
  JoyPadCheckAnalog(0);

  VECTOR fwd;
  MoveCamera(&fwd);

  // Compute the camera Lookat matrix for this frame
  //LookAt(&camera, &camera.position, &object.position, &(VECTOR){0, -ONE, 0});
  LookAt(&camera, &camera.position, &fwd, &(VECTOR){0, -ONE, 0});

  // Draw the object
  RotMatrix(&object.rotation, &worldmat);
  TransMatrix(&worldmat, &object.position);
  ScaleMatrix(&worldmat, &object.scale);

  // Create the View Matrix combining the world matrix & lookat matrix
  CompMatrixLV(&camera.lookat, &worldmat, &viewmat);

  SetRotMatrix(&viewmat);
  SetTransMatrix(&viewmat);

  // Loop all face indices and quads of our model
  for (i = 0, q = 0; i < object.numfaces; i += 4, q++) {
    polyft4 = (POLY_FT4*) GetNextPrim();
    setPolyFT4(polyft4);

    polyft4->r0 = 128;
    polyft4->g0 = 128;
    polyft4->b0 = 128;

    polyft4->u0 =  0;   polyft4->v0 =  0;
    polyft4->u1 = 63;   polyft4->v1 =  0;
    polyft4->u2 =  0;   polyft4->v2 = 63;
    polyft4->u3 = 63;   polyft4->v3 = 63;

    polyft4->tpage = getTPage(timmode & 0x3, 0, timprect.x, timprect.y);
    polyft4->clut  = getClut(timcrect.x, timcrect.y);

    // Loading the first 3 vertices (the GTE can only perform a max. of 3 vectors at a time)
    gte_ldv0(&object.vertices[object.faces[i + 0]]);
    gte_ldv1(&object.vertices[object.faces[i + 1]]);
    gte_ldv2(&object.vertices[object.faces[i + 2]]);

    gte_rtpt();

    gte_nclip();
    gte_stopz(&nclip);

    // Bypass and cull the faces that are looking away from the camera
    if (nclip < 0) {
      continue;
    }

    // Store/save the transformed projected coord of the first vertex
    gte_stsxy0(&polyft4->x0);

    // Load the last 4th vertex
    gte_ldv0(&object.vertices[object.faces[i + 3]]);

    // Project & transform the remaining 4th vertex
    gte_rtps();

    // Store the transformed last vertices
    gte_stsxy3(&polyft4->x1, &polyft4->x2, &polyft4->x3);

    gte_avsz4();
    gte_stotz(&otz);

    if ((otz > 0) && (otz < OT_LEN)) {
      addPrim(GetOTAt(GetCurrBuff(), otz), polyft4);
      IncrementNextPrim(sizeof(POLY_FT4));
    }
  }
  object.rotation.vy += 10;
}

///////////////////////////////////////////////////////////////////////////////
// Render function that invokes the draw of the current frame
///////////////////////////////////////////////////////////////////////////////
void Render(void) {
  DisplayFrame();
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(void) {
  Setup();
  while (1) {
    Update();
    Render();
  }
  return 0;
}
