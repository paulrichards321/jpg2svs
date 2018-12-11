/**************************************************************************
Initial author: Paul F. Richards (paulrichards321@gmail.com) 2016-2017 
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

#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cctype>
#if defined(_WIN32) || defined(_WIN64)
#include "console-mswin.h"
#else
#include "console-unix.h"
#endif
#include "tiffsupport.h"
#include "composite.h"

void fillTile(BYTE *pDest, BYTE *pSrc, int tileWidth, int tileHeight, int subTileWidth, int subTileHeight, BYTE bkgColor);
void simpleFillTile(BYTE *pDest, BYTE *pSrc, int tileWidth, int tileHeight, BYTE bkgColor);


class SlideConvertor
{
protected:
  CompositeSlide *slide;
  Tiff *tif;
  std::ofstream *logFile;
  std::string errMsg;
  int mBaseTotalWidth, mBaseTotalHeight;
  bool mValidObject;
  bool mIncludeZStack;
  int mQuality;
  int mStep, mZSteps;
  int mLastZLevel, mLastDirection;
public:
  SlideConvertor();
  ~SlideConvertor() { closeRelated(); }
  void closeRelated();
  std::string getErrMsg() { return errMsg; }
  int open(std::string inputFile, std::string outputFile, bool markOutline, bool includeZStack, int quality, int bestXOffset = -1, int bestYOffset = -1);
  int convert();
  int outputLevel(int level, bool tiled, int direction, int zLevel, int magnification);
};


SlideConvertor::SlideConvertor()
{
  slide = 0;
  tif = 0;
  logFile = 0;
  mValidObject = false;
  mStep=0;
  mLastZLevel=-1;
  mLastDirection=-1;
  mZSteps=0;
  mBaseTotalWidth=0;
  mBaseTotalHeight=0;
  mIncludeZStack=true;
  mQuality=90;
}


int SlideConvertor::outputLevel(int level, bool tiled, int direction, int zLevel, int magnification)
{
  int srcTotalWidth=0;
  int srcTotalHeight=0;
  int srcTotalWidthL2=1;
  int srcTotalHeightL2=1;
  int destTotalWidth=0;
  int destTotalHeight=0;
  int tileWidth=256;
  int tileHeight=256;
  double xScale=0.0, yScale=0.0;
  double xScaleL2=0.0, yScaleL2=0.0;
  double xScaleReverse=0.0, yScaleReverse=0.0;
  double xScaleReverseL2=0.0, yScaleReverseL2=0.0;
  int grabWidth=0, grabHeight=0;
  int grabWidthL2=0, grabHeightL2=0;
  BYTE* pSizedBitmap = 0;
  std::ostringstream output;
  int readZLevel = 0;
  int readDirection = 0;
  unsigned char bkgColor=255;
  bool fillin = ((tiled == true && level < 2 && slide->checkLevel(2)) ? true : false);

  if (level==0) 
  {
    readZLevel = zLevel;
    readDirection = direction;
  }
  if (mValidObject == false || 
      slide->checkZLevel(level, readDirection, readZLevel)==false)
  {
    return 0;
  }
  if (mLastZLevel != zLevel || mLastDirection != direction)
  {
    mZSteps++;
    output << std::endl;
    output << "Z Level=" << direction << "," << zLevel << std::endl;
  }
  mLastZLevel = zLevel;
  mLastDirection = direction;
  output << "Pyramid Level=" << level << " Magnification=" << magnification << std::endl;
  *logFile << output.str();
  std::cout << output.str();
  srcTotalWidth = slide->getActualWidth(level);
  srcTotalHeight = slide->getActualHeight(level);
  if (fillin)
  {
    srcTotalWidthL2 = slide->getActualWidth(2);
    srcTotalHeightL2 = slide->getActualHeight(2);
  }
  if (tiled==false)
  {
    tileWidth=0;
    tileHeight=0;
    double magnifyX=magnification;
    double magnifyY=magnification;
    double destTotalWidthDec = mBaseTotalWidth / magnifyX;
    double destTotalHeightDec = mBaseTotalHeight / magnifyY;
    destTotalWidth = (int) destTotalWidthDec;
    destTotalHeight = (int) destTotalHeightDec;
    xScale=(double) srcTotalWidth / (double) destTotalWidth;
    yScale=(double) srcTotalHeight / (double) destTotalHeight;
    xScaleReverse=(double) destTotalWidth / (double) srcTotalWidth;
    yScaleReverse=(double) destTotalHeight / (double) srcTotalHeight;
    grabWidth=(int) srcTotalWidth;
    grabHeight=(int) srcTotalHeight;
  } 
  else if (magnification==1 || slide->isPreviewSlide(level))
  {
    destTotalWidth = srcTotalWidth;
    destTotalHeight = srcTotalHeight;
    xScale=1.0;
    yScale=1.0;
    xScaleReverse=1.0;
    yScaleReverse=1.0;
    grabWidth=256;
    grabHeight=256;

    xScaleL2=(double) srcTotalWidthL2 / (double) destTotalWidth;
    yScaleL2=(double) srcTotalHeightL2 / (double) destTotalHeight;
    xScaleReverseL2=(double) destTotalWidth / (double) srcTotalWidthL2;
    yScaleReverseL2=(double) destTotalHeight / (double) srcTotalHeightL2;
    grabWidthL2=(int)ceil(256.0 * xScaleL2);
    grabHeightL2=(int)ceil(256.0 * yScaleL2);
  }
  else
  {
    double magnifyX=magnification;
    double magnifyY=magnification;
    double destTotalWidthDec = mBaseTotalWidth / magnifyX;
    double destTotalHeightDec = mBaseTotalHeight / magnifyY;
    destTotalWidth = (int) destTotalWidthDec;
    destTotalHeight = (int) destTotalHeightDec;
    xScale=(double) srcTotalWidth / (double) destTotalWidth;
    yScale=(double) srcTotalHeight / (double) destTotalHeight;
    xScaleReverse=(double) destTotalWidth / (double) srcTotalWidth;
    yScaleReverse=(double) destTotalHeight / (double) srcTotalHeight;
    grabWidth=(int)ceil(256.0 * xScale);
    grabHeight=(int)ceil(256.0 * yScale);

    xScaleL2=((double) srcTotalWidthL2 / (double) destTotalWidth) / ((double) srcTotalWidth / (double) destTotalWidth);
    yScaleL2=((double) srcTotalHeightL2 / (double) destTotalHeight) / ((double) srcTotalHeight / (double) destTotalHeight);
    xScaleReverseL2=((double) destTotalWidth / (double) srcTotalWidthL2) / 2;
    yScaleReverseL2=((double) destTotalHeight / (double) srcTotalHeightL2) / 2;
 
    grabWidthL2=(int)ceil(256.0 * (double) srcTotalWidthL2 / (double) destTotalWidth);
    grabHeightL2=(int)ceil(256.0 * (double) srcTotalHeightL2 / (double) destTotalHeight);
  }
  *logFile << " xScale=" << xScale << " yScale=" << yScale;
  *logFile << " srcTotalWidth=" << srcTotalWidth << " srcTotalHeight=" << srcTotalHeight;
  *logFile << " destTotalWidth=" << destTotalWidth << " destTotalHeight=" << destTotalHeight;
  *logFile << std::endl;
  int quality=slide->getQuality(level);
  if (quality<mQuality || quality<=0)
  {
    quality=mQuality;
  }
  int totalMag=slide->getMagnification();
  if (totalMag<=0)
  {
    totalMag=40;
  }
  std::ostringstream oss;
  oss << "Aperio Image|AppMag=" << totalMag;
  if (slide->getTotalZLevels() > 1 && mZSteps == 1) 
  {
    oss << "|TotalDepth = " << slide->getTotalZLevels() << "\0";
  }
  else if (slide->getTotalZLevels() > 1 && mZSteps > 1)
  {
    oss << "|OffsetZ = " << (mZSteps-1) << "\0";
  }
  std::string strAttributes=oss.str();
  if (tif->setAttributes(3, 8, destTotalWidth, destTotalHeight, tileWidth, tileHeight, 1, quality)==false || tif->setDescription(strAttributes, mBaseTotalWidth, mBaseTotalHeight)==false)
  {
    std::string errMsg;
    tif->getErrMsg(errMsg);
    std::cerr << "Failed to write tif attributes: " << errMsg << std::endl; 
    return 4;
  }
  int yDest=0, xDest=0;
  int perc=0, percOld=0;
  bool onePercHit=false;
  time_t timeStart=0, timeLast=0;
  bool error=false;
  timeStart = time(NULL);
  int ySrc=0;
  retractCursor();
  std::cout << "0% done...    " << std::flush;

  BYTE *pBitmapL2 = NULL;
  if (fillin)
  {
    pBitmapL2 = new BYTE[srcTotalWidthL2 * srcTotalHeightL2 * 3];
    if (pBitmapL2==NULL)
    {
      *logFile << "Failed to allocate memory for full pyramid level 2. Out of memory?" << std::endl;
    }
  }
  int readWidthL2=0;
  int readHeightL2=0;
  bool readOkL2=false;
  if (pBitmapL2)
  {
    readOkL2=slide->read(pBitmapL2, 2, 0, 0, 0, 0, srcTotalWidthL2, srcTotalHeightL2, false, &readWidthL2, &readHeightL2);
    if (readOkL2==false)
    {
      *logFile << "Failed to read full pyramid level 2." << std::endl;
    }
  }
  while (ySrc<srcTotalHeight && yDest<destTotalHeight && error==false)
  {
    xDest = 0;
    for (int xSrc=0; xSrc<srcTotalWidth && xDest<destTotalWidth && error==false; xSrc += grabWidth) 
    {
      bool toSmall = false;
      //*logFile << " slide->read(x=" << xSrc << " y=" << ySrc << " grabWidth=" << grabWidth << " grabHeight=" << grabHeight << " level=" << i << "); " << std::endl;
      BYTE *pBitmap1 = slide->allocate(level, xSrc, ySrc, grabWidth, grabHeight, false);
      BYTE *pBitmap2 = pBitmap1;
      int readWidth=0, readHeight=0;
      bool readOk=false;
      if (pBitmap1)
      {
        readOk=slide->read(pBitmap2, level, readDirection, readZLevel, xSrc, ySrc, grabWidth, grabHeight, false, &readWidth, &readHeight);
      }
      if (readOk)
      {
        cv::Mat imgScaled;
        cv::Mat imgScaled2;
        BYTE *pBitmap3 = NULL;
        if (readWidth != grabWidth || readHeight != grabHeight)
        {
        //    *logFile << "Tile shorter: width=" << readWidth << " height=" << readHeight << std::endl;
          toSmall = true;
          pSizedBitmap = new BYTE[grabWidth*grabHeight*3];
          memset(pSizedBitmap, bkgColor, grabHeight*grabWidth*3);
          for (int row=0; row < readHeight; row++)
          {
            memcpy(&pSizedBitmap[row*grabWidth*3], &pBitmap2[row*readWidth*3], readWidth*3);
          }
          pBitmap2 = pSizedBitmap;
        }
        if (tiled==false)
        {
          cv::Mat imgSrc(grabHeight, grabWidth, CV_8UC3, pBitmap2);
          cv::Size scaledSize(destTotalWidth, destTotalHeight);
          cv::resize(imgSrc, imgScaled, scaledSize, xScaleReverse, yScaleReverse);
          imgSrc.release();
          pBitmap2 = imgScaled.data;  
        } 
        else if (grabWidth!=256 || grabHeight!=256)
        {
          cv::Mat imgSrc(grabHeight, grabWidth, CV_8UC3, pBitmap2);
          cv::Size scaledSize(256, 256);
          cv::resize(imgSrc, imgScaled, scaledSize, xScaleReverse, yScaleReverse);
          imgSrc.release();
          pBitmap2 = imgScaled.data;  
          /*
          srcPixBuf = gdk_pixbuf_new_from_data((guchar*) pBitmap2, GDK_COLORSPACE_RGB, FALSE, 8, grabWidth, grabHeight, grabWidth*3, NULL, NULL);
          destPixBuf = gdk_pixbuf_scale_simple(srcPixBuf, 256, 256, GDK_INTERP_BILINEAR);
          if (destPixBuf != 0)
          {
            pBitmap2 = (BYTE*) gdk_pixbuf_read_pixels(destPixBuf);
            scaled = true;
          }
          else if (srcPixBuf != 0)
          {
            g_object_unref(srcPixBuf);
          }
          */
        }
        if (readOkL2)
        {
          int xSrcStartL2=ceil((double) xSrc * xScaleL2);
          int ySrcStartL2=ceil((double) ySrc * yScaleL2);
          int xSrcEndL2=xSrcStartL2 + grabWidthL2;
          int ySrcEndL2=ySrcStartL2 + grabHeightL2;
          if (xSrcEndL2 > srcTotalWidthL2) xSrcEndL2=srcTotalWidthL2;
          if (ySrcEndL2 > srcTotalHeightL2) ySrcEndL2=srcTotalHeightL2;
          int readWidthL2=xSrcEndL2 - xSrcStartL2;
          int readHeightL2=ySrcEndL2 - ySrcStartL2;
          if (xSrcStartL2 >= 0 && xSrcStartL2 < srcTotalWidthL2 && ySrcStartL2 >= 0 && ySrcStartL2 < srcTotalHeightL2)
          {
            pBitmap3 = new BYTE[readHeightL2 * readWidthL2 * 3];
            memset(pBitmap3, bkgColor, readHeightL2*readWidthL2*3);
            for (int row=0; row < readHeightL2; row++)
            {
              memcpy(&pBitmap3[row*readWidthL2*3], &pBitmapL2[((ySrcStartL2+row)*srcTotalWidthL2*3)+(xSrcStartL2*3)], readWidthL2*3);
            }
            cv::Mat imgSrc(readHeightL2, readWidthL2, CV_8UC3, pBitmap3);
            cv::Size scaledSize(256, 256);
            cv::resize(imgSrc, imgScaled2, scaledSize, xScaleReverseL2, yScaleReverseL2);
            imgSrc.release();
            fillTile(pBitmap2, imgScaled2.data, 256, 256, 8, 8, bkgColor);
            //simpleFillTile(pBitmap2, imgScaled2.data, 256, 256, bkgColor);
          } 
        } 
        bool writeOk=false;
        if (tiled)
          writeOk=tif->writeEncodedTile(pBitmap2, xDest, yDest, 1);
        else
          writeOk=tif->writeImage(pBitmap2);
        imgScaled.release();
        if (readOkL2)
        {
          imgScaled2.release();
        }
        if (pBitmap3)
        {
          delete[] pBitmap3;
        }
        if (toSmall && pSizedBitmap)
        {
          delete[] pSizedBitmap;
        }
        if (writeOk==false)
        {
          std::string errMsg;
          tif->getErrMsg(errMsg);
          std::cerr << "Failed to write tile: " << errMsg << std::endl;
          error = true;
        }
      }
      else
      {
        std::cerr << "Failed to read jpeg tile. " << std::endl;
        //error = true;
      }
      if (pBitmap1)
      {
        delete[] pBitmap1;
      }
      if (tiled) 
        xDest += tileWidth;
      else
        xDest = destTotalWidth;
    }
    if (tiled)
      yDest += tileHeight;
    else
      yDest = destTotalHeight;
    ySrc += grabHeight;
    perc=(int)(((double) ySrc / (double) srcTotalHeight) * 100);
    if (perc>100)
    {
      perc=100;
    }
    if (perc>percOld)
    {
      percOld=perc;
      retractCursor();
      std::cout << perc << "% done...    " << std::flush;
    }
    else if (onePercHit==false)
    {
      retractCursor();
      std::cout << perc << "% done...    " << std::flush;
      onePercHit = true;
    }
  }  
  if (error==false && tif->writeDirectory()==false)
  {
    std::string errMsg;
    tif->getErrMsg(errMsg);
    error = true;
    std::cerr << "Failed to write tif directory: " << errMsg << std::endl;
  }
  /*
  if (pBitmap3)
  {
    delete[] pBitmap3;
    pBitmap3 = NULL;
  }
  */
  if (pBitmapL2)
  {
    delete[] pBitmapL2;
    pBitmapL2 = NULL;
  }
  timeLast = time(NULL);
  if (error==false)
  {
    std::cout << "Took " << timeLast - timeStart << " seconds for this level." << std::endl;
  }
  mStep++;
  return (error==true ? 1 : 0); 
}


