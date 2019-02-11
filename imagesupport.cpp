/**************************************************************************
Initial author: Paul F. Richards (paulrichards321@gmail.com) 2005-2017
https://github.com/paulrichards321/jpg2svs

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/
#include <cstring>
#include <cstdarg>
#include <fstream>
#include "imagesupport.h"

int dprintf(const char* format, ...)
{
  int cx = 0;
  char debug[1024];
  va_list ap;
  va_start(ap, format);
  cx = vsnprintf(debug, sizeof(debug)-1, format, ap);
  puts(debug);
  va_end(ap);
  return cx;
}

Image::Image() 
{
//  dprintf("In Image::Image() constructor.\n");
  mactualWidth = 0;
  mactualHeight = 0;
  mrenderedWidth = 0;
  mrenderedHeight = 0;
  mzoomPercentage = 100;
  mupsideDown = false;
  mshiftRight = 0;
  mshiftDown = 0;
  mpageWidth = 0;
  mpageHeight = 0;
  mValidObject = false;
  mfitToWindow = false;
  mmaintainAspectRatio = true;
  mbitmapSize = 0;
  mrotation = 0;
  mpBitmap = NULL;
  mlevel = 0;
  mbkgColor = 255;
//  hPal = 0;
}

Image::~Image()
{
  cleanup();
}

void Image::cleanup()
{
  mValidObject = false;
/*
  if (hPal) 
  {
    DeleteObject(hPal);
    hPal = 0;
  }
  info.clear();
*/
  if (mpBitmap) 
  {
    delete[] mpBitmap;
    mpBitmap = NULL;
    mbitmapSize = 0;
  }
  setFileName("");
}


void Image::autoUpdatePageValues(int hWnd)
{
//  RECT rc;
//  GetClientRect(hWnd, &rc);
//  setupPageValues(rc.right, rc.bottom);
}


void Image::setupPageValues(int winWidth, int winHeight)
{
  if (mpageHeight == 0)
    setPageHeight(iround(mrenderedHeight - (mrenderedHeight - winHeight)));
  //  setPageHeight(renderedHeight > winHeight ? winHeight : renderedHeight);
  int newPageHeight = iround(mrenderedHeight - (mrenderedHeight - winHeight));
  setPageHeight(newPageHeight);
  
  // if the image is scrolled down and the window is vertically enlarged...
  int linesDisplayed = (int) mrenderedHeight - mshiftDown;
  if (mshiftDown > 0 && mpageHeight > linesDisplayed)
    setShiftDownPt(mshiftDown - (mpageHeight - linesDisplayed));

  if (mpageWidth == 0)
    setPageWidth(iround(mrenderedWidth - (mrenderedWidth - winWidth)));
  int newPageWidth = iround(mrenderedWidth - (mrenderedWidth - winWidth));
  setPageWidth(newPageWidth);

  // if the image is scrolled to the right and the window is horizontally enlarged...
  linesDisplayed = (int) mrenderedWidth - mshiftRight;
  if (mshiftRight > 0 && mpageWidth > linesDisplayed)
    setShiftRightPt(mshiftRight - (mpageWidth - linesDisplayed));
}

void Image::setPageWidth(int width)
{
  if (width < 0)
    mpageWidth = 0;
  else if (width > (int) mrenderedWidth && mfitToWindow == false)
    mpageWidth = (int) mrenderedWidth;
  else
    mpageWidth = width;
}

void Image::setPageHeight(int height)
{
  if (height < 0)
    mpageHeight = 0;
  else if (height > (int) mrenderedHeight && mfitToWindow == false)
    mpageHeight = (int) mrenderedHeight;
  else
    mpageHeight = height;
}

void Image::setShiftDownPt(int y) 
{
  if (y > (int) mrenderedHeight)
    mshiftDown = iround(mrenderedHeight);
  else if (y < 0)
    mshiftDown = 0;
  else
    mshiftDown = y;
}
    
void Image::setShiftRightPt(int x)
{
  if (x > (int) mrenderedWidth)
    mshiftRight = iround(mrenderedWidth);
  else if (x < 0)
    mshiftRight = 0;
  else
    mshiftRight = x;
}

int Image::paint(int hDC, int destx, int desty, int width, int height)
{
  return 0;
}

bool Image::rotateCW()
{
  return mupsideDown ? rotateAlgorithm2() : rotateAlgorithm1();
}

bool Image::rotateCCW()
{
  return mupsideDown ? rotateAlgorithm1() : rotateAlgorithm2();
}


