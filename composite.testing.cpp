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
#include <gtk/gtk.h>
#include <algorithm>
#include "imagesupport.h"
#include "jpegsupport.h"
#include "composite.h"

const char* CompositeSlide::iniNames[] = 
{ 
  "FinalScan.ini", "FinalCond.ini", 
  "SlideScan.ini", "SlideCond.ini" 
};

CompositeSlide::CompositeSlide() : Image()
{
  initialize();
  bkgColor = 255;
}


IniConf::IniConf()
{
  name="";
  found=false;
  xMin=0;
  xMax=0;
  xDiffMin=0;
  yMin=0;
  yMax=0;
  yDiffMin=0;
  xAdj=0.0d;
  yAdj=0.0d;
  totalTiles=0;
  xAxis=0;
  yAxis=0;
  pixelWidth=0;
  pixelHeight=0;
  totalWidth=0;
  totalHeight=0;
  yStepSize=0;
  xStepSize=0;
  isPreviewSlide=false;
}


bool CompositeSlide::isPreviewSlide()
{
  if (level != -1 && level < iniConf.size()) 
  {
    return iniConf[level].isPreviewSlide; 
  }
  return false;
}


void CompositeSlide::initialize()
{
  IniConf iniConfLocal;

  validObject = false;
  
  iniConf.clear();
  for (int i=0; i<4; i++)
  {
    iniConfLocal.name = iniNames[i];
    iniConf.push_back(iniConfLocal);
    std::cout << i << " ini conf initialized. listing: " << iniConf.size() << std::endl;
  }
  xMin=0;
  xMax=0;
  yMin=0;
  yMax=0;
  pBitmap = 0;
} 


void CompositeSlide::close()
{
  validObject = false;
  if (pBitmap != 0)
  {
    delete[] pBitmap;
    pBitmap = 0;
  }
  iniConf.clear();
}


bool CompositeSlide::setLevel(int newLevel)
{
  if (newLevel < 0 || newLevel > iniConf.size() || validObject == false) 
  {
    std::cerr << "Level " << newLevel << " out of bounds (total " << iniConf.size() << ") or not a valid object!" << std::endl;
    return false;
  }
  if (iniConf[newLevel].found == false) 
  {
    std::cerr << "Warning level=" << newLevel << " not found!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    return false;
  }
  level = newLevel;
  renderedWidth=actualWidth=iniConf[level].totalWidth;
  renderedHeight=actualHeight=iniConf[level].totalHeight;

  return true;
}