void simpleFillTile(BYTE *pDest, BYTE *pSrc, int tileWidth, int tileHeight, BYTE bkgColor)
{
  int offset=tileHeight*tileWidth*3;
  BYTE *pSrcEnd=pSrc+offset;
  while (pSrc < pSrcEnd) 
  {
    if (pDest[0] == bkgColor && pDest[1] == bkgColor && pDest[2] == bkgColor)
    {
      pDest[0] = pSrc[0];
      pDest[1] = pSrc[1];
      pDest[2] = pSrc[2];
    }
    pSrc += 3;
    pDest += 3;
  }
}


void fillTile(BYTE *pDest, BYTE *pSrc, int tileWidth, int tileHeight, int subTileWidth, int subTileHeight, BYTE bkgColor)
{
  int rowSize=tileWidth * 3;
  int subRowSize = subTileWidth * 3;
  for (int y1 = 0; y1 < tileHeight; y1 += subTileHeight)
  {
    for (int x1 = 0; x1 < rowSize; x1 += subRowSize)
    {
      int y3=y1+subTileHeight;
      bool set_bkg = true;
      for (int y2=y1; y2 < y3 && set_bkg; y2++)
      {
        BYTE *pDest2=&pDest[(y2*rowSize)+x1];
        BYTE *pDestEnd=pDest2+subRowSize;
        while (pDest2 < pDestEnd)
        {
          if (pDest2[0] != bkgColor || pDest2[1] != bkgColor || pDest2[2] != bkgColor)
          {
            set_bkg = false;
            break;
          }
          pDest2 += 3;
        }
      }
      if (set_bkg)
      {
        for (int y2=y1; y2 < y3; y2++)
        {
          int offset=(y2*rowSize)+x1;
          BYTE *pSrc2=pSrc+offset;
          BYTE *pSrcEnd=pSrc2+subRowSize;
          BYTE *pDest2=pDest+offset;
          while (pSrc2 < pSrcEnd) 
          {
            pDest2[0] = pSrc2[0];
            pDest2[1] = pSrc2[1];
            pDest2[2] = pSrc2[2];
            pSrc2 += 3;
            pDest2 += 3;
          }
        }
      }
    }
  }
}


