/*
 * xvalg.c - image manipulation algorithms (Blur, etc.)
 *
 *  Contains:
 *         void AlgInit();
 *         void DoAlg(int algnum);
 */

#include "copyright.h"

#include "xv.h"

#ifndef M_PI
#  define M_PI (3.1415926535897932385)
#endif


static void NoAlg          PARM((void));
static void Blur           PARM((void));
static void Sharpen        PARM((void));
static void EdgeDetect     PARM((void));
static void TinFoil        PARM((void));
static void OilPaint       PARM((void));
static void Blend          PARM((void));
static void FineRotate     PARM((int));
static void Pixelize       PARM((void));
static void Spread         PARM((void));
static void MedianFilter   PARM((void));

void saveOrigPic    PARM((void));


int         start24bitAlg  PARM((byte **, byte **));
void        end24bitAlg    PARM((byte *, byte *));

static byte origrmap[256], origgmap[256], origbmap[256];




/************************************************************/
void AlgInit(void)
{
  /* called whenver an image file is loaded.  disposes of origPic
     if neccessary, and points it to null */

  if (origPic) free(origPic);
  origPic = (byte *) NULL;

  algMB.dim[ALG_NONE] = 1;    /* can't undo when init'ed already */
  flmaskMB.dim[MSK_NONE]=1;
}


/************************/
void DoAlg(int anum)
{
  /* called with a value from the algMB button.  Executes the specified
     algorithm */

  switch (anum) {
  case ALG_NONE:      NoAlg();        	break;
  case ALG_BLUR:      Blur();         	break;
  case ALG_SHARPEN:   Sharpen();      	break;
  case ALG_EDGE:      EdgeDetect();   	break;
  case ALG_TINF:      TinFoil();      	break;
  case ALG_OIL:       OilPaint();     	break;
  case ALG_BLEND:     Blend();        	break;
  case ALG_ROTATE:    FineRotate(0);  	break;
  case ALG_ROTATECLR: FineRotate(1);  	break;
  case ALG_PIXEL:     Pixelize();     	break;
  case ALG_SPREAD:    Spread();       	break;
  case ALG_MEDIAN:    MedianFilter(); 	break;
  }

  algMB.dim[ALG_NONE] = (origPic == (byte *) NULL);
  flmaskMB.dim[MSK_NONE] = (origPic == (byte *) NULL);
}



/************************/
static void NoAlg(void)
{
  int i;

  /* restore original picture */
  if (!origPic) return;  /* none to restore */

  WaitCursor();

  KillOldPics();   /* toss the old pic/cpic/epic/theImage away */

  picType = origPicType;
  Set824Menus(picType);

  pic = origPic;  origPic = NULL;

  if (picType == PIC8) {
    for (i=0; i<256; i++) {
      rMap[i] = origrmap[i];
      gMap[i] = origgmap[i];
      bMap[i] = origbmap[i];
    }
  }

  InstallNewPic();
}


/************************/
static void Blur(void)
{
  /* runs a n*n convolution mask (all 1's) over 'pic',
     producing a 24-bit version.  Then calls 24to8 to generate a new 8-bit
     image, and installs it.

     Note that 'n' must be odd for things to work properly */

  byte              *pic24, *tmpPic;
  int                i, sx,sy,sw,sh, n;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '3', '\0' };

  sprintf(txt, "Blur:                                   \n\n%s",
	  "Enter mask size (ex. 3, 5, 7, ...)");

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789", 1);
  if (i==1 || strlen(buf)==0) return;
  n = atoi(buf);

  if (n < 1 || (n&1)!=1) {
    ErrPopUp("Error:  The value entered must be odd and greater than zero.",
	     "\nOh!");
    return;
  }

  WaitCursor();

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  SetISTR(ISTR_INFO, "Blurring %s with %dx%d convolution mask...",
	  (HaveSelection() ? "selection" : "image"), n,n);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doBlurConvolv(pic24, pWIDE,pHIGH, tmpPic, sx,sy,sw,sh, n);

  end24bitAlg(pic24, tmpPic);
}



/************************/
static void Sharpen(void)
{
  /* runs an edge-enhancment algorithm */

  byte              *pic24, *tmpPic;
  int                i, sx,sy,sw,sh, n;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '7', '5', '\0' };

  sprintf(txt, "Sharpen:                                   \n\n%s",
	  "Enter enhancement factor (0-99%)");

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789", 1);
  if (i==1 || strlen(buf)==0) return;
  n = atoi(buf);

  if (n>=100) {
    ErrPopUp("Error:  The value entered must be less than 100%.", "\nOh!");
    return;
  }

  WaitCursor();

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  SetISTR(ISTR_INFO, "Sharpening %s by factor of %d%%...",
	  (HaveSelection() ? "selection" : "image"), n);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doSharpConvolv(pic24, pWIDE,pHIGH, tmpPic, sx,sy,sw,sh, n);

  end24bitAlg(pic24, tmpPic);
}



