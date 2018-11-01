/*************************************************************************
Initial Author: Paul F. Richards (paulrichards321@gmail.com) 2016-2017
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
**************************************************************************/

#include <new>
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32) | defined(_WIN64)
#define stat _stat
#define strcasecmp _stricmp
#else
#include <unistd.h>
#endif
#include "imagesupport.h"
#include "jpgcachesupport.h"
#include "composite.h"

JpgCache jpgCache(1000);

const char* CompositeSlide::miniNames[] = 
{ 
  "FinalScan.ini", "FinalCond.ini", 
  "SlideScan.ini", "SlideCond.ini" 
};

CompositeSlide::CompositeSlide()
{
  initialize();
  mValidObject = false;
  mbkgColor = 255;
}


IniConf::IniConf()
{
  mname="";
  mfound=false;
  mxMin=0;
  mxMax=0;
  mxDiffMin=0;
  myMin=0;
  myMax=0;
  myDiffMin=0;
  mxAdj=0.0;
  myAdj=0.0;
  mtotalTiles=0;
  mxAxis=0;
  myAxis=0;
  mpixelWidth=0;
  mpixelHeight=0;
  mtotalWidth=0;
  mtotalHeight=0;
  myStepSize=0;
  mxStepSize=0;
  mIsPreviewSlide=false;
  mquality=0;
  for (int zSplit=0; zSplit < 2; zSplit++)
  {
    for (int zLevel=0; zLevel < 4; zLevel++)
    {
      mzStackExists[zSplit][zLevel] = false;
    }
  }
}


bool CompositeSlide::isPreviewSlide(size_t level)
{
  if (level < mConf.size() && mConf[level]->mfound) 
  {
    return mConf[level]->mIsPreviewSlide; 
  }
  return false;
}


bool CompositeSlide::checkZLevel(int level, int direction, int zLevel)
{
  return (mValidObject == true && level >= 0 && level < (int) mConf.size() && direction >= 0 && direction < 3 && mConf[level]->mfound && (direction==0 || mConf[level]->mzStackExists[direction-1][zLevel])) ? true : false; 
}


void CompositeSlide::initialize()
{
  mValidObject = false;
  
  //std::cout << "Ini Conf size: " << mConf.size();
  if (mConf.size()>0)
  {
    for (size_t i = 0; i < mConf.size(); i++)
    {
      if (mConf[i])
      {
        delete mConf[i];
        mConf[i] = NULL;
      }
    }
    mConf.clear();
  }
  for (int i=0; i<4; i++)
  {
    IniConf *mConfLocal = new IniConf;
    mConfLocal->mname = miniNames[i];
    mConf.push_back(mConfLocal);
  }
  mxMin=0;
  mxMax=0;
  myMin=0;
  myMax=0;
  mmagnification = 0;
  mTotalZLevels = 0;
  mTotalBottomZLevels = 0;
  mTotalTopZLevels = 0;
  mGrayScale = false;
} 


void CompositeSlide::close()
{
  mValidObject = false;
  if (mConf.size()>0)
  {
    for (size_t i = 0; i < mConf.size(); i++)
    {
      if (mConf[i])
      {
        delete mConf[i];
        mConf[i] = NULL;
      }
    }
    mConf.clear();
  }
}


bool CompositeSlide::checkLevel(int level)
{
  if (mValidObject == false || level < 0 || level > (int) mConf.size() || mConf[level]->mfound == false) 
  {
    return false;
  }
  return true;
}