int SlideConvertor::convert()
{
  int smallLevels[] =          {  0,  2,  1,   1,   2 };
  int smallMagnifyLevels[] =   {  1,  64,  4,  16,  32 };
  int bigLevels[] =            {  0,  3,  1,   1,   2,   2 };
  int bigMagnifyLevels[] =     {  1,  1,  4,  16,  64, 128 };
  
  int smallZLevels1[] =        {  0,  2,  0,   1,   2 };
  int smallZMagnifyLevels1[] = {  1,  64,  4,  16,  32 };
  int smallZLevels2[] =        {  0,  0,  1,   2 };
  int smallZMagnifyLevels2[] = {  1,  4, 16,  32 };

  int bigZLevels1[] =          {  0,  2,  0,   1,   2,   2 };
  int bigZMagnifyLevels1[] =   {  1,  256,  4,  16,  64, 128 };
  int bigZLevels2[] =          {  0,  0,  1,   2,   2 };
  int bigZMagnifyLevels2[] =   {  1,  4, 16,  64, 128 };

  int *levels1, *levels2, *magnifyLevels1, *magnifyLevels2;
  int totalLevels1, totalLevels2;
  
  int error = 0;
  bool tiled = true;

  if (mValidObject == false) return 1;
 
  if (slide->getTotalZLevels()==0 || mIncludeZStack==false)
  {
    if (mBaseTotalWidth >= 140000 || mBaseTotalHeight >= 140000)
    {
      totalLevels1 = 6;
      levels1 = bigLevels;
      magnifyLevels1 = bigMagnifyLevels;
    }
    else
    {
      totalLevels1 = 5;
      levels1 = smallLevels;
      magnifyLevels1 = smallMagnifyLevels;
    }
    //****************************************************************
    // Output the base level, thumbnail, 4x, 16x, and 32x levels 
    //****************************************************************
    for (int step = 0; step < totalLevels1 && error==0; step++)
    {
      tiled = (step == 1 ? false : true);
      error=outputLevel(levels1[step], tiled, 0, 0, magnifyLevels1[step]);
    }
  }
  //****************************************************************
  // Check if slide has more than one Z-Level 
  //****************************************************************
  else if (slide->getTotalZLevels() > 0)
  {
    if (mBaseTotalWidth >= 140000 || mBaseTotalHeight >= 140000)
    {
      totalLevels1 = 6;
      totalLevels2 = 5;
      levels1 = bigZLevels1;
      levels2 = bigZLevels2;
      magnifyLevels1 = bigZMagnifyLevels1;
      magnifyLevels2 = bigZMagnifyLevels2;
    }
    else
    {
      totalLevels1 = 5;
      totalLevels2 = 4;
      levels1 = smallZLevels1;
      levels2 = smallZLevels2;
      magnifyLevels1 = smallZMagnifyLevels1;
      magnifyLevels2 = smallZMagnifyLevels2;
    }
 
    int totalBottomZLevels = slide->getTotalBottomZLevels();
    int totalTopZLevels = slide->getTotalTopZLevels();
    int firstDirection;
    int bottomLevel;
    if (totalBottomZLevels == 0)
    {
      firstDirection=0; // no bottom levels do the base level first
      bottomLevel=0;
    }
    else
    {
      firstDirection=1; // do the first bottom Z-level first if it exists

      bottomLevel=totalBottomZLevels-1;
    }
    //****************************************************************
    // Output the first bottom Z-level plus the slide thumbnail
    //****************************************************************
    for (int step=0; step < totalLevels1 && error==0; step++)
    {
      tiled = (step == 1 ? false : true);
      error=outputLevel(levels1[step], tiled, firstDirection, bottomLevel, magnifyLevels1[step]);
    }
    //****************************************************************
    // Output the next three bottom levels
    //****************************************************************
    for (int zLevel = bottomLevel-1; zLevel >= 0; zLevel--)
    {
      for (int step=0; step < totalLevels2 && error==0; step++)
      {
        tiled = (step == 1 ? false : true);
        error=outputLevel(levels2[step], tiled, 1, zLevel, magnifyLevels2[step]);
      }
    }
    //****************************************************************
    // Output the base (middle) level
    //****************************************************************
    for (int step=0; step < totalLevels2 && error==0 && firstDirection != 0; step++)
    {
      tiled = (step == 1 ? false : true);
      error=outputLevel(levels2[step], tiled, 0, 0, magnifyLevels2[step]);
    }
    //****************************************************************
    // Output the last four upper levels
    //****************************************************************
    for (int zLevel = 0; zLevel < totalTopZLevels; zLevel++)
    {
      for (int step=0; step < totalLevels2 && error==0; step++)
      {
        tiled = (step == 1 ? false : true);
        error=outputLevel(levels2[step], tiled, 2, zLevel, magnifyLevels2[step]);
      }
    }
  }
  std::cout << std::endl << "All Levels Completed." << std::endl;
  return error;
}