bool CompositeSlide::open(const std::string& srcFileName)
{
  JpegFileXY jpegxy;
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
  bool removeFileName=false;
  std::string inputDir = srcFileName;
  bool xStepSizeFound=false;
  bool yStepSizeFound=false;
  int xAxis=0, yAxis=0;
  int xOffset=0, yOffset=0;
  int magnifyLevels[] = { 1, 4, 16, 32 };
  if (pBitmap != 0)
  {
    close();
  }
  initialize();

  //gchar* foldedHeaderName = g_utf8_casefold((gchar*)headerStr.c_str(), headerStr.length());

  for (int i=0; i<4; i++)
  {
    size_t namePos=inputDir.find(iniNames[i]);
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
    IniConf* pIniConf = &iniConf[fileNum];
    std::string inputName = inputDir;
    inputName += separator();
    inputName += iniNames[fileNum];

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
              pIniConf->xyArr.push_back(jpegxy);
            }
            std::string chunkName=line.substr(1, rpos-1);
            //gchar* foldedChunkName = g_utf8_casefold((gchar*)chunkName.c_str(), chunkName.length());
            if (strcasecmp(headerStr.c_str(), chunkName.c_str())==0)
            {
              //    std::cout << "In header!" << std::endl;
              jpegxy.fileName="";
              nameFound = false;
              header = true;
            }
            else
            {
              logFile << "Input Dir=" << inputDir << std::endl;
              jpegxy.fileName=inputDir;
              jpegxy.fileName += separator();
              jpegxy.fileName += chunkName;
              jpegxy.fileName += ".jpg";
              nameFound = true;
              header = false;
              logFile << "Filename=" << jpegxy.fileName << std::endl;
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
              pIniConf->pixelWidth=atoi(width.c_str());
            }
            size_t heightPos=line.find(iImageHeight);
            if (heightPos != std::string::npos && heightPos+iImageHeight.length()+1 < line.length())
            {
              std::string height = line.substr(heightPos+iImageHeight.length()+1);
              pIniConf->pixelHeight=atoi(height.c_str());
            }
            size_t xStagePos=line.find(lXStageRef);
            if (xStagePos != std::string::npos && xStagePos+lXStageRef.length()+1<line.length())
            {
              std::string xStageSubStr = line.substr(xStagePos+lXStageRef.length()+1);
              pIniConf->xAxis=atoi(xStageSubStr.c_str());
              xAxis=pIniConf->xAxis;
            //  logFile << "!!xAxis " << pIniConf->xAxis << std::endl;
            }
            size_t yStagePos=line.find(lYStageRef);
            if (yStagePos != std::string::npos && yStagePos+lYStageRef.length()+1<line.length())
            {
              std::string yStageSubStr = line.substr(yStagePos+lYStageRef.length()+1);
              pIniConf->yAxis=atoi(yStageSubStr.c_str());
              yAxis=pIniConf->yAxis;
            //  logFile << "!!yAxis " << pIniConf->yAxis << std::endl;
            }
            size_t yStepPos = line.find(lYStepSize);
            if (yStepPos != std::string::npos && yStepPos+lYStepSize.length()+1<line.length())
            {
              std::string yStepSubStr = line.substr(yStepPos+lYStepSize.length()+1);
              pIniConf->yStepSize = atoi(yStepSubStr.c_str());
              if (fileNum==0 && pIniConf->yStepSize > 0) yStepSizeFound = true;
              std::cout << " fileNum=" << fileNum << " yStepSize=" << pIniConf->xStepSize << std::endl;
            }
            size_t xStepPos = line.find(lXStepSize);
            if (xStepPos != std::string::npos && xStepPos+lXStepSize.length()+1<line.length())
            {
              std::string xStepSubStr = line.substr(xStepPos+lXStepSize.length()+1);
              pIniConf->xStepSize = atoi(xStepSubStr.c_str());
              if (fileNum==0 && pIniConf->xStepSize > 0) xStepSizeFound = true;

              std::cout << " fileNum=" << fileNum << " xStepSize=" << pIniConf->xStepSize << std::endl;
            }
            size_t xOffsetPos = line.find(lXOffset);
            if (xOffsetPos != std::string::npos && xOffsetPos+lXOffset.length()+1<line.length())
            {
              std::string xOffsetSubStr = line.substr(xOffsetPos+lXOffset.length()+1);
              xOffset = atoi(xOffsetSubStr.c_str());
              std::cout << " xOffset=" << xOffset << std::endl;
            }
            size_t yOffsetPos = line.find(lYOffset);
            if (yOffsetPos != std::string::npos && yOffsetPos+lYOffset.length()+1<line.length())
            {
              std::string yOffsetSubStr = line.substr(yOffsetPos+lYOffset.length()+1);
              yOffset = atoi(yOffsetSubStr.c_str());
            }
          }
          std::string line2=line.substr(0, 2);
          if (line2=="x=")
          {
            std::string somenum=line.substr(2);
            jpegxy.x=atoi(somenum.c_str());
            if (header) 
            {
  //            pIniConf->xAxis=jpegxy.x;
              jpegxy.x=0;
            //  logFile << "!!xAxis " << pIniConf->xAxis << std::endl;
            }
            else
            {
              xFound=true;
            }
          }
          if (line2=="y=")
          {
          std::string somenum=line.substr(2);
          jpegxy.y=atoi(somenum.c_str());
          if (header) 
          {
//            pIniConf->yAxis=jpegxy.y;
            jpegxy.y=0;
          //  logFile << "!!yAxis " << pIniConf->yAxis << std::endl;
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
      pIniConf->xyArr.push_back(jpegxy);
    }
    iniFile.close();
  }
  
  
  yMin=0, yMax=0, xMin=0, xMax=0;
  bool yMinSet=false, xMaxSet=false, xMinSet=false, yMaxSet=false;
  for (int fileNum=0; fileNum < 4; fileNum++)
  {
    IniConf* pIniConf=&iniConf[fileNum];
    if (pIniConf->xyArr.size()==0) continue;

    pIniConf->totalTiles = pIniConf->xyArr.size();
    if (pIniConf->pixelWidth<=0 || pIniConf->pixelHeight<=0)
    {
      Jpeg jpeg;
      jpeg.setUnfilledColor(bkgColor);
      if (jpeg.open(pIniConf->xyArr[0].fileName))
      {
        pIniConf->pixelWidth=jpeg.getActualWidth();
        pIniConf->pixelHeight=jpeg.getActualHeight();
        jpeg.close();
      }
      else
      {
        std::string errMsg;
        jpeg.getErrMsg(errMsg);
        std::cerr << "Warning: failed to open " << pIniConf->xyArr[0].fileName << " do not have pixel width and height for source jpegs." << std::endl;
        std::cerr << "Failed to open " << pIniConf->xyArr[0].fileName << ": " << errMsg;
      }
    }
    logFile << "fileName=" << pIniConf->name << " jpegWidth=" << pIniConf->pixelWidth << " jpegHeight=" << pIniConf->pixelHeight << std::endl;
    pIniConf->found = true;
    
    //************************************************************************
    // Get the xmin and xmax values
    //************************************************************************
    std::sort(pIniConf->xyArr.begin(), pIniConf->xyArr.end(), sort_for_y());
    pIniConf->xMin = pIniConf->xyArr[0].x;
    pIniConf->xMax = pIniConf->xyArr[pIniConf->totalTiles-1].x;
    for (int i=0; i+1 < pIniConf->totalTiles; i++)
    {
      logFile << " Sorted: x=" << pIniConf->xyArr[i].x << " y=" << pIniConf->xyArr[i].y << std::endl;
      if (pIniConf->xyArr[i+1].x==pIniConf->xyArr[i].x)
      {
        int diff=pIniConf->xyArr[i+1].y - pIniConf->xyArr[i].y;
        if ((diff>0 && diff<pIniConf->yDiffMin) || (diff>0 && pIniConf->yDiffMin<1))
        {
          pIniConf->yDiffMin=diff;
        }
      }
    }

    //************************************************************************
    // Get the ymin and ymax values
    //************************************************************************
    std::sort(pIniConf->xyArr.begin(), pIniConf->xyArr.end(), sort_for_x());
    pIniConf->yMin=pIniConf->xyArr[0].y - pIniConf->yDiffMin;
    pIniConf->yMax=pIniConf->xyArr[pIniConf->totalTiles-1].y; // + pIniConf->yDiffMin;

    logFile << "fileName=" << pIniConf->name << " yDiffMin=" << pIniConf->yDiffMin << " yMin=" << pIniConf->yMin << " yMax=" << pIniConf->yMax << " yAxis=" << pIniConf->yAxis << std::endl;
    for (int i=0; i+1 < pIniConf->totalTiles; i++)
    {
      //logFile << " Sorted: x=" << pIniConf->xyArr[i].x << " y=" << pIniConf->xyArr[i].y << std::endl;
      if (pIniConf->xyArr[i+1].y==pIniConf->xyArr[i].y)
      {
        int diff=pIniConf->xyArr[i].x - pIniConf->xyArr[i+1].x;
        if ((diff>0 && diff<pIniConf->xDiffMin) || (diff>0 && pIniConf->xDiffMin<1)) 
        {
          pIniConf->xDiffMin=diff;
        }
      }
    }
    pIniConf->xMin -= pIniConf->xDiffMin;
    if (fileNum<2)
    {
      //pIniConf->xMin += xOffset; // Note try removing this!
      //pIniConf->yMin += yOffset;
    }

    logFile << "fileName=" << pIniConf->name << " xDiffMin=" << pIniConf->xDiffMin << " xMin=" << pIniConf->xMin << " xMax=" << pIniConf->xMax << " xAxis=" << pIniConf->xAxis << std::endl;
    if (pIniConf->pixelWidth>0) 
    {
      if (pIniConf->xStepSize>0)
      {
        pIniConf->xAdj = (double) pIniConf->xStepSize / (double) pIniConf->pixelWidth;
      }
      else
      {

        pIniConf->xAdj = (double) (iniConf[fileNum-1].xStepSize*4) / (double) pIniConf->pixelWidth;
        //pIniConf->xAdj = (double) (pIniConf->xDiffMin) / (double) pIniConf->pixelWidth;

      }
      logFile << "fileName=" << pIniConf->name << " Guessed xAdj=" << pIniConf->xAdj << std::endl;
    }
    if (pIniConf->pixelHeight>0)
    {
      if (pIniConf->yStepSize>0)
      {
        pIniConf->yAdj = (double) pIniConf->yStepSize / (double) pIniConf->pixelHeight;
      }
      else
      {
        pIniConf->yAdj = (double) (iniConf[fileNum-1].yStepSize*4) / (double) pIniConf->pixelHeight;
        //pIniConf->yAdj = (double) (pIniConf->yDiffMin) / (double) pIniConf->pixelHeight;
      }
      logFile << "fileName=" << pIniConf->name << " Guessed yAdj=" << pIniConf->yAdj << std::endl;
    }
    if ((yMinSet==false || pIniConf->yMin < yMin) && fileNum < 3)
    {
      yMin=pIniConf->yMin;
      yMinSet = true;
    }
    if ((yMaxSet==false || pIniConf->yMax > yMax) && fileNum < 3)
    {
      yMax=pIniConf->yMax;
      yMaxSet = true;
    }
    if ((xMinSet==false || pIniConf->xMin < xMin) && fileNum < 3)
    { 
      xMin=pIniConf->xMin;
      xMinSet = true;
    }
    if ((xMaxSet==false || pIniConf->xMax > xMax) && fileNum < 3)
    {
      xMax=pIniConf->xMax;
      xMaxSet = true;
    }
  }
  