// For an image that is right side up, this algorithm rotates the image clockwise
// For an image that is upside down, this algorithm rotates the image counter-clockwise
bool Image::rotateAlgorithm1()
{
  merrMsg.str("");
  
  // dprintf("Source bitmap:\n");
  //for (uint z = 0; z < bitmapSize; z++)
  //{
  //  dprintf("%X", (uint)pBitmap[z]);
    //if (z > 0 && z % 10 == 0) dprintf("\n");
  //}
  //dprintf("\n");

  int newWidth = mactualHeight;
  int newHeight = mactualWidth;

  int srcUnpaddedScanlineBits = mactualWidth * mbitCount;
  int srcPaddedScanlineBits = srcUnpaddedScanlineBits;
  while (srcPaddedScanlineBits & (sizeof(uint)*8-1)) srcPaddedScanlineBits++;
  
  int destUnpaddedScanlineBits = newWidth * mbitCount;
  int destPaddedScanlineBits = destUnpaddedScanlineBits;
  while (destPaddedScanlineBits & (sizeof(uint)*8-1)) destPaddedScanlineBits++;

  mbitmapSize = int(ceil(double(destPaddedScanlineBits)/8)) * newHeight;

  BYTE *pNewBitmap = new(std::nothrow) BYTE[mbitmapSize+sizeof(uint)];
  if (pNewBitmap == NULL) 
  {
    merrMsg << "Insufficient memory to rotate '" << mfileName << "'";
    return false;
  }
  memset(pNewBitmap, 0, mbitmapSize);

  uint src, dest; // these two values are the offset in bits
  uint x, y; // points on the source image

  for (y = 0; y < mactualHeight; y++) 
  {
    src = y*srcPaddedScanlineBits;
    
    for (x = 0; x < mactualWidth; x++, src += mbitCount) 
    {
      // read a small chunk from the image bit array
      uint srcchunk = *reinterpret_cast<uint*>(&mpBitmap[src/8]);
      // if the bit depth of image is less than 8, the bits we are trying to access
      // might be inside, or left of the srcchunk bit array
      // dprintf("\nsrcchunk location: %i\n", src);
      // dprintf("srcchunk pulled: %X\n", srcchunk);
      // also note the order of the image for bit depths less than 8 is reversed
      // in other words, for a 4 bit image the first pixel is in the left side of
      // the byte, not the right side where you would expect it
      if (mbitCount < 8)
        srcchunk = srcchunk >> ((8 - mbitCount) - (src % 8));
      // create a mask that will turn all the unwanted bits to zeros
      uint mask = ~(-1 << mbitCount);
      srcchunk = srcchunk & mask;
      // dprintf("srcchunk after mask: %X\n", srcchunk);
      
      // calculate where the source pixel goes
      dest = (x*destPaddedScanlineBits)+destUnpaddedScanlineBits;
      dest -= (y*mbitCount) + mbitCount;

      // remember that the order of the image for bit depths less than 8 is reversed
      if (mbitCount < 8)
        srcchunk = srcchunk << ((8 - mbitCount) - (dest % 8));
      // dprintf("destchunk location: %i\n", dest);
      // we need to read a chunk from the destination array in case
      // the bit depth is less than 8
      uint destchunk = *reinterpret_cast<uint*>(&pNewBitmap[dest/8]);
      // dprintf("destchunk pulled from memory: %X\n", destchunk);
            
      // modify the right bits only, leave alone the rest
      destchunk = destchunk | srcchunk;
      *reinterpret_cast<uint*>(&pNewBitmap[dest/8]) = destchunk;
      //dprintf("destchunk after OR: %X\n", destchunk);
      //dprintf("destchunk goes to: %i\n", dest);
    }
  }
 
  // set new geometry
  mactualWidth = newWidth;
  mactualHeight = newHeight;
  calcRenderedDims();
  //pbmInfo->bmiHeader.biWidth = actualWidth;
  //pbmInfo->bmiHeader.biHeight = (upsideDown) ? actualHeight : -(int) actualHeight;
  //pbmInfo->bmiHeader.biSizeImage = bitmapSize;

  // free old memory
  delete[] mpBitmap;
  mpBitmap = pNewBitmap;
  
  /*
  dprintf("Destination bitmap:\n");
  for (uint z = 0; z < bitmapSize; z++)
  {
    dprintf("%X", (uint)pBitmap[z]);
    //if (z > 0 && z % 10 == 0) dprintf("\n");
  }
  dprintf("\n"); */

  return true;
}