/************************/
static void EdgeDetect(void)
{
  byte *pic24, *p24, *tmpPic, *tlp;
  const char *str;
  int  i, j, v, maxv, sx,sy,sw,sh;

  WaitCursor();

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  str = "Doing edge detection...";
  SetISTR(ISTR_INFO, str);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doEdgeConvolv(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh);

  SetISTR(ISTR_INFO, "%snormalizing...", str);

  /* Normalize results */
  for (i=sy, maxv=0; i<sy+sh; i++) {              /* compute max value */
    p24 = tmpPic + (i*pWIDE + sx) * 3;
    for (j=sx; j<sx+sw; j++, p24+=3) {
      v = MONO(p24[0], p24[1], p24[2]);
      if (v>maxv) maxv = v;
    }
  }

  for (i=sy; maxv>0 && i<sy+sh; i++) {            /* normalize */
    p24 = tlp = tmpPic + (i*pWIDE + sx) * 3;
    for (j=0; j<sw*3; j++) {
      v = (((int) *p24) * 255) / maxv;
      RANGE(v,0,255);
      *p24++ = (byte) v;
    }
  }

  end24bitAlg(pic24, tmpPic);
}


/************************/
static void TinFoil(void)
{
  byte *pic24, *tmpPic, *tp, *tlp;
  const char *str;
  int  i, j, v, sx,sy,sw,sh;

  WaitCursor();

  str = "Doing cheesy embossing effect...";
  SetISTR(ISTR_INFO, str);

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  /* fill selected area of tmpPic with gray128 */
  for (i=sy; i<sy+sh; i++) {
    tp = tlp = tmpPic + (i*pWIDE + sx) * 3;
    for (j=sx; j<sx+sw; j++) {
      *tp++ = 128;  *tp++ = 128;  *tp++ = 128;
    }
  }

  doAngleConvolv(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh);

  /* mono-ify selected area of tmpPic */
  for (i=sy; i<sy+sh; i++) {
    tp = tlp = tmpPic + (i*pWIDE + sx) * 3;
    for (j=sx; j<sx+sw; j++, tp+=3) {
      v = MONO(tp[0], tp[1], tp[2]);
      RANGE(v,0,255);
      tp[0] = tp[1] = tp[2] = (byte) v;
    }
  }

  end24bitAlg(pic24, tmpPic);
}


/************************/
static void OilPaint(void)
{
  byte *pic24, *tmpPic;
  int   sx,sy,sw,sh;

  WaitCursor();

  SetISTR(ISTR_INFO, "Doing oilpaint effect...");

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doOilPaint(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh, 3);

  end24bitAlg(pic24, tmpPic);
}



/************************/
static void Blend(void)
{
  byte *pic24, *tmpPic;
  int   sx,sy,sw,sh;

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  WaitCursor();

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doBlend(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh);

  end24bitAlg(pic24, tmpPic);
}


/************************/
static void FineRotate(int clr)
{
  byte              *pic24, *tmpPic;
  int                i,sx,sy,sw,sh;
  double             rotval;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '\0' };

  sprintf(txt, "Rotate (%s):\n\nEnter rotation angle, in degrees:  (>0 = CCW)",
	  (clr ? "Clear" : "Copy"));

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789.-", 1);
  if (i==1 || strlen(buf)==0) return;
  rotval = atof(buf);

  if (rotval == 0.0) return;


  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  WaitCursor();

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doRotate(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh, rotval, clr);

  end24bitAlg(pic24, tmpPic);
}


/************************/
static void Pixelize(void)
{
  byte              *pic24, *tmpPic;
  int                i,sx,sy,sw,sh, pixX,pixY,err;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '4', '\0' };

  sprintf(txt, "Pixelize:\n\nEnter new pixel size, in image pixels:  %s",
	  "(ex. '3', '5x8')");

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789x", 1);
  if (i==1 || strlen(buf)==0) return;

  pixX = pixY = err = 0;

  if (index(buf, 'x')) {
    if (sscanf(buf, "%d x %d", &pixX, &pixY) != 2) err++;
  }
  else {
    if (sscanf(buf, "%d", &pixX) != 1) err++;
    pixY = pixX;
  }

  if (pixX<1 || pixY<1 || err) {
    ErrPopUp("Error:  The entered string is not valid.", "\nWho Cares!");
    return;
  }


  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  WaitCursor();

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doPixel(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh, pixX, pixY);

  end24bitAlg(pic24, tmpPic);
}



/************************/
static void Spread(void)
{
  byte              *pic24, *tmpPic;
  int                i,sx,sy,sw,sh, pixX,pixY,err;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '5', '\0' };

  sprintf(txt, "Spread:\n\nEnter spread factor (or x,y factors):  %s",
	  "(ex. '10', '1x5')");

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789x", 1);
  if (i==1 || strlen(buf)==0) return;

  pixX = pixY = err = 0;

  if (index(buf, 'x')) {
    if (sscanf(buf, "%d x %d", &pixX, &pixY) != 2) err++;
    if (pixX<0 || pixY<0) err++;
  }
  else {
    if (sscanf(buf, "%d", &pixX) != 1) err++;
    pixX = -pixX;
  }

  if (!pixX && !pixY) return;     /* 0x0 has no effect */

  if (err) {
    ErrPopUp("Error:  The entered string is not valid.", "\nBite Me!");
    return;
  }


  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  WaitCursor();

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doSpread(pic24, pWIDE, pHIGH, tmpPic, sx,sy,sw,sh, pixX, pixY);

  end24bitAlg(pic24, tmpPic);
}