int SlideConvertor::open(std::string inputFile, std::string outputFile, bool markOutline, bool includeZStack, int quality, int bestXOffset, int bestYOffset)
{
  closeRelated();
  logFile = new std::ofstream("jpg2svs.log");
  slide = new CompositeSlide();
  errMsg="";
  if (slide->open(inputFile.c_str(), false, markOutline, bestXOffset, bestYOffset)==false)
  {
    return 1;
  }
  tif = new Tiff();
  if (tif->createFile(outputFile)==false)
  {
    tif->getErrMsg(errMsg);
    return 2;
  }
  for (int level=0; level<4; level++)
  {
    if (slide->checkLevel(level))
    {
      mBaseTotalWidth = slide->getActualWidth(level);
      mBaseTotalHeight = slide->getActualHeight(level);
      break;
    }
  }
  mStep=0;
  mLastZLevel=-1;
  mIncludeZStack = includeZStack;
  mQuality = quality;
  if (mBaseTotalWidth > 0 && mBaseTotalHeight > 0)
  {
    mValidObject=true;
    return 0;
  }
  mValidObject=false;
  return 3;
}


void SlideConvertor::closeRelated()
{
  if (logFile)
  {
    logFile->close();
    delete logFile;
    logFile = NULL;
  }
  if (slide)
  {
    slide->close();
    delete slide;
    slide = NULL;
  }
  if (tif)
  {
    tif->close();
    delete tif;
    tif = NULL;
  }
  mStep=0;
  mZSteps=0;
  mLastDirection=-1;
  mLastZLevel=-1;
  mValidObject=false;
  mBaseTotalWidth=0;
  mBaseTotalHeight=0;
}