// For an image that is right side up, this algorithm rotates the image counter-clockwise
// For an image that is upside down, this algorithm rotates the image clockwise
bool Image::rotateAlgorithm2()
{
  merrMsg.str("");
  
  int newWidth = mactualHeight;
  int newHeight = mactualWidth;

  int srcUnpaddedScanlineBits = mactualWidth * mbitCount;
  int srcPaddedScanlineBits = srcUnpaddedScanlineBits;
  while (srcPaddedScanlineBits & (sizeof(uint)*8-1)) srcPaddedScanlineBits++;
  
  int destUnpaddedScanlineBits = newWidth * mbitCount;
  int destPaddedScanlineBits = destUnpaddedScanlineBits;
  while (destPaddedScanlineBits & (sizeof(uint)*8-1)) destPaddedScanlineBits++;

  mbitmapSize = int(ceil(double(destPaddedScanlineBits)/8)) * newHeight;

  BYTE* pNewBitmap = new(std::nothrow) BYTE[mbitmapSize+sizeof(uint)];
  if (pNewBitmap == NULL) 
  {
    merrMsg << "Insufficient memory to rotate '" << mfileName << "'";
    return false;
  }
  memset(pNewBitmap, 0, mbitmapSize);

  uint src, dest; // these two values are the offset in bits
  uint x, y; // points on the source image

  for (y = 0; y < mactualHeight; y++) 
  {
    src = y * srcPaddedScanlineBits;
    for (x = 0; x < mactualWidth; x++, src += mbitCount) 
    {
      // read a small chunk from the image bit array
      uint srcchunk = *reinterpret_cast<uint*>(&mpBitmap[src/8]);
      // if the bit depth of image is less than 8, the bits we are trying to access
      // might be inside, or left of the srcchunk bit array
      // also note the order of the image for bit depths less than 8 is reversed
      // in other words, for a 4 bit image the first pixel is in the left side of
      // the byte, not the right side where you would expect it
      if (mbitCount < 8)
        srcchunk = srcchunk >> ((8 - mbitCount) - (src % 8));
    
      // create a mask that will turn all the unwanted bits to zeros
      uint mask = ~(-1 << mbitCount);
      srcchunk = srcchunk & mask;

      // calculate where the source pixel goes
      dest = (mactualWidth-x-1) * destPaddedScanlineBits;
      dest += (y*mbitCount);
      
      // remember that the order of the image for bit depths less than 8 is reversed
      if (mbitCount < 8)
        srcchunk = srcchunk << ((8 - mbitCount) - (dest % 8));
      // we need to read a chunk from the destination array in case
      // the bit depth isn't the sizeof(uint)
      uint destchunk = *reinterpret_cast<uint*>(&pNewBitmap[dest/8]);
      // modify the right bits only, leave alone the rest
      destchunk = destchunk | srcchunk;
      *reinterpret_cast<uint*>(&pNewBitmap[dest/8]) = destchunk;
    }
  }
 
  // set new geometry
  mactualWidth = newWidth;
  mactualHeight = newHeight;
  calcRenderedDims();
  //pbmInfo->bmiHeader.biWidth = actualWidth;
  //pbmInfo->bmiHeader.biHeight = (upsideDown) ? actualHeight : -(int) actualHeight;
  //pbmInfo->bmiHeader.biSizeImage = bitmapSize;
 
  // free old memory
  delete[] mpBitmap;
  mpBitmap = pNewBitmap;
  return true;
}


bool Image::setZoomPercentage(double value)
{
  if (value <= 0 || value > 1000)
  {
    merrMsg.str("The custom percentage must be a value greater than zero but less than or equal to 1000.");
    return false;
  }
  else
  { 
    merrMsg.str("");
    mzoomPercentage = value;
    calcRenderedDims();
    return true;
  }
}


void Image::calcRenderedDims()
{
  mrenderedWidth = mactualWidth * mzoomPercentage / 100;
  mrenderedHeight = mactualHeight * mzoomPercentage / 100;
}


void Image::resetRenderedDims()
{
  mrenderedWidth = mactualWidth;
  mrenderedHeight = mactualHeight;
}


bool Image::writeBmp(char *fileName)
{
  std::ofstream outfile;

  merrMsg.str("");
  
  if (mValidObject == false)
    return false;
  
  try {
/*
        BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfType = WORD(('M' << 8) | 'B');
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + bitmapSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
    
    outfile.open(fileName, std::ios::binary);
    if(outfile.fail())
    {
      errMsg << "Cannot open file '" << fileName << "'";
      throw std::ofstream::failure(errMsg.str());
    }
    outfile.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
    outfile.write((char*)pbmInfo, sizeof(BITMAPINFO));
    outfile.write((char*)pBitmap, bitmapSize);
    outfile.close();
        */
  }
  catch (std::ofstream::failure &e) 
  {
/*
        outfile.close();
    return false;
        */
  }
  return true;
}

