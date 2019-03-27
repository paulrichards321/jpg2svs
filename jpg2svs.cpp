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
#include <cstdint>
#include <cctype>
#if defined(_WIN32) || defined(_WIN64)
#include "console-mswin.h"
#include "getopt.h"
#else
#include "console-unix.h"
#include <unistd.h>
#endif
#include "tiffsupport.h"
#include "composite.h"

class SlideConvertor
{
protected:
  CompositeSlide *slide;
  Tiff *tif;
  std::ofstream *logFile;
  std::string errMsg;
  int64_t mBaseTotalWidth, mBaseTotalHeight;
  bool mValidObject;
  bool mBlendTopLevel, mBlendByRegion;
  bool mIncludeZStack;
  int mQuality;
  int mStep, mZSteps;
  int mLastZLevel, mLastDirection;
public:
  SlideConvertor();
  ~SlideConvertor() { closeRelated(); }
  void closeRelated();
  std::string getErrMsg() { return errMsg; }
  int open(std::string inputFile, std::string outputFile, bool blendTopLevel, bool blendByRegion, bool markOutline, bool includeZStack, int quality, int64_t bestXOffset = -1, int64_t bestYOffset = -1);
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
  int64_t srcTotalWidth=0;
  int64_t srcTotalHeight=0;
  int64_t srcTotalWidthL2=1;
  int64_t srcTotalHeightL2=1;
  int64_t destTotalWidth=0;
  int64_t destTotalHeight=0;
  const int bkgdLimit=16;
  int outputWidth=256;
  int outputHeight=256;
  int inputTileWidth=256+bkgdLimit;
  int inputTileHeight=256+bkgdLimit;
  double xScale=0.0, yScale=0.0;
  double xScaleL2=0.0, yScaleL2=0.0;
  double xScaleReverse=0.0, yScaleReverse=0.0;
  double xScaleResize=0.0, yScaleResize=0.0;
  int64_t grabWidthA=0, grabWidthB=0;
  int64_t grabHeightA=0, grabHeightB=0;
  int64_t grabWidthL2=0, grabHeightL2=0;
  BYTE* pSizedBitmap = 0;
  BYTE* pBitmap4 = NULL;
  std::ostringstream output;
  int readZLevel = 0;
  int readDirection = 0;
  unsigned char bkgColor=255;
  bool fillin = ((level < 2 && slide->checkLevel(2)) ? true : false);
  int scaleMethod=cv::INTER_CUBIC;
  int scaleMethodL2=cv::INTER_CUBIC;
  int64_t totalXSections=0, totalYSections=0;
  bool *xSubSections=NULL;
  bool *ySubSections=NULL;

  if (mBlendByRegion)
  {
    inputTileWidth=256;
    inputTileHeight=256;
  }
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
    double magnifyX=magnification;
    double magnifyY=magnification;
    double destTotalWidthDec = mBaseTotalWidth / magnifyX;
    double destTotalHeightDec = mBaseTotalHeight / magnifyY;
    destTotalWidth = (int64_t) destTotalWidthDec;
    destTotalHeight = (int64_t) destTotalHeightDec;
    xScale=(double) srcTotalWidth / (double) destTotalWidth;
    yScale=(double) srcTotalHeight / (double) destTotalHeight;
    xScaleReverse=(double) destTotalWidth / (double) srcTotalWidth;
    yScaleReverse=(double) destTotalHeight / (double) srcTotalHeight;
    grabWidthA=(int64_t) srcTotalWidth;
    grabHeightA=(int64_t) srcTotalHeight;
    grabWidthB=(int64_t) srcTotalWidth;
    grabHeightB=(int64_t) srcTotalHeight;

    xScaleL2=(double) srcTotalWidthL2 / (double) srcTotalWidth;
    yScaleL2=(double) srcTotalHeightL2 / (double) srcTotalHeight;
 
    grabWidthL2=(int64_t) srcTotalWidthL2;
    grabHeightL2=(int64_t) srcTotalHeightL2;