int main(int argc, char** argv)
{
  SlideConvertor slideConv;
  int error=0;
  char *infile = 0, *outfile = 0;
  int bestXOffset = -1, bestYOffset = -1;
  bool doBorderHighlight = true;
  bool includeZStack = false;
  int quality = 90;
  char syntax[] = "syntax: jpg2svs -h[0,1] -x[bestXOffset] -y[bestYOffset] -z[0,1] <inputfolder> <outputfile> \nFlags:\t-h highlight visible areas with a black border on the top pyramid level. Default off.\n\t-x and -y Optional: set best X, Y offset of image if upper and lower pyramid levels are not aligned.\n\t-z Process Z-stack. Set to 0 to turn off. Default on if the image has one.\n\t-q Set minimal jpeg quality percentage. Default 90.\n";

  if (argc < 3)
  {
    std::cerr << syntax;
    return 1;
  }
  
  for (int arg=1; arg < argc; arg++)
  {
    if (argv[arg][0] == '-')
    {
      if (strlen(argv[arg]) < 3)
      {
        std::cerr << syntax;
        return 1;
      }
      if (argv[arg][1] == 'h')
      {
        if (argv[arg][2] == '0')
        {
          doBorderHighlight = false;
        }
        else if (argv[arg][2] == '1')
        {
          doBorderHighlight = true;
        }
        else
        {
          std::cerr << syntax;
          return 1;
        }
      }
      else if (argv[arg][1] == 'q')
      {
        quality = atoi(&argv[arg][2]);
      }
      else if (argv[arg][1] == 'x')
      {
        bestXOffset = atoi(&argv[arg][2]);  
      }
      else if (argv[arg][1] == 'y')
      {
        bestYOffset = atoi(&argv[arg][2]);
      }
      else if (argv[arg][1] == 'z')
      {
        if (argv[arg][2] == '0')
        {
          includeZStack = false;
        }
        else if (argv[arg][2] == '1')
        {
          includeZStack = true;
        }
        else
        {
          std::cerr << syntax;
          return 1;
        }
      }
      else
      {
        std::cerr << syntax;
        return 1;
      }
    }
    else
    {
      if (infile == 0)
      {
        infile=argv[arg];
      }
      else if (outfile == 0) 
      {
        outfile=argv[arg];
      }
    }
  }
  if (infile == 0 || outfile == 0)
  {
    std::cerr << syntax;
    return 1;
  }
  
  error=slideConv.open(infile, outfile, doBorderHighlight, includeZStack, quality, bestXOffset, bestYOffset);
  if (error==0)
  {
    error=slideConv.convert();
  }
  else if (error>0) 
  {
    if (error==1)
    {
      std::cerr << "Failed to open " << argv[1] << std::endl;
    }
    else if (error==2)
    {
      std::cerr << "Failed to create tif " << argv[2] << ": " << slideConv.getErrMsg() << std::endl;
    }
    else if (error==3)
    {
      std::cerr << "No valid levels found." << std::endl;
    }
    error++;
  }
  slideConv.closeRelated();
  return error;
}