void RGBALineToRGB(BYTE *pRGBA, int RGBALineSize, BYTE *pRGB, int RGBLineSize, BYTE RGBBackground[3])
{
  //int foreground[4];  /* image pixel: R, G, B, A */
  //int background[3];  /* background pixel: R, G, B */
  //int fbpix[3];       /* frame buffer pixel */
  //int 255;   /* foreground max sample */
  //int 255;   /* background max sample */
  //int 255;   /* frame buffer max sample */
  int src, dest;
  int ialpha;
  float alpha, compalpha;
  float gamfg, gambg, comppix;
   
  /* Get max sample values in data and frame buffer */
  //255 = (1 << fg_sample_depth) - 1;
  //255 = (1 << bg_sample_depth) - 1;
  //255 = (1 << frame_buffer_sample_depth) - 1;
    
  for (src=0,dest=0; src < RGBALineSize; src+=4,dest+=3) {
    /*
       * Get integer version of alpha.
       * Check for opaque and transparent special cases;
     * no compositing needed if so.
     *
     * We show the whole gamma decode/correct process in
     * floating point, but it would more likely be done
     * with lookup tables.
     */
    ialpha = pRGBA[src+3];
   
    if (ialpha == 0) {
      // Foreground image is transparent here, copy background
      pRGB[dest] = RGBBackground[2];
      pRGB[dest+1] = RGBBackground[1];
      pRGB[dest+2] = RGBBackground[0];
    } else if (ialpha == 255) {
      // Opaque, copy foreground
      pRGB[dest] = pRGBA[src];
      pRGB[dest+1] = pRGBA[src+1];
      pRGB[dest+2] = pRGBA[src+2];
      /*
      for (i = 0; i < 3; i++) {
        gamfg = (float) foreground[i] / 255;
        linfg = pow(gamfg, 1.0 / fg_gamma);
        comppix = linfg;
        gcvideo = pow(comppix, 1.0 / display_exponent);
        fbpix[i] = (int) (gcvideo * 255 + 0.5);
      }*/
    } else {
      /*
       * Compositing is necessary.
       * Get floating-point alpha and its complement.
       * Note: alpha is always linear; gamma does not
       * affect it.
       */
      alpha = (float) ialpha / 255;
      compalpha = 1.0 - alpha;

      for (int i = 0; i < 3; i++) {
        /*
         * Convert foreground and background to floating
         * point, then undo gamma encoding.
         */
        gamfg = (float) pRGBA[src+i] / 255;
        //linfg = pow(gamfg, 1.0 / fg_gamma);
        gambg = (float) RGBBackground[2-i] / 255;
        //linbg = pow(gambg, 1.0 / bg_gamma);
        /* 
         * Composite.
         */
        comppix = gamfg * alpha + gambg * compalpha;
        /*
         * Gamma correct for display.
         * Convert to integer frame buffer pixel.
         */
        pRGB[dest+i] = BYTE(comppix * 255 + 0.5);
      }
    }
  }
  while (dest < RGBLineSize) {
    pRGB[dest] = 0;
    dest++;
  }
}

//
// Currently these algorithms are not used
//
bool Image::rotate24bitCW()
{
  if (mbitCount != 24) 
  {
    merrMsg.str("Rotation currently only works with images that have a bit depth of 24");
    return false;
  }
 
  uint newWidth = mactualHeight;
  uint newHeight = mactualWidth;

  uint srcPaddedScanlineBytes = mactualWidth * 3;
  while ((srcPaddedScanlineBytes & 3) != 0) srcPaddedScanlineBytes++;
  uint destUnpaddedScanlineBytes = newWidth * 3;
  uint destPaddedScanlineBytes = destUnpaddedScanlineBytes;
  while ((destPaddedScanlineBytes & 3) != 0) destPaddedScanlineBytes++;
  mbitmapSize = destPaddedScanlineBytes * newHeight;

  BYTE* pNewBitmap = new(std::nothrow) BYTE[mbitmapSize];
  if (pNewBitmap == NULL) 
  {
    merrMsg << "Insufficient memory to rotate '" << mfileName << "'";
    return false;
  }
  memset(pNewBitmap, 0, mbitmapSize);

  uint src, dest;
  uint x, y;

  for (y = 0; y < mactualHeight; y++) 
  {
    src = y*srcPaddedScanlineBytes;
    for (x = 0; x < mactualWidth; x++,src+=3) 
    {
      dest = (x*destPaddedScanlineBytes)+destUnpaddedScanlineBytes-3;
      dest -= (y*3);
      pNewBitmap[dest] = mpBitmap[src];
      pNewBitmap[dest+1] = mpBitmap[src+1];
      pNewBitmap[dest+2] = mpBitmap[src+2];
    }
  }
 
  // set new geometry
  mactualWidth = newWidth;
  mactualHeight = newHeight;
  calcRenderedDims();
//  pbmInfo->bmiHeader.biWidth = actualWidth;
//  pbmInfo->bmiHeader.biHeight = (upsideDown) ? actualHeight : -(int) actualHeight;
//  pbmInfo->bmiHeader.biSizeImage = bitmapSize;
 
  // free allocated memory
  delete[] mpBitmap;
  mpBitmap = pNewBitmap;
  return true;
}