//  std::cout << "!!!!!!!!!!!!!!!! xMax (of all)=" << xMax << " yMax (of all)=" << yMax << std::endl;
  
  //*******************************************************************
  // Find the pyramid level lowest zoom and set that as current image
  //*******************************************************************
  level=-1;
  for (int min=3; min>=0; min--)
  {
    if (iniConf[min].found==true)
    {
      level=min;
      break;
    }
  }
  if (level==-1)
  {
    logFile << "File has no readable levels." << std::endl;
    validObject = false;
    return false;
  }
  validObject = true;

  //****************************************************************
  // Guess the total image width and height for each pyramid level
  //****************************************************************
  for (int fileNum=0; fileNum < 4; fileNum++)
  {
    IniConf* pIniConf=&iniConf[fileNum];
    
    int widthNextSlide = iniConf[fileNum+1].xMax - iniConf[fileNum+1].xMin;
    int widthThisSlide = iniConf[fileNum].xMax - iniConf[fileNum].xMin;
    int slideWidthDiff = widthNextSlide-widthThisSlide;
    
    int heightNextSlide = iniConf[fileNum+1].yMax - iniConf[fileNum+1].yMin;
    int heightThisSlide = iniConf[fileNum].yMax - iniConf[fileNum].yMin;
    int slideHeightDiff = heightNextSlide-heightThisSlide;
 
/*    if (fileNum>1)
    {
      pIniConf->totalWidth=(pIniConf->xMax - pIniConf->xMin)/pIniConf->xAdj;
      pIniConf->totalHeight=(pIniConf->yMax - pIniConf->yMin)/pIniConf->yAdj;
    }
    else*/
    {
      pIniConf->totalWidth=(xMax - xMin)/pIniConf->xAdj;
      pIniConf->totalHeight=(yMax - yMin)/pIniConf->yAdj;
    }

    if (level==fileNum)
    {
      renderedWidth=actualWidth = pIniConf->totalWidth;
      renderedHeight=actualHeight = pIniConf->totalHeight;
    }
    logFile << " This Level Image Width=" << pIniConf->totalWidth << " Image Height=" << pIniConf->totalHeight << std::endl;
  }

  //logFile << "fileName=" << pIniConf->name << " Calculated from input yAdj=" << pIniConf->yAdj << std::endl;
  //iniConf[fileNum].totalWidth=iniConf[0].totalWidth/magnifyLevels[fileNum];
  //iniConf[fileNum].totalHeight=iniConf[0].totalHeight/magnifyLevels[fileNum];
  //logFile << " Stored Image Width=" << iniConf[fileNum].totalWidth << " Stored Image Height=" << iniConf[fileNum].totalHeight << std::endl;
 
  //*****************************************************************
  // Calculate the x and y coordinate of each file starting pixels
  //*****************************************************************
  for (int fileNum=0; fileNum<4; fileNum++)
  {
    IniConf* pIniConf=&iniConf[fileNum];
    if (pIniConf->found==false) continue;
    
    int widthNextSlide = iniConf[fileNum+1].xMax - iniConf[fileNum+1].xMin;
    int widthThisSlide = iniConf[fileNum].xMax - iniConf[fileNum].xMin;
    int slideWidthDiff = ((widthNextSlide-widthThisSlide)/2);
    
    int heightNextSlide = iniConf[fileNum+1].yMax - iniConf[fileNum+1].yMin;
    int heightThisSlide = iniConf[fileNum].yMax - iniConf[fileNum].yMin;
    int slideHeightDiff = ((heightNextSlide-heightThisSlide)/2);

    int slideWidthDiff2 = iniConf[fileNum+1].xMax - iniConf[fileNum].xMax - xOffset;
    int slideHeightDiff2 = iniConf[fileNum+1].yMax - iniConf[fileNum].yMax - yOffset;
    for (int i=0; i<pIniConf->totalTiles; i++)
    {
      double xPixel;

      //xPixel=(double)(xMax - pIniConf->xyArr[i].x)/(double)pIniConf->xAdj;
      if (fileNum>1) 
      {
        xPixel=(double)(pIniConf->xMax - pIniConf->xyArr[i].x)/(double)pIniConf->xAdj;
      }
      else
      {
        xPixel=(double)(pIniConf->xMax - pIniConf->xyArr[i].x + xOffset)/(double)pIniConf->xAdj;
      }
      //xPixel=(double)(xMax - pIniConf->xyArr[i].x)/(double)pIniConf->xAdj;
      pIniConf->xyArr[i].xPixel=(int)xPixel;
      
      double yPixel;
      //yPixel=(double)(yMax - pIniConf->xyArr[i].y)/(double)pIniConf->yAdj;
      if (fileNum>1)
      {
        yPixel=(double)(pIniConf->yMax - pIniConf->xyArr[i].y)/(double)pIniConf->yAdj;
      }
      else
      {
        yPixel=(double)(pIniConf->yMax - pIniConf->xyArr[i].y + yOffset)/(double)pIniConf->yAdj;
      }
      //yPixel=(double)(yMax - pIniConf->xyArr[i].y)/(double)pIniConf->yAdj;
      pIniConf->xyArr[i].yPixel=(int)yPixel;

      logFile << "filename=" << pIniConf->xyArr[i].fileName << " x=" << xPixel << " y=" << yPixel << std::endl;
    }
    std::sort(pIniConf->xyArr.begin(), pIniConf->xyArr.end());
  }
 
  std::sort(iniConf.begin(), iniConf.end(), sort_for_dimen());
  baseWidth = iniConf[0].totalWidth;
  baseHeight = iniConf[0].totalHeight;
  std::cout << "Total number of iniConf: " << iniConf.size() << std::endl;
  std::string previewFileName = inputDir;
  previewFileName += separator();
  previewFileName += "PreviewSlide.jpg";
  Jpeg previewJpeg;
  if (previewJpeg.open(previewFileName))
  {
    IniConf previewConf;
    previewConf.pixelWidth = previewConf.totalWidth = previewJpeg.getActualWidth();
    previewConf.pixelHeight = previewConf.totalHeight = previewJpeg.getActualHeight();
    previewConf.totalTiles = 1;
    logFile << " PreviewSlide.jpg found. Width=" << previewConf.pixelWidth << " Height=" << previewConf.pixelHeight << std::endl;
    jpegxy.fileName = previewFileName;
    jpegxy.xPixel=0;
    jpegxy.yPixel=0;
    jpegxy.x=0.0d;
    jpegxy.y=0.0d;
    previewConf.found = true;
    previewConf.xyArr.push_back(jpegxy);
    previewConf.isPreviewSlide = true;
    iniConf.push_back(previewConf);
    previewJpeg.close();
  }
  else
  {
    std::cerr << "Warning: PreviewSlide.jpg not found." << std::endl;
  }
  logFile.close();
  return true;
}