    inputTileWidth=destTotalWidth;
    inputTileHeight=destTotalHeight;
    outputWidth=destTotalWidth;
    outputHeight=destTotalHeight;
  } 
  else if (magnification==1 || slide->isPreviewSlide(level))
  {
    destTotalWidth = srcTotalWidth;
    destTotalHeight = srcTotalHeight;
    xScale=1.0;
    yScale=1.0;
    xScaleReverse=1.0;
    yScaleReverse=1.0;
    grabWidthA=inputTileWidth;
    grabHeightA=inputTileHeight;
    grabWidthB=outputWidth;
    grabHeightB=outputHeight;

    xScaleL2=(double) srcTotalWidthL2 / (double) destTotalWidth;
    yScaleL2=(double) srcTotalHeightL2 / (double) destTotalHeight;
    grabWidthL2=(int64_t)ceil((double) 256.0 * xScaleL2);
    grabHeightL2=(int64_t)ceil((double) 256.0 * yScaleL2);
  }
  else
  {
    double magnifyX=magnification;
    double magnifyY=magnification;
    double destTotalWidthDec = mBaseTotalWidth / magnifyX;
    double destTotalHeightDec = mBaseTotalHeight / magnifyY;
    destTotalWidth = (int64_t) destTotalWidthDec;
    destTotalHeight = (int64_t) destTotalHeightDec;
    xScale=(double) srcTotalWidth / (double) destTotalWidth;
    yScale=(double) srcTotalHeight / (double) destTotalHeight;
    xScaleReverse=(double) destTotalWidth / (double) srcTotalWidth;
    yScaleReverse=(double) destTotalHeight / (double) srcTotalHeight;
    grabWidthA=(int64_t)ceil((double) inputTileWidth * xScale);
    grabHeightA=(int64_t)ceil((double) inputTileHeight * yScale);
    grabWidthB=(int64_t)ceil((double) outputWidth * xScale);
    grabHeightB=(int64_t)ceil((double) outputHeight * yScale);

    xScaleL2=(double) srcTotalWidthL2 / (double) srcTotalWidth;
    yScaleL2=(double) srcTotalHeightL2 / (double) srcTotalHeight;
 
    grabWidthL2=(int64_t)ceil((double) 256.0 * (double) srcTotalWidthL2 / (double) destTotalWidth);
    grabHeightL2=(int64_t)ceil((double) 256.0 * (double) srcTotalHeightL2 / (double) destTotalHeight);
  }
  if (xScaleReverse < 1.0 || yScaleReverse < 1.0)
  {
    scaleMethod=cv::INTER_AREA;
  }
  if (mBlendByRegion==false)
  {
    pBitmap4 = new BYTE[outputWidth * outputHeight * 3];
    if (pBitmap4 == NULL)
    {
      output << "Out of memory allocating final tile bitmap. Cannot finish scaling!" << std::endl;
      return 1;
    }
    totalXSections = (int64_t) ceil((double) destTotalWidth / (double) outputWidth)*outputWidth;
    totalYSections = (int64_t) ceil((double) destTotalHeight / (double) outputHeight)*outputHeight;
    xSubSections=new bool[totalXSections];
    ySubSections=new bool[totalYSections];
    if (xSubSections == NULL || ySubSections == NULL)
    {
      output << "Out of memory tile subsections. Cannot finish scaling!" << std::endl;
      return 1;
    } 
    memset(xSubSections, 0, totalXSections);
    memset(ySubSections, 0, totalYSections);
  }
  *logFile << " xScale=" << xScale << " yScale=" << yScale;
  *logFile << " srcTotalWidth=" << srcTotalWidth << " srcTotalHeight=" << srcTotalHeight;
  *logFile << " destTotalWidth=" << destTotalWidth << " destTotalHeight=" << destTotalHeight;
  *logFile << " srcTotalWidthL2=" << srcTotalWidthL2 << " srcTotalHeightL2=" << srcTotalHeightL2;
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
  if (tif->setAttributes(3, 8, destTotalWidth, destTotalHeight, (tiled==true ? outputWidth : 0), (tiled==true ? outputHeight : 0), 1, quality)==false || tif->setDescription(strAttributes, mBaseTotalWidth, mBaseTotalHeight)==false)
  {
    std::string errMsg;
    tif->getErrMsg(errMsg);
    std::cerr << "Failed to write tif attributes: " << errMsg << std::endl; 
    return 4;
  }
  int64_t yDest=0, xDest=0;
  int perc=0, percOld=0;
  bool onePercHit=false;
  time_t timeStart=0, timeLast=0;
  bool error=false;
  timeStart = time(NULL);
  int64_t ySrc=0;
  retractCursor();
  std::cout << "0% done...    " << std::flush;

  BYTE *pBitmapL2 = NULL;

  int64_t L2Size=srcTotalWidthL2 * srcTotalHeightL2 * 3;
  if (fillin)
  {
    pBitmapL2 = new BYTE[L2Size];
    if (pBitmapL2==NULL)
    {
      *logFile << "Failed to allocate memory for full pyramid level 2. Out of memory?" << std::endl;
      return 1;
    }
  }
  int64_t readWidthL2=0;
  int64_t readHeightL2=0;
  bool readOkL2=false;
  if (pBitmapL2)
  {
    readOkL2=slide->read(pBitmapL2, 2, 0, 0, 0, 0, srcTotalWidthL2, srcTotalHeightL2, false, &readWidthL2, &readHeightL2);
    if (readOkL2==false)
    {
      *logFile << "Failed to read full pyramid level 2." << std::endl;
    }
    else if (srcTotalWidthL2 > 0 && srcTotalHeightL2 > 0)
    {
      xScaleResize=(double) destTotalWidth / (double) srcTotalWidthL2;
      yScaleResize=(double) destTotalHeight / (double) srcTotalHeightL2;
      if (xScaleResize < 1.0 || yScaleResize < 1.0)
      {
        scaleMethodL2=cv::INTER_AREA;
      }
    }
  }
  while (ySrc<srcTotalHeight && yDest<destTotalHeight && error==false)
  {
    xDest = 0;
    for (int64_t xSrc=0; xSrc<srcTotalWidth && xDest<destTotalWidth && error==false; xSrc += grabWidthB) 
    {
      bool toSmall = false;
      BYTE *pBitmap1 = slide->allocate(level, xSrc, ySrc, grabWidthA, grabHeightA, false);
      BYTE *pBitmap2 = pBitmap1;
      int64_t readWidth=0, readHeight=0;
      bool readOk=false;
      if (pBitmap1)
      {
        *logFile << " slide->read(x=" << xSrc << " y=" << ySrc << " grabWidthA=" << grabWidthA << " grabHeightA=" << grabHeightA << " level=" << level << "); " << std::endl;
        readOk=slide->read(pBitmap2, level, readDirection, readZLevel, xSrc, ySrc, grabWidthA, grabHeightA, false, &readWidth, &readHeight);
      }
      if (readOk)
      {
        cv::Mat imgScaled;
        cv::Mat imgScaled2;
        BYTE *pBitmap3 = NULL;
        BYTE *pBitmapFinal = pBitmap2;
        if (readWidth != grabWidthA || readHeight != grabHeightA)
        {
        //    *logFile << "Tile shorter: width=" << readWidth << " height=" << readHeight << std::endl;
          toSmall = true;
          pSizedBitmap = new BYTE[grabWidthA*grabHeightA*3];
          memset(pSizedBitmap, bkgColor, grabHeightA*grabWidthA*3);
          for (int64_t row=0; row < readHeight; row++)
          {
            memcpy(&pSizedBitmap[row*grabWidthA*3], &pBitmap2[row*readWidth*3], readWidth*3);
          }
          pBitmap2 = pSizedBitmap;
          pBitmapFinal = pSizedBitmap;
        }
        if (tiled==false)
        {
          cv::Mat imgSrc(grabHeightA, grabWidthA, CV_8UC3, pBitmap2);
          cv::Size scaledSize(destTotalWidth, destTotalHeight);
          cv::resize(imgSrc, imgScaled, scaledSize, xScaleReverse, yScaleReverse, scaleMethod);
          imgSrc.release();
          pBitmap2 = imgScaled.data;  
          pBitmapFinal = imgScaled.data;
        } 
        else if (grabWidthA!=inputTileWidth || grabHeightA!=inputTileHeight)
        {
          cv::Mat imgSrc(grabHeightA, grabWidthA, CV_8UC3, pBitmap2);
          cv::Size scaledSize(inputTileWidth, inputTileHeight);
          cv::resize(imgSrc, imgScaled, scaledSize, xScaleReverse, yScaleReverse, scaleMethod);
          imgSrc.release();
          pBitmap2 = imgScaled.data;  
          pBitmapFinal = imgScaled.data;
        }
        if (readOkL2)
        {
          int64_t xSrcStartL2=round((double) xSrc * xScaleL2);
          int64_t ySrcStartL2=round((double) ySrc * yScaleL2);
          int64_t xSrcEndL2=xSrcStartL2 + grabWidthL2;
          int64_t ySrcEndL2=ySrcStartL2 + grabHeightL2;
          if (xSrcEndL2 > readWidthL2) xSrcEndL2=readWidthL2;
          if (ySrcEndL2 > readHeightL2) ySrcEndL2=readHeightL2;
          int64_t grabWidth2=xSrcEndL2 - xSrcStartL2;
          int64_t grabHeight2=ySrcEndL2 - ySrcStartL2;
          int64_t rowSize = grabWidthL2 * 3;
          int64_t copySize = grabWidth2 * 3;
          int64_t tileSize = rowSize * grabHeightL2;
          if (magnification==32 && level==1)
          {
            *logFile << " xSrc=" << xSrc << " xSrcStartL2=" << xSrcStartL2 << " ySrc=" << ySrc << " ySrcStartL2=" << ySrcStartL2 << " grabWidthL2=" << grabWidthL2 << " grabHeightL2=" << grabHeightL2 << std::endl;
          }
          if (xSrcStartL2 >= 0 && xSrcStartL2 < readWidthL2 && ySrcStartL2 >= 0 && ySrcStartL2 < readHeightL2)
          {
            pBitmap3 = new BYTE[tileSize];
            memset(pBitmap3, bkgColor, tileSize);
            for (int64_t row=0; row < grabHeight2; row++)
            {
              int64_t offset3 = row*rowSize;
              int64_t offset4 = ((ySrcStartL2+row)*readWidthL2*3)+(xSrcStartL2*3);
              if (offset3 + copySize <= tileSize && offset4 + copySize <= L2Size)
              {
                memcpy(&pBitmap3[offset3], &pBitmapL2[offset4], copySize);
              }
            }
            cv::Mat imgSrc(grabHeightL2, grabWidthL2, CV_8UC3, pBitmap3);
            cv::Size scaledSize(outputWidth, outputHeight);
            cv::resize(imgSrc, imgScaled2, scaledSize, xScaleResize, yScaleResize, scaleMethodL2);
            imgSrc.release();
            if (mBlendByRegion)
            {
              slide->blendLevelsByRegion(imgScaled2.data, pBitmap2, xSrc, ySrc, grabWidthA, grabHeightA, inputTileWidth, inputTileHeight, xScaleReverse, yScaleReverse, level); 
              pBitmapFinal=imgScaled2.data;
            }
            else
            {
              blendLevelsByBkgd(pBitmap4, pBitmap2, imgScaled2.data, xDest, yDest, inputTileWidth, inputTileHeight, bkgdLimit, xSubSections, totalXSections, ySubSections, totalYSections, 245, tiled);
              pBitmapFinal = pBitmap4;
            }
          } 
        } 
        bool writeOk=false;
        if (tiled)
          writeOk=tif->writeEncodedTile(pBitmapFinal, xDest, yDest, 1);
        else
          writeOk=tif->writeImage(pBitmapFinal);
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
      xDest += outputWidth;
    }
    yDest += outputHeight;
    ySrc += grabHeightB;
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
  if (pBitmap4)
  {
    delete[] pBitmap4;
    pBitmap4 = NULL;
  }

  timeLast = time(NULL);
  if (error==false)
  {
    std::cout << "Took " << timeLast - timeStart << " seconds for this level." << std::endl;
  }
  mStep++;
  return (error==true ? 1 : 0); 
}


int SlideConvertor::convert()
{
  //----------------------------------------------------------------------
  // Below three blocks for blending the middle level into the top level
  //----------------------------------------------------------------------
  int smallBlendLevels[] =          {  0,  1,  1,   1,   1 };
  int smallBlendMagnifyLevels[] =   {  1,  64,  4,  16,  32 };
  int bigBlendLevels[] =            {  0,  1,  1,   1,   1,   1 };
  int bigBlendMagnifyLevels[] =     {  1,  128,  4,  16,  64, 128 };
  
  int smallBlendZLevels1[] =        {  0,  1,  1,   1,   1 };
  int smallBlendZMagnifyLevels1[] = {  1,  64,  4,  16,  32 };
  int smallBlendZLevels2[] =        {  0,  1,  1,   1 };
  int smallBlendZMagnifyLevels2[] = {  1,  4, 16,  32 };

  int bigBlendZLevels1[] =          {  0,  1,  1,   1,   1,   1 };
  int bigBlendZMagnifyLevels1[] =   {  1,  256,  4,  16,  64, 128 };
  int bigBlendZLevels2[] =          {  0,  1,  1,   1,   1 };
  int bigBlendZMagnifyLevels2[] =   {  1,  4, 16,  64, 128 };

  //----------------------------------------------------------------------
  // Below three blocks for NOT blending the middle level into the top level
  //----------------------------------------------------------------------
  int smallLevels[] =          {  0,  2,  1,   1,   2 };
  int smallMagnifyLevels[] =   {  1,  64,  4,  16,  32 };
  int bigLevels[] =            {  0,  3,  1,   1,   2,   2 };
  int bigMagnifyLevels[] =     {  1,  1,  4,  16,  64, 128 };
  
  int smallZLevels1[] =        {  0,  2,  1,   1,   2 };
  int smallZMagnifyLevels1[] = {  1,  64,  4,  16,  32 };
  int smallZLevels2[] =        {  0,  1,  1,   2 };
  int smallZMagnifyLevels2[] = {  1,  4, 16,  32 };

  int bigZLevels1[] =          {  0,  2,  1,   1,   2,   2 };
  int bigZMagnifyLevels1[] =   {  1,  256,  4,  16,  64, 128 };
  int bigZLevels2[] =          {  0,  1,  1,   2,   2 };
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
      if (mBlendTopLevel)
      {
        levels1 = bigBlendLevels;
        magnifyLevels1 = bigBlendMagnifyLevels;
      }
      else
      {
        levels1 = bigLevels;
        magnifyLevels1 = bigMagnifyLevels;
      }
    }
    else
    {
      totalLevels1 = 5;
      if (mBlendTopLevel)
      {
        levels1 = smallBlendLevels;
        magnifyLevels1 = smallBlendMagnifyLevels;
      }
      else
      {
        levels1 = smallLevels;
        magnifyLevels1 = smallMagnifyLevels;
      }
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
      if (mBlendTopLevel)
      {
        levels1 = bigBlendZLevels1;
        levels2 = bigBlendZLevels2;
        magnifyLevels1 = bigBlendZMagnifyLevels1;
        magnifyLevels2 = bigBlendZMagnifyLevels2;
      }
      else
      {
        levels1 = bigZLevels1;
        levels2 = bigZLevels2;
        magnifyLevels1 = bigZMagnifyLevels1;
        magnifyLevels2 = bigZMagnifyLevels2;
      }
    }
    else
    {
      if (mBlendTopLevel)
      {
        totalLevels1 = 5;
        totalLevels2 = 4;
        levels1 = smallBlendZLevels1;
        levels2 = smallBlendZLevels2;
        magnifyLevels1 = smallBlendZMagnifyLevels1;
        magnifyLevels2 = smallBlendZMagnifyLevels2;
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


int SlideConvertor::open(std::string inputFile, std::string outputFile, bool blendTopLevel, bool blendByRegion, bool markOutline, bool includeZStack, int quality, int64_t bestXOffset, int64_t bestYOffset)
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
  mBlendTopLevel = blendTopLevel;
  mBlendByRegion = blendByRegion;
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


bool getBoolOpt(const char *optarg, bool& invalidOpt)
{
  const char *available[14] = { 
    "1", 
    "true", "TRUE", 
    "enable", "ENABLE", 
    "on", "ON", 
    "0", 
    "false", "FALSE", 
    "disable", "DISABLE", 
    "off", "OFF" 
  };
  if (optarg == NULL) 
  {
    invalidOpt = true;
    return false;
  }
  std::string optarg2 = optarg;
  for (int i = 0; i < 14; i++)
  {
    if (optarg2.find(available[i]) != std::string::npos)
    {
      if (i < 7) return true;
      else return false;
    }
  }
  invalidOpt = true;
  return false;
}


int getIntOpt(const char *optarg, bool& invalidOpt)
{
  unsigned int i=0;
  if (optarg==NULL)
  {
    invalidOpt = true;
    return 0;
  }
  std::string optarg2 = optarg;
  while (i < optarg2.length() && (optarg2[i]==' ' || optarg2[i]==':' || optarg2[i]=='=' || optarg2[i]=='\t')) i++;
  optarg2 = optarg2.substr(i);
  if (optarg2.length() > 0 && isdigit(optarg2[0]))
  {
    return atoi(optarg2.c_str());
  }
  invalidOpt = true;
  return 0;
}


int main(int argc, char** argv)
{
  SlideConvertor slideConv;
  int error=0;
  std::string infile, outfile;
  int64_t bestXOffset = -1, bestYOffset = -1;
  bool blendTopLevel = true;
  bool blendByRegion = false;
  bool doBorderHighlight = true;
  bool includeZStack = false;
  int quality = 90;
  char syntax[] = "syntax: jpg2svs -b=[on,off] -h=[on,off] -x=[bestXOffset] -y=[bestYOffset] -z=[on,off] <inputfolder> <outputfile> \nFlags:\t-b blend the top level with the middle level. Default on.\n\t-r blend the top level with the middle level only by region, not by empty background. Default off.\n\t-h highlight visible areas with a black border. Default on.\n\t-q Set minimal jpeg quality percentage. Default 90.\n\t-x and -y Optional: set best X, Y offset of image if upper and lower pyramid levels are not aligned.\n\t-z Process Z-stack if the image has one. Default off.\n";

  if (argc < 3)
  {
    std::cerr << syntax;
    return 1;
  }
  int opt;
  bool invalidOpt = false;
  while((opt = getopt(argc, argv, "b:h:r:q:x:y:z:")) != -1)
  {
    if (optarg != NULL) std::cout << " optarg=" << optarg << std::endl;
    switch (opt)
    {
      case 'b':
        blendTopLevel = getBoolOpt(optarg, invalidOpt);
        break;
      case 'h':
        doBorderHighlight = getBoolOpt(optarg, invalidOpt);
        break;
      case 'r':
        blendByRegion = getBoolOpt(optarg, invalidOpt);
        break;
      case 'q':
        quality = getIntOpt(optarg, invalidOpt);
        break;
      case 'x':
        bestXOffset = getIntOpt(optarg, invalidOpt);
        break;
      case 'y':
        bestYOffset = getIntOpt(optarg, invalidOpt);
        break;
      case 'z':
        includeZStack = getBoolOpt(optarg, invalidOpt);
        break;
      case '?':
        if (infile.length() == 0)
        {
          infile=optarg;
        }
        else if (outfile.length() == 0)
        {
          outfile=optarg;
        }
        else
        {
          invalidOpt = true;
        }
        break;
    }
    if (invalidOpt)
    {
      std::cerr << syntax;
      return 1;
    } 
  }
  for (; optind < argc; optind++)
  {
    if (infile.length() == 0)
    {
      infile=argv[optind];
    }
    else if (outfile.length() == 0)
    {
      outfile=argv[optind];
    }
  }
  if (infile.length() == 0 || outfile.length() == 0)
  {
    std::cerr << syntax;
    return 1;
  }
  
  error=slideConv.open(infile.c_str(), outfile.c_str(), blendTopLevel, blendByRegion, doBorderHighlight, includeZStack, quality, bestXOffset, bestYOffset);
  if (error==0)
  {
    error=slideConv.convert();
  }
  else if (error>0) 
  {
    if (error==1)
    {
      std::cerr << "Failed to open " << infile << std::endl;
    }
    else if (error==2)
    {
      std::cerr << "Failed to create tif " << outfile << ": " << slideConv.getErrMsg() << std::endl;
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