bool Image::rotate24bitCCW()
{
  if (mbitCount != 24) 
  {
    merrMsg.str("Rotation currently only works with images that have a bit depth of 24");
    return false;
  }
 
  uint newWidth = mactualHeight;
  uint newHeight = mactualWidth;
  
  uint srcPaddedScanlineBytes = mactualWidth * 3;
  while ((srcPaddedScanlineBytes & 3) != 0) srcPaddedScanlineBytes++;
  uint destUnpaddedScanlineBytes = newWidth * 3;
  uint destPaddedScanlineBytes = destUnpaddedScanlineBytes;
  while ((destPaddedScanlineBytes & 3) != 0) destPaddedScanlineBytes++;
  mbitmapSize = destPaddedScanlineBytes * newHeight;

  BYTE* pNewBitmap = new(std::nothrow) BYTE[mbitmapSize];
  if (pNewBitmap == NULL) 
  {
    merrMsg << "Insufficient memory to rotate '" << mfileName << "'";
    return false;
  }
  memset(pNewBitmap, 0, mbitmapSize);

  uint src, dest;
  uint x, y;

  for (y = 0; y < mactualHeight; y++) 
  {
    src = y*srcPaddedScanlineBytes;
    for (x = 0; x < mactualWidth; x++,src+=3) 
    {
      dest = (mactualWidth-x-1)*destPaddedScanlineBytes;
      dest += (y*3);
      pNewBitmap[dest] = mpBitmap[src];
      pNewBitmap[dest+1] = mpBitmap[src+1];
      pNewBitmap[dest+2] = mpBitmap[src+2];
    }
  }
 
  // set new geometry
  mactualWidth = newWidth;
  mactualHeight = newHeight;
  calcRenderedDims();
//  pbmInfo->bmiHeader.biWidth = actualWidth;
//  pbmInfo->bmiHeader.biHeight = (upsideDown) ? actualHeight : -(int) actualHeight;
//  pbmInfo->bmiHeader.biSizeImage = bitmapSize;
 
  // free allocated memory
  delete[] mpBitmap;
  mpBitmap = pNewBitmap;
  return true;
}


void Image::alignBytes()
{
  uint unpaddedWidth = mreadWidth * 3;
  uint paddedWidth = unpaddedWidth;
  uint diff=0;
  BYTE* pOldBitmap=mpBitmap;
  if (mpBitmap != 0)
  {
    while ((paddedWidth & 3) != 0) paddedWidth++;
    diff = paddedWidth - unpaddedWidth;
    if (diff > 0)
    {
      BYTE* pBitmap2 = new BYTE[paddedWidth * mreadHeight];
      for (int row=0; row<mreadHeight; row++)
      {
          memcpy(&pBitmap2[row*paddedWidth], &mpBitmap[row*unpaddedWidth], unpaddedWidth);
          memset(&pBitmap2[(row+1)*paddedWidth-diff], 0, diff);
      }
      mpBitmap = pBitmap2;
      delete[] pOldBitmap;
    }
    /*
    info.resize(sizeof(BITMAPINFO));
    pbmInfo = reinterpret_cast<BITMAPINFO*>(&info[0]);
    pbmInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmInfo->bmiHeader.biWidth = readWidth;
    pbmInfo->bmiHeader.biHeight = -(int)readHeight;
    pbmInfo->bmiHeader.biPlanes = 1;
    pbmInfo->bmiHeader.biBitCount = 24;
    pbmInfo->bmiHeader.biCompression = BI_RGB;
    pbmInfo->bmiHeader.biSizeImage = paddedWidth * readHeight;
    pbmInfo->bmiHeader.biClrImportant = 0;
    pbmInfo->bmiHeader.biXPelsPerMeter = 0;
    pbmInfo->bmiHeader.biYPelsPerMeter = 0;
    */
  }
}