bool CompositeSlide::read(int x, int y, int width, int height)
{
  BYTE* pOldBitmap = pBitmap;
//  std::cout << "In CompositeSlide::read" << std::endl;
//  std::cout << " x=" << x << " y=" << y << " width=" << width << " height=" << height << std::endl;
  //int originalWidth=width;
  //int originalHeight=height;
  if (validObject==false) 
  {
    return false;
  }
  if (level==-1 || iniConf[level].found==false)
  {
    return false;
  }
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
  int bmpSize=width*height*3;
  pBitmap=new BYTE[bmpSize];
  memset(pBitmap, bkgColor, bmpSize);
  if (pOldBitmap != 0)
  {
    delete[] pOldBitmap;
  }
  if (x+width>actualWidth)
  {
    width=actualWidth-x;
  }
  if (y+height>actualHeight)
  {
    height=actualHeight-y;
  }
 
  IniConf* pIniConf=&iniConf[level];
  //if (x<pIniConf->xMin || x>pIniConf->xMax)
  //{
  //  readWidth=width;
  //  readHeight=height;
  //  return true;
  //}
  int fileWidth=pIniConf->pixelWidth;
  int fileHeight=pIniConf->pixelHeight;
  int widthGrab=0, heightGrab=0;
  int totalTilesRead=0;
  bool found=false;
  for (int tileNum=0; tileNum<pIniConf->totalTiles; tileNum++)
  {
    int xFilePos=pIniConf->xyArr[tileNum].xPixel;
    int yFilePos=pIniConf->xyArr[tileNum].yPixel;
    //std::cout << pIniConf->xyArr[tileNum].fileName << " xFilePos=" << xFilePos << " yFilePos=" << yFilePos << " x=" << x << " y=" << y << " grabWidth=" << widthGrab << " grabHeight=" << heightGrab << std::endl;
    if (((x<xFilePos && x+width>xFilePos) || (x>=xFilePos && x<xFilePos+fileWidth)) &&
        ((y<yFilePos && y+height>yFilePos) || (y>=yFilePos && y<yFilePos+fileHeight)))
    {
//      std::cout << "In Tile " << tileNum << std::endl;
      Jpeg jpeg;
      jpeg.setUnfilledColor(bkgColor);
      int xRead=0;
      int xWrite=xFilePos-x;
      widthGrab=(x+width)-xFilePos;
      if (xWrite<0)
      {
        xWrite=0;
        xRead=x-xFilePos;
        widthGrab=fileWidth-xRead;
        if (widthGrab>width)
        {
          widthGrab=width;
        }
      }
      /*
      if (widthGrab>fileWidth)
      {
        widthGrab=fileWidth;
      }
      */
      /* 
      if (xFilePos+widthGrab>x+width)
      {
        widthGrab=width;
      }
      */
      int yRead=0;
      int yWrite=yFilePos-y;
      heightGrab=(y+height)-yFilePos;
      if (yWrite<0)
      {
        yWrite=0;
        yRead=y-yFilePos;
        heightGrab=fileHeight-yRead;
        if (heightGrab>height)
        {
          heightGrab=height;
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
      //std::cout << "Filename to open: " << pIniConf->xyArr[tileNum].fileName << " xFilePos: " << xFilePos << " yFilePos: " << yFilePos << " widthGrab: " << widthGrab << " heightGrab: " << heightGrab << " xRead, xWrite: " << xRead << ", " << xWrite << " yRead, yWrite: " << yRead << ", " << yWrite << std::endl;
      if (jpeg.open(pIniConf->xyArr[tileNum].fileName)
       && jpeg.read(xRead, yRead, widthGrab, heightGrab))
      {
        int jpegCX=jpeg.getReadWidth();
        int jpegCY=jpeg.getReadHeight();
        for (int row=0; row<jpegCY; row++)
        {
          BYTE* jpegBitmap=jpeg.bitmapPointer();
      //    std::cout << "read bytes: " << parsedHeight+parsedWidth+(row*width*3) << std::endl;
          uint desti=(yWrite*width*3)+(xWrite*3)+(row*width*3);
          if (desti+(jpegCX*3) > bmpSize)
          {
            std::cerr << "In CompositeSlide::read, pointer out of bounds: bmpSize=" << bmpSize << " desti=" << desti << std::endl;
          }
          else
          {
            memcpy(&pBitmap[desti], &jpegBitmap[row*jpegCX*3], jpegCX*3);
          }
        }
        totalTilesRead++;
        jpeg.close();
      }
      else
      {
        std::string errMsg;
        jpeg.getErrMsg(errMsg);
        std::cerr << "Warning: failed to read " << pIniConf->xyArr[tileNum].fileName << ": " << errMsg << std::endl;
        jpeg.close();
      }
      found = true;
    }
  }
  readWidth=width;
  readHeight=height;
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