/************************/
static void MedianFilter(void)
{
  /* runs median filter algorithm (for n*n rect centered around each pixel,
     replace with median value */

  byte              *pic24, *tmpPic;
  int                i, sx,sy,sw,sh, n;
  static const char *labels[] = { "\nOk", "\033Cancel" };
  char               txt[256];
  static char        buf[64] = { '3', '\0' };

  sprintf(txt, "DeSpeckle (median filter):                          \n\n%s",
	  "Enter mask size (ex. 3, 5, 7, ...)");

  i = GetStrPopUp(txt, labels, 2, buf, 64, "0123456789", 1);
  if (i==1 || strlen(buf)==0) return;
  n = atoi(buf);

  if (n < 1 || (n&1)!=1) {
    ErrPopUp("Error:  The value entered must be odd and greater than zero.",
	     "\nOh!");
    return;
  }

  WaitCursor();

  if (HaveSelection()) GetSelRCoords(&sx,&sy,&sw,&sh);
  else { sx = 0;  sy = 0;  sw = pWIDE;  sh = pHIGH; }
  CropRect2Rect(&sx,&sy,&sw,&sh, 0,0,pWIDE,pHIGH);

  SetISTR(ISTR_INFO, "DeSpeckling %s with %dx%d convolution mask...",
	  (HaveSelection() ? "selection" : "image"), n,n);

  if (start24bitAlg(&pic24, &tmpPic)) return;
  xvbcopy((char *) pic24, (char *) tmpPic, (size_t) (pWIDE*pHIGH*3));

  doMedianFilter(pic24, pWIDE,pHIGH, tmpPic, sx,sy,sw,sh, n);

  end24bitAlg(pic24, tmpPic);
}





/***********************************************/
int start24bitAlg(byte **pic24, byte **tmpPic)
{
  /* generates a 24-bit version of 'pic', if neccessary, and also mallocs
   * a pWIDE*pHIGH*3 24-bit output pic.
   *
   * Returns '1' if there's some sort of screwup, '0' if cool
   */


  if (picType == PIC8) {
    *pic24 = Conv8to24(pic, pWIDE, pHIGH, rMap, gMap, bMap);
    if (!*pic24) { SetCursors(-1);  return 1; }
  }
  else *pic24 = pic;


  /* need to create another w*h*3 pic to hold results */
  *tmpPic = (byte *) calloc((size_t) (pWIDE * pHIGH * 3), (size_t) 1);
  if (!(*tmpPic)) {
    SetCursors(-1);
    ErrPopUp("Unable to malloc() tmp 24-bit image in start24bitAlg()",
	     "\nTough!");
    if (picType == PIC8) free(*pic24);
    return 1;
  }

  return 0;
}



/***********************************************/
void end24bitAlg(byte *pic24, byte *outPic)
{
  /* given pic24, and outPic, which has the new 24-bit image, installs it */


  saveOrigPic();  /* also kills pic/cpic/epic/egampic/theImage, NOT pic24 */

  /* copy results to pic24 */
  xvbcopy((char *) outPic, (char *) pic24, (size_t) (pWIDE*pHIGH*3));
  free(outPic);

  if (picType == PIC8) {
    pic = Conv24to8(pic24, pWIDE, pHIGH, ncols, rMap,gMap,bMap);
    free(pic24);
    if (!pic) {
      SetCursors(-1);
      ErrPopUp("Some sort of failure occured in 24to8 conversion\n","\nDamn!");
      NoAlg();
      return;
    }
  }
  else pic = pic24;

  InstallNewPic();
}


/************************/
void saveOrigPic(void)
{
  /* saves original picture into origPic, if it hasn't already been done.
     This allows us to undo algorithms...

     Also, frees all pics, (except 'pic', if we're in PIC24 mode) */


  int i;

  FreeEpic();
  if (cpic && cpic != pic) free(cpic);
  xvDestroyImage(theImage);
  theImage = NULL;
  cpic = NULL;

  if (!origPic) {
    /* make a backup copy of 'pic' */
    origPic = (byte *) malloc((size_t)(pWIDE*pHIGH*((picType==PIC8) ? 1 : 3)));
    if (!origPic) FatalError("out of memory in 'saveOrigPic()'");
    xvbcopy((char *) pic, (char *) origPic,
	    (size_t) (pWIDE * pHIGH * ((picType==PIC8) ? 1 : 3)));

    origPicType = picType;

    if (picType == PIC8) {
      for (i=0; i<256; i++) {   /* save old colormap */
	origrmap[i] = rorg[i];
	origgmap[i] = gorg[i];
	origbmap[i] = borg[i];
      }
    }
  }


  if (picType != PIC24) {  /* kill pic, as well */
    if (pic) free(pic);
    pic = NULL;
  }
}