bool CompositeSlide::open(const std::string& srcFileName, bool setGrayScale)
{
  JpgFileXY jpgxy;
  JpgFileXY jpgxyzstack;                  
  bool nameFound=false;
  bool xFound=false, yFound=false;
  bool header=false;
  std::string iImageWidth = "iImageWidth";
  std::string iImageHeight = "iImageHeight";
  std::string lXStageRef = "lXStageRef";
  std::string lYStageRef = "lYStageRef";
  std::string lXStepSize = "lXStepSize";
  std::string lYStepSize = "lYStepSize";
  std::string lXOffset = "lXOffset";
  std::string lYOffset = "lYOffset";
  std::string headerStr = "header";
  std::string dMagnification = "dMagnification";
  std::string ImageQuality = "ImageQuality";
  std::string inputDir = srcFileName;
  mGrayScale = setGrayScale;
  int xAxis, yAxis;
  int xOffset, yOffset;

  initialize();

  for (int i=0; i<4; i++)
  {
    size_t namePos=inputDir.find(miniNames[i]);
    if (namePos != std::string::npos)
    {
      inputDir = srcFileName.substr(0, namePos-1);
      break;
    }
  }
  if (inputDir.length()>0)
  {
    char lastKey=inputDir[inputDir.length()];
    if (lastKey=='\\' || lastKey=='/')
    {
      inputDir=inputDir.substr(0, inputDir.length()-1);
    }
  }
  
  std::fstream logFile("SlideScan.openimage.log", std::ios::out);
  for (int fileNum = 0; fileNum < 4; fileNum++)
  {
    IniConf* pConf = mConf[fileNum];
    std::string inputName = inputDir;
    inputName += separator();
    inputName += miniNames[fileNum];

    xFound = false;
    yFound = false;
    nameFound = false;
    header = false;
    std::cout << "Trying to open " << inputName << "... ";
    std::ifstream iniFile(inputName.c_str());
    if (iniFile.good())
    {
      std::cout << "Success!" << std::endl;
    }
    else
    {
      std::cout << "failed to open file. " << std::endl;
    }

    int c = 0;
    while (iniFile.good() && iniFile.eof()==false)
    {
      std::string line="";
      do
      {
        c = iniFile.get();
        if (c == 0x0A || c == EOF) break;
        if (c != 0x0D && c != ' ' && c != '\t') line += (char) c;
        } while (iniFile.eof()==false && iniFile.good());

        if (line.length()>=3)
        {
          size_t rpos = line.find(']');
          if (line[0]=='[' && rpos != std::string::npos)
          {
            if (xFound && yFound && nameFound)
            {
              pConf->mxyArr.push_back(jpgxy);
            }
            std::string chunkName=line.substr(1, rpos-1);
            //gchar* foldedChunkName = g_utf8_casefold((gchar*)chunkName.c_str(), chunkName.length());
            if (strcasecmp(headerStr.c_str(), chunkName.c_str())==0)
            {
              //    std::cout << "In header!" << std::endl;
              jpgxy.mBaseFileName.clear();
              nameFound = false;
              header = true;
            }
            else
            {
              //logFile << "Input Dir=" << inputDir << std::endl;
              jpgxy.mBaseFileName = inputDir;
              jpgxy.mBaseFileName += separator();
              jpgxy.mBaseFileName += chunkName;
              char location[2][4] = { "_u", "_d" };
              struct stat fileStat;
              for (int zSplit=0; zSplit < 2; zSplit++)
              {
                for (int zLevel=0; zLevel<4; zLevel++)
                {
                  std::ostringstream nameStream;
                  nameStream << jpgxy.mBaseFileName << location[zSplit] << (zLevel+1) << ".jpg";
                  jpgxy.mFileName[zSplit][zLevel] = nameStream.str();
                  if (stat(jpgxy.mFileName[zSplit][zLevel].c_str(), &fileStat)==0)
                  {
                    pConf->mzStackExists[zSplit][zLevel] = true;
                    jpgxy.mzStack[zSplit][zLevel] = true;
                  }
                  else
                  {
                    jpgxy.mzStack[zSplit][zLevel] = false;
                  }
                }
              }
              jpgxy.mBaseFileName += ".jpg";
              nameFound = true;
              header = false;
              //logFile << "Filename=" << jpgxy.fileName << std::endl;
            }
            //g_object_unref(foldedHeaderName);
            xFound = false;
            yFound = false;
          }
          if (header)
          {
            size_t widthPos=line.find(iImageWidth);
            if (widthPos != std::string::npos && widthPos+iImageWidth.length()+1 < line.length())
            {
              std::string width = line.substr(widthPos+iImageWidth.length()+1);
              pConf->mpixelWidth=atoi(width.c_str());
            }
            size_t heightPos=line.find(iImageHeight);
            if (heightPos != std::string::npos && heightPos+iImageHeight.length()+1 < line.length())
            {
              std::string height = line.substr(heightPos+iImageHeight.length()+1);
              pConf->mpixelHeight=atoi(height.c_str());
            }
            size_t xStagePos=line.find(lXStageRef);
            if (xStagePos != std::string::npos && xStagePos+lXStageRef.length()+1<line.length())
            {
              std::string xStageSubStr = line.substr(xStagePos+lXStageRef.length()+1);
              pConf->mxAxis=atoi(xStageSubStr.c_str());
              xAxis=pConf->mxAxis;
            //  logFile << "!!xAxis " << pConf->xAxis << std::endl;
            }
            size_t yStagePos=line.find(lYStageRef);
            if (yStagePos != std::string::npos && yStagePos+lYStageRef.length()+1<line.length())
            {
              std::string yStageSubStr = line.substr(yStagePos+lYStageRef.length()+1);
              pConf->myAxis=atoi(yStageSubStr.c_str());
              yAxis=pConf->myAxis;
            //  logFile << "!!yAxis " << pConf->yAxis << std::endl;
            }
            size_t yStepPos = line.find(lYStepSize);
            if (yStepPos != std::string::npos && yStepPos+lYStepSize.length()+1<line.length())
            {
              std::string yStepSubStr = line.substr(yStepPos+lYStepSize.length()+1);
              pConf->myStepSize = atoi(yStepSubStr.c_str());
//              logFile << " fileNum=" << fileNum << " yStepSize=" << pConf->myStepSize << std::endl;
              std::cout << "Exact y step measurements found for level " << fileNum << std::endl;
            }
            size_t xStepPos = line.find(lXStepSize);
            if (xStepPos != std::string::npos && xStepPos+lXStepSize.length()+1<line.length())
            {
              std::string xStepSubStr = line.substr(xStepPos+lXStepSize.length()+1);
              pConf->mxStepSize = atoi(xStepSubStr.c_str());
//              logFile << " fileNum=" << fileNum << " xStepSize=" << pConf->mxStepSize << std::endl;
              std::cout << "Exact x step measurements found for level " << fileNum << std::endl;
            }
            size_t xOffsetPos = line.find(lXOffset);
            if (xOffsetPos != std::string::npos && xOffsetPos+lXOffset.length()+1<line.length())
            {
              std::string xOffsetSubStr = line.substr(xOffsetPos+lXOffset.length()+1);
              xOffset = atoi(xOffsetSubStr.c_str());
            }
            size_t yOffsetPos = line.find(lYOffset);
            if (yOffsetPos != std::string::npos && yOffsetPos+lYOffset.length()+1<line.length())
            {
              std::string yOffsetSubStr = line.substr(yOffsetPos+lYOffset.length()+1);
              yOffset = atoi(yOffsetSubStr.c_str());
            }
            size_t dMagPos = line.find(dMagnification);
            if (dMagPos != std::string::npos && dMagPos+dMagnification.length()+1<line.length())
            {
              std::string dMagSubStr = line.substr(dMagPos+dMagnification.length()+1);
              mmagnification = atoi(dMagSubStr.c_str());
            }
            size_t qualityPos = line.find(ImageQuality);
            if (qualityPos != std::string::npos && qualityPos+ImageQuality.length()+1<line.length())
            {
              std::string qualitySubStr = line.substr(qualityPos+ImageQuality.length()+1);
              pConf->mquality = atoi(qualitySubStr.c_str());
            }
          }
          std::string line2=line.substr(0, 2);
          if (line2=="x=")
          {
            std::string somenum=line.substr(2);
            jpgxy.mx=atoi(somenum.c_str());
            if (header) 
            {
  //            pConf->xAxis=jpgxy.x;
              jpgxy.mx=0;
            //  logFile << "!!xAxis " << pConf->xAxis << std::endl;
            }
            else
            {
              xFound=true;
            }
          }
          if (line2=="y=")
          {
          std::string somenum=line.substr(2);
          jpgxy.my=atoi(somenum.c_str());
          if (header) 
          {
//            pConf->yAxis=jpgxy.y;
            jpgxy.my=0;
          //  logFile << "!!yAxis " << pConf->yAxis << std::endl;
          }
          else
          {
            yFound=true;
          }
        }
      }
    }
    if (xFound && yFound && nameFound)
    {
      pConf->mxyArr.push_back(jpgxy);
    }
    iniFile.close();
  }
  
  
  myMin=0, myMax=0, mxMin=0, mxMax=0;
  bool yMinSet=false, xMaxSet=false, xMinSet=false, yMaxSet=false;
  for (int fileNum=0; fileNum < 4; fileNum++)
  {
    IniConf* pConf=mConf[fileNum];
    if (pConf->mxyArr.size()==0) continue;

    pConf->mtotalTiles = pConf->mxyArr.size();
    if (pConf->mpixelWidth<=0 || pConf->mpixelHeight<=0)
    {
      Jpg jpg;
      jpg.setUnfilledColor(mbkgColor);
      if (jpg.open(pConf->mxyArr[0].mBaseFileName))
      {
        pConf->mpixelWidth=jpg.getActualWidth();
        pConf->mpixelHeight=jpg.getActualHeight();
        jpg.close();
      }
      else
      {
        std::string errMsg;
        jpg.getErrMsg(errMsg);
        std::cerr << "Error: failed to open " << pConf->mxyArr[0].mBaseFileName << " do not have pixel width and height for source jpgs." << std::endl;
        std::cerr << "Returned error: " << errMsg << std::endl;
        logFile << "Error: failed to open " << pConf->mxyArr[0].mBaseFileName << " do not have pixel width and height for source jpgs." << std::endl;
        logFile << "Returned error: " << errMsg << std::endl;
        logFile.close();
        return false;
      }
    }
    logFile << "fileName=" << pConf->mname << " jpgWidth=" << pConf->mpixelWidth << " jpgHeight=" << pConf->mpixelHeight << std::endl;
    pConf->mfound = true;
    
    //************************************************************************
    // Get the xmin and xmax values
    //************************************************************************
    std::sort(pConf->mxyArr.begin(), pConf->mxyArr.end(), JpgFileXYSortForX());
    pConf->mxMin = pConf->mxyArr[0].mx;
    pConf->mxMax = pConf->mxyArr[pConf->mtotalTiles-1].mx;
    for (int i=0; i+1 < pConf->mtotalTiles; i++)
    {
      //logFile << " Sorted: x=" << pConf->xyArr[i].x << " y=" << pConf->xyArr[i].y << std::endl;
      if (pConf->mxyArr[i+1].mx==pConf->mxyArr[i].mx)
      {
        int diff=pConf->mxyArr[i+1].my - pConf->mxyArr[i].my;
        if ((diff>0 && diff<pConf->myDiffMin) || (diff>0 && pConf->myDiffMin<1))
        {
          pConf->myDiffMin=diff;
        }
      }
    }

    //************************************************************************
    // Get the ymin and ymax values
    //************************************************************************
    std::sort(pConf->mxyArr.begin(), pConf->mxyArr.end(), JpgFileXYSortForY());
    pConf->myMin=pConf->mxyArr[0].my;
    pConf->myMax=pConf->mxyArr[pConf->mtotalTiles-1].my; // + pConf->yDiffMin;

    logFile << "fileName=" << pConf->mname << " yDiffMin=" << pConf->myDiffMin << " yStepSize=" << pConf->myStepSize << " yMin=" << pConf->myMin << " yMax=" << pConf->myMax << " yAxis=" << pConf->myAxis << std::endl;
    for (int i=0; i+1 < pConf->mtotalTiles; i++)
    {
      //logFile << " Sorted: x=" << pConf->xyArr[i].x << " y=" << pConf->xyArr[i].y << std::endl;
      if (pConf->mxyArr[i+1].my==pConf->mxyArr[i].my)
      {
        int diff=pConf->mxyArr[i].mx - pConf->mxyArr[i+1].mx;
        if ((diff>0 && diff<pConf->mxDiffMin) || (diff>0 && pConf->mxDiffMin<1)) 
        {
          pConf->mxDiffMin=diff;
        }
      }
    }
    if (fileNum<2)
    {
      //pConf->xMin += xOffset; // Note try removing this!
      //pConf->yMin += yOffset;
    }

    logFile << "fileName=" << pConf->mname << " xDiffMin=" << pConf->mxDiffMin << " xStepSize=" << pConf->mxStepSize << " xMin=" << pConf->mxMin << " xMax=" << pConf->mxMax << " xAxis=" << pConf->mxAxis << std::endl;
    if (pConf->mpixelWidth>0) 
    {
      if (pConf->mxStepSize>0)
      {
        pConf->mxAdj = (double) pConf->mxStepSize / (double) pConf->mpixelWidth;
        logFile << "fileName=" << pConf->mname << " Read xAdj=" << pConf->mxAdj << std::endl;
	    	pConf->mxMin -= pConf->mxStepSize;
      }
      else
      {
        if (fileNum>0 && mConf[fileNum-1]->mfound && mConf[fileNum-1]->mxStepSize>0)
        {
          pConf->mxAdj = (double) (mConf[fileNum-1]->mxStepSize*4) / (double) pConf->mpixelWidth;
	  			pConf->mxMin -= mConf[fileNum-1]->mxStepSize*4;
        }
        else
        {
          pConf->mxAdj = (double) (pConf->mxDiffMin) / (double) pConf->mpixelWidth;
				  pConf->mxMin -= pConf->mxDiffMin;
        }
        logFile << "fileName=" << pConf->mname << " Guessed xAdj=" << pConf->mxAdj << std::endl;
      }
    }
    if (pConf->mpixelHeight>0)
    {
      if (pConf->myStepSize>0)
      {
        pConf->myAdj = (double) pConf->myStepSize / (double) pConf->mpixelHeight;
        logFile << "fileName=" << pConf->mname << " Read yAdj=" << pConf->myAdj << std::endl;
    		pConf->myMin -= pConf->myStepSize;
      }
      else
      {
        if (fileNum>0 && mConf[fileNum-1]->mfound && mConf[fileNum-1]->myStepSize>0)
        {
          pConf->myAdj = (double) (mConf[fileNum-1]->myStepSize*4) / (double) pConf->mpixelHeight;
				  pConf->myMin -= mConf[fileNum-1]->myStepSize*4;
        }
        else
        {
          pConf->myAdj = (double) (pConf->myDiffMin) / (double) pConf->mpixelHeight;
	  			pConf->myMin -= pConf->myDiffMin;
        }
        logFile << "fileName=" << pConf->mname << " Guessed yAdj=" << pConf->myAdj << std::endl;
      }
    }
    pConf->mdetailedWidth = (pConf->mxMax - pConf->mxMin + pConf->mxStepSize) / pConf->mxAdj;
    pConf->mdetailedHeight = (pConf->myMax - pConf->myMin + pConf->myStepSize) / pConf->myAdj;
    if ((yMinSet==false || pConf->myMin < myMin) && fileNum < 3)
    {
      myMin=pConf->myMin;
      yMinSet = true;
    }
    if ((yMaxSet==false || pConf->myMax > myMax) && fileNum < 3)
    {
      myMax=pConf->myMax;
      yMaxSet = true;
    }
    if ((xMinSet==false || pConf->mxMin < mxMin) && fileNum < 3)
    { 
      mxMin=pConf->mxMin;
      xMinSet = true;
    }
    if ((xMaxSet==false || pConf->mxMax > mxMax) && fileNum < 3)
    {
      mxMax=pConf->mxMax;
      xMaxSet = true;
    }
    if (fileNum==0)
    {
      for (int zLevel=0; zLevel<4; zLevel++)
      {
        if (pConf->mzStackExists[0][zLevel])
        {
          mTotalBottomZLevels++;
          mTotalZLevels++;
        }
      }
      for (int zLevel=0; zLevel<4; zLevel++)
      {
        if (pConf->mzStackExists[1][zLevel])
        {
          mTotalTopZLevels++;
          mTotalZLevels++;
        }
      }
    }
  }

  std::sort(mConf.begin(), mConf.end(), JpgFileXYSortForXAdj());
//  std::cout << "!!!!!!!!!!!!!!!! xMax (of all)=" << xMax << " yMax (of all)=" << yMax << std::endl;
  
  //*******************************************************************
  // Find the pyramid level lowest zoom and set that as current image
  //*******************************************************************
  int level=-1;
  for (int min=3; min>=0; min--)
  {
    if (mConf[min]->mfound==true)
    {
      level=min;
      break;
    }
  }
  if (level==-1)
  {
    logFile << "File has no readable levels." << std::endl;
    mValidObject = false;
    logFile.close();
    return false;
  }
  
  //****************************************************************
  // Guess the total image width and height for each pyramid level
  //****************************************************************
  double multiX[3] = { 1.0, 1.0, 1.0 };
  double multiY[3] = { 1.0, 1.0, 1.0 };
  
  if (mConf[3]->mfound && mConf[2]->mfound)
  {
    multiX[2] = mConf[2]->mxAdj / mConf[3]->mxAdj;
    multiY[2] = mConf[2]->myAdj / mConf[3]->myAdj;
  }
  if (mConf[2]->mfound && mConf[1]->mfound)
  {
    multiX[1] = mConf[2]->mxAdj / mConf[1]->mxAdj;
    multiY[1] = mConf[2]->myAdj / mConf[1]->myAdj;
  }
  else if (mConf[3]->mfound && mConf[1]->mfound)
  {
    multiX[1] = mConf[3]->mxAdj / mConf[1]->mxAdj;
    multiY[1] = mConf[3]->myAdj / mConf[1]->myAdj;
  }
  if (mConf[0]->mfound)
  {
    if (mConf[2]->mfound)
    {
      multiX[0] = mConf[2]->mxAdj / mConf[0]->mxAdj;
      multiY[0] = mConf[2]->myAdj / mConf[0]->myAdj;
    }
    else if (mConf[3]->mfound)
    {
      multiX[0] = mConf[3]->mxAdj / mConf[0]->mxAdj;
      multiY[0] = mConf[3]->myAdj / mConf[0]->myAdj;
    }
    else if (mConf[1]->mfound)
    {
      multiX[0] = mConf[1]->mxAdj / mConf[0]->mxAdj;
      multiY[0] = mConf[1]->myAdj / mConf[0]->myAdj;
    }
  }
  
  int highestLvl;
  for (highestLvl = 3; highestLvl >= 1 && mConf[highestLvl]->mfound==false; highestLvl--);

  if (mConf[2]->mfound)
  {
    mConf[2]->mtotalWidth = (double)(mConf[2]->mxMax - mConf[2]->mxMin) / (double) mConf[2]->mxAdj;
    mConf[2]->mtotalHeight = (double)(mConf[2]->myMax - mConf[2]->myMin) / (double) mConf[2]->myAdj;

    mConf[3]->mtotalWidth = mConf[2]->mtotalWidth * multiX[2];
    mConf[3]->mtotalHeight = mConf[2]->mtotalHeight * multiY[2];

    mConf[1]->mtotalWidth = mConf[2]->mtotalWidth * multiX[1];
    mConf[1]->mtotalHeight = mConf[2]->mtotalHeight * multiY[1];

    mConf[0]->mtotalWidth = mConf[2]->mtotalWidth * multiX[0];
    mConf[0]->mtotalHeight = mConf[2]->mtotalHeight * multiY[0];
  }
  else if (mConf[3]->mfound)
  {
    mConf[3]->mtotalWidth = (double)(mConf[3]->mxMax - mConf[3]->mxMin) / (double) mConf[3]->mxAdj;
    mConf[3]->mtotalHeight = (double)(mConf[3]->myMax - mConf[3]->myMin) / (double) mConf[3]->myAdj;

    mConf[1]->mtotalWidth = mConf[3]->mtotalWidth * multiX[1];
    mConf[1]->mtotalHeight = mConf[3]->mtotalHeight * multiY[1];

    mConf[0]->mtotalWidth = mConf[3]->mtotalWidth * multiX[0];
    mConf[0]->mtotalHeight = mConf[3]->mtotalHeight * multiY[0];
  }

  if (highestLvl==1)
  {
    mConf[1]->mtotalWidth = (double)(mConf[1]->mxMax - mConf[1]->mxMin) / (double) mConf[1]->mxAdj;
    mConf[1]->mtotalHeight = (double)(mConf[1]->myMax - mConf[1]->myMin) / (double) mConf[1]->myAdj;

    mConf[0]->mtotalWidth = mConf[1]->mtotalWidth * multiX[0];
    mConf[0]->mtotalHeight = mConf[1]->mtotalHeight * multiY[0];
  }
  else if (highestLvl==0)
  {
    mConf[0]->mtotalWidth = (double)(mConf[0]->mxMax - mConf[0]->mxMin) / (double) mConf[0]->mxAdj;
    mConf[0]->mtotalHeight = (double)(mConf[0]->myMax - mConf[0]->myMin) / (double) mConf[0]->myAdj;
  }
  // log file width and height
  for (int fileNum=0; fileNum < 4; fileNum++)
  {
    logFile << "fileName=" << mConf[fileNum]->mname << " totalWidth in pixels=" << mConf[fileNum]->mtotalWidth << " totalHeight in pixels=" << mConf[fileNum]->mtotalHeight << std::endl;
  }

  IniConf* pHigherConf = NULL;
  IniConf* pLowerConf = NULL;
  int higherLevel = -1;
  int lowerLevel = -1;
  bool higherLevelFound = false, lowerLevelFound = false;
  double higherLevelXDiv, higherLevelYDiv;
  //int zoomedXDetail, zoomedYDetail;
  if (mConf[2]->mfound)
  {
    pHigherConf = mConf[2];
    higherLevelFound = true;
    higherLevel = 2;
  }
  else if (mConf[3]->mfound)
  {
    pHigherConf = mConf[3];
    higherLevelFound = true;
    higherLevel = 3;
  }
  if (mConf[1]->mfound)
  {
    pLowerConf = mConf[1];
    lowerLevel = 1;
    lowerLevelFound = true;
  }
  else if (mConf[0]->mfound)
  {
    pLowerConf = mConf[0];
    lowerLevel = 0;
    lowerLevelFound = true;
  }
  if (lowerLevelFound && higherLevelFound)
  {
    higherLevelXDiv = pHigherConf->mxAdj / pLowerConf->mxAdj;
    higherLevelYDiv = pHigherConf->myAdj / pLowerConf->myAdj; 
    // zoomedXDetail = pLowerConf->detailedWidth / higherLevelXDiv;
    // zoomedYDetail = pLowerConf->detailedHeight / higherLevelYDiv;
  } 
  //*****************************************************************
  // Calculate the x and y coordinate of higherLevels 
  //*****************************************************************
  for (int fileNum=2; fileNum<4; fileNum++)
  {
    IniConf* pConf=mConf[fileNum];
    if (pConf->mfound==false) continue;
     
    for (int i=0; i<pConf->mtotalTiles; i++)
    {
      double xPixel=((double)(pConf->mxMax - pConf->mxyArr[i].mx)/(double)pConf->mxAdj);
      pConf->mxyArr[i].mxPixel=(int)lround(xPixel);
      
      double yPixel=((double)(pConf->myMax - pConf->mxyArr[i].my)/(double)pConf->myAdj);
      pConf->mxyArr[i].myPixel=(int)lround(yPixel);

      logFile << "filename=" << pConf->mxyArr[i].mBaseFileName << " x=" << xPixel << " y=" << yPixel << std::endl;

      for (int zSplit=0; zSplit < 2; zSplit++)
      {
        for (int zLevel=0; zLevel<4; zLevel++)
        {
          if (pConf->mxyArr[i].mzStack[zSplit][zLevel])
          {
            logFile << "filename=" << pConf->mxyArr[i].mFileName[zSplit][zLevel] << " x=" << xPixel << " y=" << yPixel << std::endl;
          }
        }
      }
    }
    std::sort(pConf->mxyArr.begin(), pConf->mxyArr.end());
  }
  mValidObject = true;

  int bestXOffset0, bestXOffset1;
  int bestYOffset0, bestYOffset1;
  if (lowerLevelFound && higherLevelFound)
  {
    findXYOffset(lowerLevel, higherLevel, &bestXOffset0, &bestYOffset0, &bestXOffset1, &bestYOffset1, logFile);
  }
  //*****************************************************************
  // Calculate the x and y coordinate of each file starting pixels
  //*****************************************************************
  for (int fileNum=0; fileNum<2; fileNum++)
  {
    IniConf* pConf=mConf[fileNum];
    if (pConf->mfound==false) continue;
    pConf->mxSortedArr.resize(pConf->mxyArr.size());

    for (int i=0; i<pConf->mtotalTiles; i++)
    {
      double xPixel;
      xPixel=((double)(pConf->mxMax - pConf->mxyArr[i].mx)/(double)pConf->mxAdj);
      double yPixel;
      yPixel=((double)(pConf->myMax - pConf->mxyArr[i].my)/(double)pConf->myAdj);
      /*
      if (higherLevelFound)
      {
        xPixel += bestXOffset * (pHigherConf->mxAdj / pConf->mxAdj);
        yPixel += bestYOffset * (pHigherConf->myAdj / pConf->myAdj);
      }
      if (higherLevelFound && fileNum==0 && mConf[1]->mfound)
      {
        xPixel += (mConf[1]->mxMax - mConf[0]->mxMax) / pConf->mxAdj;
        yPixel += (mConf[1]->myMax - mConf[0]->myMax) / pConf->myAdj;
      }*/
      if (higherLevelFound && fileNum==0)
      {
        xPixel += bestXOffset0;
        yPixel += bestYOffset0;
      }  
      else if (higherLevelFound && fileNum==1)
      {
        xPixel += bestXOffset1;
        yPixel += bestYOffset1;
      }
      pConf->mxyArr[i].mxPixel=(int)xPixel;
      pConf->mxyArr[i].myPixel=(int)yPixel;
      pConf->mxSortedArr[i].mxPixel=(int)xPixel;
      pConf->mxSortedArr[i].myPixel=(int)yPixel;
      
      logFile << "filename=" << pConf->mxyArr[i].mBaseFileName << " x=" << xPixel << " y=" << yPixel << std::endl;
    }
    std::sort(pConf->mxyArr.begin(), pConf->mxyArr.end());
    std::sort(pConf->mxSortedArr.begin(), pConf->mxSortedArr.end(), JpgXYSortForX());
    for (int tileNum=0; tileNum< (int) pConf->mxyArr.size(); tileNum++)
    {
      for (int tileNum2=0; tileNum2< (int) pConf->mxyArr.size(); tileNum2++)
      {
        if (pConf->mxSortedArr[tileNum].mxPixel==pConf->mxyArr[tileNum2].mxPixel && pConf->mxyArr[tileNum2].myPixel==pConf->mxSortedArr[tileNum].myPixel)
        {
          pConf->mxyArr[tileNum2].mxSortedIndex = tileNum;
          break;
        }
      }
    }
  }

  mbaseWidth = mConf[0]->mtotalWidth;
  mbaseHeight = mConf[0]->mtotalHeight;
  //std::cout << "Total number of mConf: " << mConf.size() << std::endl;
  std::string previewFileName = inputDir;
  previewFileName += separator();
  previewFileName += "PreviewSlide.jpg";
  Jpg previewJpg;
  if (previewJpg.open(previewFileName))
  {
    IniConf *previewConf = new IniConf;
    previewConf->mpixelWidth = previewConf->mtotalWidth = previewJpg.getActualWidth();
    previewConf->mpixelHeight = previewConf->mtotalHeight = previewJpg.getActualHeight();
    previewConf->mtotalTiles = 1;
    logFile << " PreviewSlide.jpg found. Width=" << previewConf->mpixelWidth << " Height=" << previewConf->mpixelHeight << std::endl;
    jpgxy.mBaseFileName = previewFileName;
    jpgxy.mxPixel=0;
    jpgxy.myPixel=0;
    jpgxy.mx=0.0;
    jpgxy.my=0.0;
    previewConf->mfound = true;
    previewConf->mxyArr.push_back(jpgxy);
    previewConf->mIsPreviewSlide = true;
    mConf.push_back(previewConf);
    previewJpg.close();
  }
  else
  {
    std::cerr << "Warning: PreviewSlide.jpg not found." << std::endl;
  }
  logFile.close();
  return true;
}


bool CompositeSlide::findXYOffset(int lowerLevel, int higherLevel, int *bestXOffset0, int *bestYOffset0, int *bestXOffset1, int *bestYOffset1, std::fstream& logFile)
{
  double xMulti0 = mConf[2]->mxAdj / mConf[0]->mxAdj;
  double yMulti0 = mConf[2]->myAdj / mConf[0]->myAdj;
  double xMulti1 = mConf[2]->mxAdj / mConf[1]->mxAdj;
  double yMulti1 = mConf[2]->myAdj / mConf[1]->myAdj;
  double xMulti2 = mConf[1]->mxAdj / mConf[2]->mxAdj;
  double yMulti2 = mConf[1]->myAdj / mConf[2]->myAdj;
  int bestXOffset2, bestYOffset2;
  *bestXOffset0=(int) lround((((mConf[2]->mxMax - mConf[2]->mxMin) / mConf[2]->mxAdj) * xMulti0) - ((mConf[0]->mxMax - mConf[0]->mxMin) / mConf[0]->mxAdj));
  *bestYOffset0=(int) lround((((mConf[2]->myMax - mConf[2]->myMin) / mConf[2]->myAdj) * yMulti0) - ((mConf[0]->myMax - mConf[0]->myMin) / mConf[0]->myAdj));
  *bestXOffset1=(int) lround((((mConf[2]->mxMax - mConf[2]->mxMin) / mConf[2]->mxAdj) * xMulti1) - ((mConf[1]->mxMax - mConf[1]->mxMin) / mConf[1]->mxAdj));
  *bestYOffset1=(int) lround((((mConf[2]->myMax - mConf[2]->myMin) / mConf[2]->myAdj) * yMulti1) - ((mConf[1]->myMax - mConf[1]->myMin) / mConf[1]->myAdj));
  bestXOffset2=(int)((double) (*bestXOffset1 * xMulti2));
  bestYOffset2=(int)((double) (*bestYOffset1 * yMulti2));
  IniConf *pLowerConf = mConf[lowerLevel];
  IniConf *pHigherConf = mConf[higherLevel];
  double xZoomOut = pHigherConf->mxAdj / pLowerConf->mxAdj;
  double yZoomOut = pHigherConf->myAdj / pLowerConf->myAdj;
  int simulatedWidth = (int) lround((double)pLowerConf->mdetailedWidth / xZoomOut);
  int simulatedHeight = (int) lround((double)pLowerConf->mdetailedHeight / yZoomOut);
  //cv::Mat imgTest = cv::imread(pLowerConf->mxyArr[0].mFileName[0], CV_LOAD_IMAGE_COLOR); 
  //simulatedWidth += imgTest.cols / xZoomOut;
  //simulatedHeight += imgTest.rows / yZoomOut;

  logFile << "simulatedWidth=" << simulatedWidth << " simulatedHeight=" << simulatedHeight << std::endl;
  cv::Mat imgComplete1(simulatedHeight, simulatedWidth, CV_8UC3, cv::Scalar(255,255,255));
  logFile << "Reading level " << lowerLevel << " and scaling..." << std::endl;
  std::cout << "Reading level " << lowerLevel << " and scaling..." << std::endl;
  border.clear();
  for (int i=0; i<pLowerConf->mtotalTiles; i++)
  {
    cv::Mat imgPart = cv::imread(pLowerConf->mxyArr[i].mBaseFileName, CV_LOAD_IMAGE_COLOR); 
    Pt ptNow, ptStart;
    ptNow.x=0;
    ptNow.y=0;
    if (imgPart.total()>0)
    {
      //************************************************************
      // Find the left and right borders
      //************************************************************
      for (int y=0; y < imgPart.rows; y++)
      {
        int offset=y * imgPart.cols * 3;
        ptStart.x=0;
        ptStart.y=y;
        int xcount = 0;
        bool leftBorderSet=false;
        bool rightBorderSet=false;
        bool skipStartPush=true;
        for (int x=0; x < imgPart.cols; x++, offset += 3)
        {
          if (imgPart.data[offset] == 255 && imgPart.data[offset+1] == 255 && imgPart.data[offset+2] == 255)
          {
            if (xcount==0)
            {
              ptStart.x = x;
              ptStart.y = y;
            }
            xcount++;
            if (xcount > 2)
            {
              ptNow.x = x;
              ptNow.y = y;
            }
            if (x == imgPart.cols-1 && )
            {

            }
          }
          else
          {
            if (xcount > 2)
            {
              if (skipStartPush==true && xcount==x-1)
              {
                skipStartPush=false;
              }
              else
              {
                pLowerConf->mxyArr[i].border.push_back(ptStart);
              }
              pLowerConf->mxyArr[i].border.push_back(ptNow);
              xcount = 0;
            }
            else if (pLowerConf->mxyArr[i].mx==pLowerConf->mxMax && x < 3 && leftBorderSet==false)
            {
              leftBorderSet=true;
              ptNow.x = x;
              ptNow.y = y;
              pLowerConf->mxyArr[i].border.push_back(ptNow);
            }
            if (pLowerConf->mxyArr[i].mx==pLowerConf->mxMin && x < 
          }
        }
      }
      //************************************************************
      // Find the top and bottom borders
      //************************************************************
      for (int x=0; x < imgPart.cols; x++)
      {
        ptStart.x=x;
        ptStart.y=0;
        int ycount = 0;
        bool topBorderSet=false;
        bool skipStartPush=true;
        for (int y=0; y < imgPart.rows; y++)
        {
          int offset=(y * imgPart.cols * 3) + (x * 3);
          if (imgPart.data[offset] == 255 && imgPart.data[offset+1] == 255 && imgPart.data[offset+2] == 255)
          {
            if (ycount==0)
            {
              ptStart.x = x;
              ptStart.y = y;
            }
            ycount++;
            if (ycount > 2)
            {
              ptNow.x = x;
              ptNow.y = y;
            }
          }
        }
        else
        {
          if (ycount > 2)
          {
            if (skipStartPush==true && ycount==y-1)
            {
              skipStartPush=false;
            }
            else
            {
              pLowerConf->mxyArr[i].border.push_back(ptStart);
            }
            pLowerConf->mxyArr[i].border.push_back(ptNow);
            ycount = 0;
          }
          else if (pLowerConf->mxyArr[i].mx==pLowerConf->mxMin && x < 3 && leftBorderSet==false)
          {
            leftBorderSet=true;
            ptNow.x = x;
            ptNow.y = y;
            pLowerConf->mxyArr[i].border.push_back(ptNow);
          }
        }
      }
 
      cv::Size scaledSize((int)lround(imgPart.cols / xZoomOut), (int)lround(imgPart.rows / yZoomOut));
      cv::Mat imgScaled((int)scaledSize.width, (int)scaledSize.height, CV_8UC3, cv::Scalar(255,255,255));
      cv::resize(imgPart, imgScaled, scaledSize);

      double xPixel=((double)((double)(pLowerConf->mxMax - pLowerConf->mxyArr[i].mx)/pLowerConf->mxAdj)/xZoomOut);
      double yPixel=((double)((double)(pLowerConf->myMax - pLowerConf->mxyArr[i].my)/pLowerConf->myAdj)/yZoomOut);
      int xPixelInt = (int) xPixel;
      if (xPixelInt > 0) xPixelInt--;
      int yPixelInt = (int) yPixel;
      if (yPixelInt > 0) yPixelInt--;
      // std::cout << "xPixel=" << xPixelInt << " yPixel=" << yPixelInt << " scaledSize.width=" << scaledSize.width << " scaledSize.height=" << scaledSize.height << std::endl;
      if (xPixelInt + scaledSize.width <= imgComplete1.cols && yPixelInt + scaledSize.height <= imgComplete1.rows)
      {
        cv::Mat imgRoi(imgComplete1, cv::Rect(xPixelInt, yPixelInt, (int)scaledSize.width, (int)scaledSize.height)); 
        imgScaled.copyTo(imgRoi);
        imgRoi.release();
      }
      else
      {
        std::cerr << "Warning: ROI outside of image boundaries: xPixel: " << xPixelInt << " width: " << scaledSize.width << " > " << imgComplete1.cols;
        std::cerr << " yPixel: " << yPixelInt << " height: " << scaledSize.height << " > " << imgComplete1.rows << std::endl;
      }
    }
    imgPart.release();
  }
  //cv::imshow("Scaled Image", imgComplete1);

  cv::Mat imgComplete2((int)pHigherConf->mdetailedHeight, (int)pHigherConf->mdetailedWidth, CV_8UC3, cv::Scalar(255,255,255));
  //std::cout << "imgComplete2 width " << pHigherConf->detailedWidth << " height: " << pHigherConf->detailedHeight,
  logFile << "Reading level " << higherLevel << " and scaling." << std::endl;
  for (int i=0; i<pHigherConf->mtotalTiles; i++)
  {
    cv::Mat imgPart = cv::imread(pHigherConf->mxyArr[i].mBaseFileName, CV_LOAD_IMAGE_COLOR); 
    double xPixel=((double)(pHigherConf->mxMax - pHigherConf->mxyArr[i].mx)/(double)pHigherConf->mxAdj);
    double yPixel=((double)(pHigherConf->myMax - pHigherConf->mxyArr[i].my)/(double)pHigherConf->myAdj);
    int xPixelInt = (int) xPixel;
    if (xPixelInt > 0) xPixelInt--;
    int yPixelInt = (int) yPixel;
    if (yPixelInt > 0) yPixelInt--;
    //std::cout << "xPixel=" << xPixelInt << " yPixel=" << yPixelInt << " width=" << imgPart.cols << " height=" << imgPart.rows << std::endl;
    if (xPixelInt + imgPart.cols <= imgComplete2.cols && yPixelInt + imgPart.rows <= imgComplete2.rows)
    {
      cv::Rect roi(xPixelInt, yPixelInt, imgPart.cols, imgPart.rows);
      cv::Mat imgRoi = imgComplete2(roi); 
      imgPart.copyTo(imgRoi);
      imgRoi.release();
    }
    else
    {
      std::cerr << "Warning: ROI outside of image boundaries: xPixel: " << xPixelInt << " width: " << imgPart.cols << " > " << imgComplete2.cols;
      std::cerr << " yPixel: " << yPixelInt << " height: " << imgPart.rows << " > " << imgComplete2.rows << std::endl;
    }
    imgPart.release();
  }
  //------------------------------------------------------------------------
  // AKAZE
  //------------------------------------------------------------------------
  std::cout << "Finding unique image descriptors in lower and higher levels ..." << std::endl;
  cv::Ptr<cv::AKAZE> akaze_detector = cv::AKAZE::create();
  std::vector<cv::KeyPoint> keypoints1, keypoints2;
  cv::Mat descriptors1, descriptors2;

  akaze_detector->detectAndCompute(imgComplete1, cv::noArray(), keypoints1, descriptors1);
  akaze_detector->detectAndCompute(imgComplete2, cv::noArray(), keypoints2, descriptors2);

  std::cout << "Running descriptor matcher..." << std::endl;
  cv::BFMatcher bfMatcher(cv::NORM_HAMMING);
//    std::vector<std::vector<cv::DMatch> > nn_matches;
//    matcher.knnMatch(descriptors1, descriptors2, nn_matches, 2);
  std::vector<cv::DMatch> matches;
  bfMatcher.match(descriptors1, descriptors2, matches);

  //std::vector< cv::DMatch > good_matches2;
  //std::vector< cv::KeyPoint > matched1, matched2;
  
  //double nn_match_ratio = 0.8;
  cv::imwrite("imgComplete1.jpg", imgComplete1);
  cv::imwrite("imgComplete2.jpg", imgComplete2);
  imgComplete1.release();
  imgComplete2.release();
  
  std::vector<double> diffXs;
  std::vector<double> diffYs;
  std::sort(matches.begin(), matches.end(), CVMatchCompare());
  for (size_t i = 0; i < matches.size(); i++)
  { 
    double diffX = keypoints2[matches[i].trainIdx].pt.x - keypoints1[matches[i].queryIdx].pt.x;
    double diffY = keypoints2[matches[i].trainIdx].pt.y - keypoints1[matches[i].queryIdx].pt.y;
    if (diffX >= 0 && diffY >= 0)
    {
      diffXs.push_back(diffX);
      diffYs.push_back(diffY);
    }
  }
  int medianIndex = 0;
  int bestXOffset, bestYOffset;
  if (diffXs.size()>0)
  {
    //std::sort(diffXs.begin(), diffXs.end());
    //std::sort(diffYs.begin(), diffYs.end());
    //int medianIndex = diffXs.size()/2;
    bestXOffset = diffXs[0];
    bestYOffset = diffYs[0];
  }
  else
  {
    bestXOffset = 0;
    bestYOffset = 0;
  }
  logFile << "Diff X Vector Size: " << diffXs.size() << std::endl;
  logFile << "Best (First in sorted arrays) X, Y alignment: " << bestXOffset << " " << bestYOffset << std::endl;

  *bestXOffset0 = (bestXOffset * xMulti0) + ((mConf[1]->mxMax - mConf[0]->mxMax) / mConf[0]->mxAdj);
  *bestYOffset0 = (bestYOffset * yMulti0) + ((mConf[1]->myMax - mConf[0]->myMax) / mConf[0]->myAdj);
  *bestXOffset1 = bestXOffset * xMulti1;
  *bestYOffset1 = bestYOffset * yMulti1;
 
  //-- Draw only "good" matches
  /*
  cv::Mat img_matches;
  cv::drawMatches( imgComplete1, keypoints1, imgComplete2, keypoints2,
               good_matches2, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1),
               std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

  //-- Show detected matches
  imshow( "Good Matches", img_matches );

  for( int i = 0; i < (int)good_matches.size(); i++ )
  { 
    printf( "-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx ); 
    printf( "-- Average distances x: %i  y: %i", bestXOffset, bestYOffset);
  }
  cv::waitKey(0);
  */
  
  return true;
}


bool CompositeSlide::read(int x, int y, int width, int height, bool setGrayScale)
{
  return false;
}


BYTE* CompositeSlide::allocate(int level, int x, int y, int width, int height, bool setGrayScale)
{
  if (mValidObject==false || level<0 || level > (int) mConf.size() || mConf[level]->mfound==false)
  {
    return 0;
  }
  int actualWidth=mConf[level]->mtotalWidth;
  int actualHeight=mConf[level]->mtotalHeight;
  if (x>actualWidth || x<0 || y>actualHeight || y<0)
  {
    std::cerr << "x or y out of bounds: x=" << x << " y=" << y;
    return 0;
  }
  if (width <= 0 || height <= 0)
  {
    std::cerr << "width or height out of bounds: width=" << width << " height=" << height;
    return 0;
  } 
  int samplesPerPixel = 3;
  if (setGrayScale || mGrayScale)
  {
    samplesPerPixel = 1;
  }  
  int maxWidth=width;
  int maxHeight=height;
  if (x+width>actualWidth)
  {
    maxWidth=actualWidth-x;
  }
  if (y+height>actualHeight)
  {
    maxHeight=actualHeight-y;
  }
 
  int bmpSize=maxWidth*maxHeight*samplesPerPixel;
  BYTE *pBmp=new BYTE[bmpSize];
  if (pBmp)
  {
    memset(pBmp, mbkgColor, bmpSize);
  }
  return pBmp;
}
 

bool CompositeSlide::read(BYTE *pBmp, int level, int direction, int zLevel, int x, int y, int width, int height, bool setGrayScale, int *pReadWidth, int *pReadHeight)
{
//  std::cout << "In CompositeSlide::read" << std::endl;
//  std::cout << " x=" << x << " y=" << y << " width=" << width << " height=" << height << std::endl;
  //int originalWidth=width;
  //int originalHeight=height;
  *pReadWidth = 0;
  *pReadHeight = 0;
  if (checkZLevel(level, direction, zLevel)==false)
  {
    return false;
  }
  int actualWidth = mConf[level]->mtotalWidth;
  int actualHeight = mConf[level]->mtotalHeight;
  if (x>actualWidth || x<0 || y>actualHeight || y<0)
  {
    std::cerr << "x or y out of bounds: x=" << x << " y=" << y;
    return false;
  }
  if (width <= 0 || height <= 0)
  {
    std::cerr << "width or height out of bounds: width=" << width << " height=" << height;
    return false;
  } 
  int samplesPerPixel = 3;
  if (setGrayScale || mGrayScale)
  {
    samplesPerPixel = 1;
    setGrayScale = true;
  }  
  int maxWidth=width;
  int maxHeight=height;
  if (x+width>actualWidth)
  {
    maxWidth=actualWidth-x;
  }
  if (y+height>actualHeight)
  {
    maxHeight=actualHeight-y;
  }
 
  int bmpSize=maxWidth*maxHeight*samplesPerPixel;
  IniConf* pConf=mConf[level];
  int fileWidth=pConf->mpixelWidth;
  int fileHeight=pConf->mpixelHeight;
  int widthGrab=0, heightGrab=0;
  int totalTilesRead=0;
  for (int tileNum=0; tileNum<pConf->mtotalTiles; tileNum++)
  {
    if (zLevel > 0 && direction > 0 && pConf->mxyArr[tileNum].mzStack[direction-1][zLevel] == false) continue;
    int xFilePos=pConf->mxyArr[tileNum].mxPixel;
    int yFilePos=pConf->mxyArr[tileNum].myPixel;
    if (((x<xFilePos && x+maxWidth>xFilePos) || (x>=xFilePos && x<xFilePos+fileWidth)) &&
        ((y<yFilePos && y+maxHeight>yFilePos) || (y>=yFilePos && y<yFilePos+fileHeight)))
    {
      Jpg *pjpg;
      int xRead=0;
      int xWrite=xFilePos-x;
      widthGrab=(x+maxWidth)-xFilePos;
      if (xWrite<0)
      {
        xWrite=0;
        xRead=x-xFilePos;
        widthGrab=fileWidth-xRead;
        if (widthGrab>maxWidth)
        {
          widthGrab=maxWidth;
        }
      }
      int yRead=0;
      int yWrite=yFilePos-y;
      heightGrab=(y+maxHeight)-yFilePos;
      if (yWrite<0)
      {
        yWrite=0;
        yRead=y-yFilePos;
        heightGrab=fileHeight-yRead;
        if (heightGrab>maxHeight)
        {
          heightGrab=maxHeight;
        }
      }
      if (yRead+heightGrab>fileHeight)
      {
        heightGrab=fileHeight-yRead;
      }
      if (xRead+widthGrab>fileWidth)
      {
        widthGrab=fileWidth-xRead;
      }
      /*
      if (level==2 && y < 32)
      {
        std::cout << "Filename to open: " << pConf->mxyArr[tileNum].mFileName[0] << " xFilePos: " << xFilePos << " yFilePos: " << yFilePos << " widthGrab: " << widthGrab << " heightGrab: " << heightGrab << " xRead, xWrite: " << xRead << ", " << xWrite << " yRead, yWrite: " << yRead << ", " << yWrite << std::endl;
      }
      */
      std::string& fileName=(direction > 0 ? pConf->mxyArr[tileNum].mFileName[direction-1][zLevel] : pConf->mxyArr[tileNum].mBaseFileName);
      pjpg=jpgCache.open(fileName, setGrayScale);
      if (pjpg->isValidObject() && pjpg->read(xRead, yRead, widthGrab, heightGrab))
      {
        int jpgCX=pjpg->getReadWidth();
        int jpgCY=pjpg->getReadHeight();
        int jpgSamplesPerPixel=pjpg->getSamplesPerPixel();
        BYTE *jpgBitmap=pjpg->bitmapPointer();
        for (int row=0; row<jpgCY; row++)
        {
      //    std::cout << "read bytes: " << parsedHeight+parsedWidth+(row*maxWidth*3) << std::endl;
      //    std::cout << "jpgBitmap=" << (long long) jpgBitmap << std::endl;
          int desti=(yWrite*maxWidth*samplesPerPixel)+(xWrite*samplesPerPixel)+(row*maxWidth*samplesPerPixel);
          if (desti+(jpgCX*samplesPerPixel) > bmpSize)
          {
            std::cerr << "In CompositeSlide::read, pointer out of bounds: bmpSize=" << bmpSize << " desti=" << desti << std::endl;
          }
          else if (samplesPerPixel == jpgSamplesPerPixel)
          {
            memcpy(&pBmp[desti], &jpgBitmap[row*jpgCX*jpgSamplesPerPixel], jpgCX*jpgSamplesPerPixel);
          }
          else if (samplesPerPixel == 3 && jpgSamplesPerPixel == 1)
          {
            // TODO Convert grayscale jpg to color bitmap if color bitmap was requested
            std::cerr << "Conversion from grayscale to color bitmap not supported yet!" << std::endl;

          }
        }
        totalTilesRead++;
        if (level==2)
        {
          drawBorder(pBmp, samplesPerPixel, x, y, maxWidth, maxHeight, level); 
        }
      }
      else
      {
        std::string errMsg;
        pjpg->getErrMsg(errMsg);
        std::cerr << "Warning: failed to read " << fileName << ": " << errMsg << std::endl;
        jpgCache.release(pjpg);
      }
    }
  }
  *pReadWidth=maxWidth;
  *pReadHeight=maxHeight;
  return true;
}


bool CompositeSlide::drawBorder(BYTE *pBmp, int samplesPerPixel, int x, int y, int width, int height, int level)
{
  IniConf *pHigherConf=mConf[level];
  //std::ofstream logFile("drawBorder.log", std::ios::out | std::ofstream::app);
  
  //logFile << " drawBorder x: " << x << " y: " << y << " width: " << width << " height: " << height << " level: " << level << " samplesPerPixel: " << samplesPerPixel << std::endl;

  int srcLevel=-1;
  for (int curLevel=level-1; curLevel>=0 && srcLevel==-1; curLevel--)
  {
    if (mConf[curLevel]->mfound)
    {
      srcLevel=curLevel;
    }
  }
  if (srcLevel==-1)
  {
    return false;
  }
  IniConf *pLowerConf=mConf[srcLevel];

  double xZoomOut = pHigherConf->mxAdj / pLowerConf->mxAdj;
  double yZoomOut = pHigherConf->myAdj / pLowerConf->myAdj;
  int fileWidth=(int) lround(pLowerConf->mpixelWidth / xZoomOut);
  int fileHeight=(int) lround(pLowerConf->mpixelHeight / yZoomOut);
  int xTolerance=(int)(((double)fileWidth) * 1);
  int yTolerance=(int)(((double)fileHeight) * 1);
  int xLastPos=-1,yLastPos=-1;
  int xNextPos=-1,yNextPos=-1;
  for (int tileNum=0; tileNum<pLowerConf->mtotalTiles; tileNum++)
  {
    int xCurrentPos=(int) lround(pLowerConf->mxyArr[tileNum].mxPixel / xZoomOut);
    int yCurrentPos=(int) lround(pLowerConf->mxyArr[tileNum].myPixel / yZoomOut);
    // first check if the x y coordinates are within the region of the bitmap
    
    if (((x<xCurrentPos && x+width>xCurrentPos) || (x>=xCurrentPos && x<xCurrentPos+fileWidth)) &&
       ((y<yCurrentPos && y+height>yCurrentPos) || (y>=yCurrentPos && y<yCurrentPos+fileHeight)))
    {
      if (tileNum+1<pLowerConf->mtotalTiles)
      {
        xNextPos=(int) lround(pLowerConf->mxyArr[tileNum+1].mxPixel / xZoomOut);
      }
      else
      {
        xNextPos = -1;
      }
      
      int xIndex=pLowerConf->mxyArr[tileNum].mxSortedIndex;
      if (xIndex<=0)
      {
        yLastPos = -1;
      }
      else
      {
        yLastPos = (int) lround(pLowerConf->mxSortedArr[xIndex-1].myPixel / yZoomOut);
      }
      if (xIndex+1< pLowerConf->mtotalTiles)
      {
        yNextPos = (int) lround(pLowerConf->mxSortedArr[xIndex+1].myPixel / yZoomOut);
      }
      else
      {
        yNextPos = -1;
      }
      // check to see if we draw a left border
      if ((xLastPos==-1 || ((xLastPos+fileWidth)-xCurrentPos) > xTolerance) && x < xCurrentPos+8)
      {
        int yWrite1=yCurrentPos-y;
        int yWrite2=yWrite1+fileHeight;
        if (yWrite1 < 0) 
        {
          yWrite2=fileHeight+yWrite1;
          yWrite1=0;
        }
        if (yWrite2 > height)
        {
          yWrite2=height;
        }
        int xLineMark=xCurrentPos-x;
        int thickness;
        if (xLineMark < 0)
        {
          thickness=xLineMark+8;
          xLineMark=0;
        }  
        else
        {
          thickness=8;
        }  
        if (xLineMark+thickness > width)
        {
          thickness=width - xLineMark;
        }
        //logFile << " YHighlight xLineMark: " << xLineMark << " yWrite1: " << yWrite1 << " yWrite2: " << yWrite2 << " width: " << width << " height: " << height << " thickness: " << thickness << std::endl;
        drawYHighlight(pBmp, samplesPerPixel, xLineMark, yWrite1, yWrite2, width, height, thickness, 1);
      }
      // check to see if we draw a right border
      if ((xNextPos==-1 || ((xCurrentPos+fileWidth)-xNextPos) > xTolerance) && x+width > xCurrentPos+fileWidth-8)
      {
        int yWrite1=yCurrentPos-y;
        int yWrite2=yWrite1+fileHeight;
        if (yWrite1 < 0) 
        {
          yWrite2=fileHeight+yWrite1;
          yWrite1=0;
        }
        if (yWrite2 > height)
        {
          yWrite2=height;
        }
        int thickness;
        int xLineMark=(xCurrentPos+fileWidth-8)-x;
        if (xLineMark<0)
        {
          thickness=xLineMark+8;
          xLineMark=0;
        }
        else
        {
          thickness=8;
        }  
        if (xLineMark+thickness > width)
        {
          thickness=width - xLineMark;
        }
        //logFile << " YHighlight xLineMark: " << xLineMark << " yWrite1: " << yWrite1 << " yWrite2: " << yWrite2 << " width: " << width << " height: " << height << " thickness: " << thickness << std::endl;
        drawYHighlight(pBmp, samplesPerPixel, xLineMark, yWrite1, yWrite2, width, height, thickness, 1);
      }
      // check to see if we draw a top border
      if ((yLastPos==-1 || ((yLastPos+fileHeight)-yCurrentPos) > yTolerance) && y < yCurrentPos+8)
      {
        int xWrite1=xCurrentPos-x;
        int xWrite2=xWrite1+fileWidth;
        if (xWrite1 < 0) 
        {
          xWrite2=fileWidth+xWrite1;
          xWrite1=0;
        }
        if (xWrite2>width)
        {
          xWrite2=width;
        }
        int thickness;
        int yLineMark=yCurrentPos-y;
        if (yLineMark < 0)
        {
          thickness=yLineMark + 8;
          yLineMark = 0;
        }
        else
        {
          thickness=8;
        }  
        if (yLineMark+thickness > height)
        {
          thickness=height - yLineMark;
        }
        //logFile << " XHighlight yLineMark: " << yLineMark << " xWrite1: " << xWrite1 << " xWrite2: " << xWrite2 << " width: " << width << " height: " << height << " thickness: " << thickness << std::endl;
        drawXHighlight(pBmp, samplesPerPixel, yLineMark, xWrite1, xWrite2, width, height, thickness, 1);
      }
      // check to see if we draw a bottom border
      if ((yNextPos==-1 || ((yCurrentPos+fileHeight)-yNextPos) > yTolerance) && y+height > yCurrentPos+fileHeight-8)
      {
        int xWrite1=xCurrentPos-x;
        int xWrite2=xWrite1+fileWidth;
        if (xWrite1 < 0) 
        {
          xWrite2=fileWidth+xWrite1;
          xWrite1=0;
        }
        if (xWrite2>width)
        {
          xWrite2=width;
        }
        int thickness;
        int yLineMark=(yCurrentPos+fileHeight-8)-y;
        if (yLineMark < 0)
        {
          thickness=yLineMark + 8;
          yLineMark = 0;
        }
        else
        {
          thickness=8;
        }
        if (yLineMark+thickness > height)
        {
          thickness=height - yLineMark;
        }  
        //logFile << " XHighlight yLineMark: " << yLineMark << " xWrite1: " << xWrite1 << " xWrite2: " << xWrite2 << " width: " << width << " height: " << height << " thickness: " << thickness << std::endl;
        drawXHighlight(pBmp, samplesPerPixel, yLineMark, xWrite1, xWrite2, width, height, thickness, 1);
      }
    }
    xLastPos=xCurrentPos;
  }
  return true;
}


bool drawXHighlight(BYTE *pBmp, int samplesPerPixel, int y1, int x1, int x2, int width, int height, int thickness, int position)
{
  if (x1 < 0 || x2 < 0 || y1 < 0)
  {
    std::cerr << "Warning: drawYHighlight: parameters out of bound!" << std::endl;
    std::cerr << " y1=" << y1 << " x1=" << x1 << " x2=" << x2 << " width=" << width << " height=" << height << std::endl;
    return false;
  }
  if (x1 > x2)
  {
    int x3=x1;
    x1=x2;
    x2=x3;
  }
  if (x2 > width)
  {
    std::cerr << "Warning: drawXHighlight: parameters out of bound!" << std::endl;
    std::cerr << " y1=" << y1 << " x1=" << x1 << " x2=" << x2 << " width=" << width << " height=" << height << std::endl;
    return false;
  }
  int bmpSize = width * height * samplesPerPixel;
  if (samplesPerPixel==3 || samplesPerPixel==4)
  {
    while (thickness > 0)
    {
      thickness--;
      int offset = (width * samplesPerPixel * (y1+(thickness*position))) + (x1 * samplesPerPixel);
      for (int x3=x1; x3 < x2 && offset+2 < bmpSize; x3++, offset+=samplesPerPixel)
      {
        pBmp[offset] = 0;
        pBmp[offset+1] = 0;
        pBmp[offset+2] = 0;
      }
    }
  }
  else if (samplesPerPixel==1)
  {
    while (thickness > 0)
    {
      thickness--;
      int offset = (width * (y1+(thickness*position))) + x1;
      for (int x3=x1; x3 < x2 && offset < bmpSize; x3++, offset++)
      {
        pBmp[offset] = 0;
      }
    }
  } 
  return true;
}


bool drawYHighlight(BYTE *pBmp, int samplesPerPixel, int x1, int y1, int y2, int width, int height, int thickness, int position)
{
  if (y1 < 0 || y2 < 0 || x1 < 0)
  {
    std::cerr << "Warning: drawYHighlight: parameters out of bound!" << std::endl;
    std::cerr << " x1=" << x1 << " y1=" << y1 << " y2=" << y2 << " width=" << width << " height=" << height << std::endl;
    return false;
  }
  if (y1 > y2)
  {
    int y3=y1;
    y1=y2;
    y2=y3;
  }
  if (y2 > height)
  {
    std::cerr << "Warning: drawYHighlight: parameters out of bound!" << std::endl;
    std::cerr << " x1=" << x1 << " y1=" << y1 << " y2=" << y2 << " width=" << width << " height=" << height << std::endl;
    return false;
  }
  int bmpSize=width * height * samplesPerPixel;
  if (samplesPerPixel==3 || samplesPerPixel==4)
  {
    while (thickness>0)
    {
      thickness--;
      for (int y3=y1; y3 < y2; y3++)
      {
        int offset = (width * samplesPerPixel * y3) + ((x1+(thickness*position)) * samplesPerPixel);
        if (offset+2 < bmpSize)
        {
          pBmp[offset] = 0;
          pBmp[offset+1] = 0;
          pBmp[offset+2] = 0;
        }
      }
    }
  }
  else if (samplesPerPixel==1)
  {
    while (thickness>0)
    {
      thickness--;
      for (int y3=y1; y3 < y2; y3++)
      {
        int offset = (width * y3) + x1+(thickness*position);
        if (offset < bmpSize)
        {
          pBmp[offset] = 0;
        }
      }
    }
  } 
  return true;
}



bool CompositeSlide::testHeader(BYTE* fileHeader, int length)
{
  std::string headerStr = (const char*) fileHeader;
  std::string header = "header";
  if (length >= 8 && headerStr.length() >= 8)
  {
    std::string chunk = headerStr.substr(1, 6);
//    std::cout << chunk << std::endl;
//    gchar* foldedChunkName = g_utf8_casefold((gchar*)chunk.c_str(), chunk.size());
//    gchar* foldedHeaderName = g_utf8_casefold((gchar*)header.c_str(), header.length());
    if (strcasecmp(chunk.c_str(), header.c_str())==0)
    {
      return true;
    }
  }
  return false;
}

