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
#include <new>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <memory>
#include <stdexcept>
#include "imagesupport.h"
#include "tiffsupport.h"

using namespace TIFFLIB;
#define CVT(x)  (((x) * 255L) / ((1L<<16)-1))

/*
static int colorMapBitCount(int n, uint16 *r, uint16 *g, uint16 *b)
{
  while (n-- > 0) {
    if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
      return 16;
  }
  return 8;
}
*/

bool Tiff::testHeader(BYTE* header, int size)
{
  if (header[0] == 'I' && header[1] == 'I' && header[2] == 42 && header[3] == 0)
    return true;
  else if (header[0] == 'M' && header[1] == 'M' && header[2] == 0 && header[3] == 42)
    return true;
  else
    return false;
}


bool Tiff::writeEncodedTile(BYTE* buff, unsigned int x, unsigned int y, unsigned int z)
{
  if (mtif)
  {
    ttile_t tile = TIFFComputeTile(mtif, x, y, z, 0);
    tsize_t saved = TIFFWriteEncodedTile(mtif, tile, buff, (mtileWidth*mtileHeight*mbitCount*msamplesPerPixel)/8);
    if (saved==(mtileWidth*mtileHeight*mbitCount*msamplesPerPixel)/8)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}


bool Tiff::writeImage(BYTE* buff)
{
  if (mtif)
  {
    tsize_t stripSize = mactualWidth*mactualHeight*msamplesPerPixel;
    TIFFSetField(mtif, TIFFTAG_ROWSPERSTRIP, mactualHeight);
    tsize_t wroteSize = TIFFWriteEncodedStrip(mtif, 0, (void*) buff, stripSize);
    if (stripSize == wroteSize)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}



bool Tiff::writeDirectory()
{
  if (mtif)
  {
    return TIFFWriteDirectory(mtif) == 1 ? true : false;
  }
  return false;
}


bool Tiff::setAttributes(int newSamplesPerPixel, int newBitsPerSample, int newImageWidth, int newImageHeight, int newTileWidth, int newTileHeight, int newTileDepth, int quality)
{
  mactualWidth = newImageWidth;
  mactualHeight = newImageHeight;
  mtileWidth = newTileWidth;
  mtileHeight = newTileHeight;
  mbitCount = newBitsPerSample;
  msamplesPerPixel = newSamplesPerPixel;
  mquality = quality;
  uint32 u32TifImageWidth = (uint32) newImageWidth;
  uint32 u32TifImageLength = (uint32) newImageHeight;
  uint32 u32TileWidth = (uint32) newTileWidth;
  uint32 u32TileLength = (uint32) newTileHeight;
  uint32 u32TileDepth = (uint32) newTileDepth;
  uint16 u16BitsPerSample = (uint16) newBitsPerSample;
  uint16 u16SamplesPerPixel = (uint16) newSamplesPerPixel;
  uint16 photometric = PHOTOMETRIC_RGB;
  uint16 planarConfig = 1;
//    uint32 totalTilesX = 0, totalTilesY = 0;
//    uint32 tileSize;

  if (mtif)
  {
    try 
    {
      TIFFSetField(mtif, TIFFTAG_IMAGEWIDTH, u32TifImageWidth);
      TIFFSetField(mtif, TIFFTAG_IMAGELENGTH, u32TifImageLength);  
      TIFFSetField(mtif, TIFFTAG_BITSPERSAMPLE, u16BitsPerSample);
      TIFFSetField(mtif, TIFFTAG_SAMPLESPERPIXEL, u16SamplesPerPixel);
          //TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);  
          //TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &stripByteCounts);
      TIFFSetField(mtif, TIFFTAG_PHOTOMETRIC, photometric);
      if (mtileWidth > 0 && mtileHeight > 0)
      {
        TIFFSetField(mtif, TIFFTAG_TILEWIDTH, u32TileWidth);
        TIFFSetField(mtif, TIFFTAG_TILELENGTH, u32TileLength);
        TIFFSetField(mtif, TIFFTAG_TILEDEPTH, u32TileDepth);
      }
      TIFFSetField(mtif, TIFFTAG_PLANARCONFIG, planarConfig);
      TIFFSetField(mtif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
      TIFFSetField(mtif, TIFFTAG_JPEGQUALITY, quality);    
      TIFFSetField(mtif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
    }
    catch (std::bad_alloc &e) 
    {
      if (mtif) TIFFClose(mtif);
      merrMsg << "Insufficient memory to decompress '" << mfileName;
      merrMsg << "' into memory";
      mtif = 0;
      return false;
    } 
    catch (std::runtime_error &e) 
    {
      if (mtif) TIFFClose(mtif);
      mtif = 0;
      return false;
    }
 }
  else
  {
    return false;
  }
  return true;
}


bool Tiff::setDescription(std::string& strAttributes, int baseWidth, int baseHeight)
{
  std::ostringstream oss;
  int retval=0;
  if (mtif)
  {
    oss << strAttributes << "\r\n";
    oss << baseWidth << "x" << baseHeight << " ";
    if (mtileWidth > 0 && mtileHeight > 0)
    {
      oss << "(" << mtileWidth << "x" << mtileHeight << ") ";
    }
    oss << "-> " << mactualWidth << "x" << mactualHeight << " JPEG/RGB Q=" << mquality;
    std::string attr = oss.str();
    retval=TIFFSetField(mtif, TIFFTAG_IMAGEDESCRIPTION, attr.c_str());
  }
  return (retval == 1 ? true : false);
}


bool Tiff::setThumbNail()
{
  if (mtif)
  {
    TIFFSetField(mtif, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
    return true;
  }
  return false;
}


bool Tiff::createFile(const std::string& newFileName)
{
  if (mValidObject)
    cleanup();
  mValidObject = false;
  mtif = 0;
  mquality = 70;
  setFileName(newFileName);
    
  try {
    mtif = TIFFOpen(mfileName.c_str(), "wb8");
    if (!mtif) {
      merrMsg << "Error opening '" << mfileName << "': ";
      throw std::runtime_error(merrMsg.str());
    }
  } 
  catch (std::runtime_error &e) 
  {
    if (mtif) TIFFClose(mtif);
    mtif = 0;
    return false;
  }
  return true;
}


bool Tiff::load(const std::string& newFileName)
{
  unsigned int unpaddedScanlineBytes, paddedScanlineBytes;
  uint32 tifImageWidth = 0, tifImageLength = 0;
  uint32 tileWidth = 0, tileLength = 0;
  uint16 bitsPerSample = 0, samplesPerPixel = 0;
  uint32 rowsPerStrip = 0;
  //uint32 stripByteCounts = 0;
  uint16 photometric = 0;
  uint16 planarConfig = 0;
  uint32 tileDepth = 0;
  //uint32 totalTilesX = 0, totalTilesY = 0;
  uint32 tileSize;
  uint32 subFileType=0;
  uint32 osubFileType=0;
  uint32 subIFD=0;
  uint32 fullLength=0;
  uint32 fullWidth=0;
  uint32 xres=0, yres=0;
  uint32 xpos=0, ypos=0;
  uint32 imageDepth = 0;
  char* description = 0;

  /*struct { uint16 photometric; char const* description; } photostrings[] = { 
    { PHOTOMETRIC_MINISWHITE, "Minimal is White" },
    { PHOTOMETRIC_MINISBLACK, "Minimal is Black" },
    { PHOTOMETRIC_MASK, "Hold Mask" },
    { PHOTOMETRIC_SEPARATED, "Color Separated" },
    { PHOTOMETRIC_YCBCR, "YCBCR/CCIR 601" },
    { PHOTOMETRIC_CIELAB, "CIE Lab" },
    { PHOTOMETRIC_ICCLAB, "ICC Lab" },
    { PHOTOMETRIC_ITULAB, "ITU Lab" },
    { PHOTOMETRIC_LOGL, "CIE Log2(L)" },
    { PHOTOMETRIC_LOGLUV, "CIE Log2(L) (u', v')" },
    { 0, NULL }
  };*/

  //if (mValidObject)
  //    cleanup();
  mValidObject = false;
  mtif = 0;
  setFileName(newFileName);
  printf("Opening tiff file %s...\n", mfileName.c_str());
  
  merrMsg.str("");
 try 
  {
    mtif = TIFFOpen(mfileName.c_str(), "r");
    if (!mtif) {
      merrMsg << "Error opening '" << mfileName << "': ";
      printf("%s\n", merrMsg.str().c_str());
      throw std::runtime_error(merrMsg.str());
    }

    int dirCount = 0;
    do
    {
      TIFFGetField(mtif, TIFFTAG_IMAGEWIDTH, &tifImageWidth);
      TIFFGetField(mtif, TIFFTAG_IMAGELENGTH, &tifImageLength);  
      TIFFGetField(mtif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
      TIFFGetField(mtif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
      TIFFGetField(mtif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);  
      //TIFFGetField(mtif, TIFFTAG_STRIPBYTECOUNTS, &stripByteCounts);
      TIFFGetField(mtif, TIFFTAG_PHOTOMETRIC, &photometric);
      TIFFGetField(mtif, TIFFTAG_TILEWIDTH, &tileWidth);
      TIFFGetField(mtif, TIFFTAG_TILELENGTH, &tileLength);
      TIFFGetField(mtif, TIFFTAG_TILEDEPTH, &tileDepth);
      TIFFGetField(mtif, TIFFTAG_PLANARCONFIG, &planarConfig);
      TIFFGetField(mtif, TIFFTAG_SUBFILETYPE, &subFileType);
      TIFFGetField(mtif, TIFFTAG_OSUBFILETYPE, &osubFileType);
      TIFFGetField(mtif, TIFFTAG_SUBIFD, &subIFD);
      TIFFGetField(mtif, TIFFTAG_PIXAR_IMAGEFULLWIDTH, &fullWidth);
      TIFFGetField(mtif, TIFFTAG_PIXAR_IMAGEFULLLENGTH, &fullLength);
      TIFFGetField(mtif, TIFFTAG_XRESOLUTION, &xres);
      TIFFGetField(mtif, TIFFTAG_YRESOLUTION, &yres);
      TIFFGetField(mtif, TIFFTAG_XPOSITION, &xpos);
      TIFFGetField(mtif, TIFFTAG_YPOSITION, &ypos);
      TIFFGetField(mtif, TIFFTAG_IMAGEDEPTH, &imageDepth);
      toff_t dir_offset=0;
      TIFFGetField(mtif, TIFFTAG_EXIFIFD, &dir_offset);
      //int descRet=TIFFGetField(mtif, TIFFTAG_IMAGEDESCRIPTION, &description);
      if (dir_offset != 0)
      {
          //TIFFReadEXIFDirectory(mtif, dir_offset);
      }
      toff_t custom_offset=0;
      TIFFGetField(mtif, TIFFTAG_SUBIFD, &custom_offset);
      /*TIFFFieldArray infoarray;
      memset(&infoarray, 0, sizeof(infoarray));
      if (custom_offset != 0)
      {
          TIFFReadCustomDirectory(mtif, custom_offset, infoarray);
      }
      */
      tileSize = TIFFTileSize(mtif);
      mactualWidth = tifImageWidth;
      mactualHeight = tifImageLength;
      calcRenderedDims();
      mbitCount = bitsPerSample * samplesPerPixel;
      // number of bytes in one line
      unpaddedScanlineBytes = mactualWidth * samplesPerPixel; 
      printf("scanline size: mine %i tifflib %i\n", unpaddedScanlineBytes,
              (int) TIFFScanlineSize(mtif));
      paddedScanlineBytes = unpaddedScanlineBytes;
      
      // each row of the bitmap must be aligned at dword boundaries
      while ((paddedScanlineBytes & 3) != 0) paddedScanlineBytes++;
  
      printf("tiffDirectory %i\n", dirCount);
      printf("actualWidth %i actualHeight %i bitsPerSample %i samplesPerPixel"
              " %i tileWidth %i tileHeight %i\n",
          mactualWidth, mactualHeight, (int) bitsPerSample, (int) samplesPerPixel,
          tileWidth, tileLength);
      printf("rowsPerStrip %i photometric %i\n", (int) rowsPerStrip, 
          (int) photometric);
      printf("tileDepth %i tileSize %i planarConfig %i\n", (int) tileDepth, (int) tileSize, (int) planarConfig);
      printf("Custom Offset %i SubfileType %i OSubFileType %i SubIFD %i xRes %i yRes %i xPos %i yPos %i Image Depth %i", (int) custom_offset, (int) subFileType, osubFileType, subIFD, xres, yres, xpos, ypos, imageDepth);
      if (description != NULL) printf("Image Description: %s", description);
      printf("\n");
      dirCount++;
      } 
      while (TIFFReadDirectory(mtif));
      TIFFSetDirectory(mtif, dirCount-1);

      mbitmapSize = paddedScanlineBytes * mactualHeight;
//        pBitmap = new BYTE[bitmapSize];

/*
      switch (photometric) {
      case PHOTOMETRIC_RGB:
          if (samplesPerPixel != 3 || bitsPerSample != 8) {
              errMsg << "Error decompressing '" << fileName << "': ";
              errMsg << "Unsupported bit depth in tiff file.";
              throw std::runtime_error(errMsg.str());
          }
          break;
      case PHOTOMETRIC_PALETTE: { // color palette
          uint16 *red, *green, *blue;
          unsigned int paletteSize = 1 << bitsPerSample;
          TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);
          bool palette16bits = (colorMapBitCount
                              (paletteSize, red, green, blue)
                              == 16 ? true : false);
          for (unsigned int i = 0; i < paletteSize; i++) {
          }
          break;
      }
      default:
          int i = 0;
          while (photostrings[i].description != NULL &&
                 photostrings[i].photometric != photometric) {
              i++;
          }
          std::ostringstream description;
          if (photostrings[i].description == NULL)
              description << photometric;
          else
              description << photostrings[i].description;
          errMsg << "Error decompressing '" << fileName << "': ";
          errMsg << "Unsupported Tiff file format.\n";
          errMsg << "Tiff files that are encoded using a photometric value of " 
              << description << " are currently unsupported.";
          throw std::runtime_error(errMsg.str());
      }
      if (tileWidth > 1 && tileLength > 0) 
      {
          totalTilesX = tifImageWidth / tileWidth;
          totalTilesY = tifImageLength / tileLength;
          std::vector<BYTE> sampleTile(tileSize);
           
          for (int srcy = 0; srcy < tifImageLength; srcy += tileLength) 
          {
              for (int srcx = 0; srcx < tifImageWidth; srcx += tileWidth) 
              {
                  int bytes_read = TIFFReadTile(tif, &sampleTile[0], srcx, srcy, 0, 0);
                  if (bytes_read == -1) 
                  {
                      memset(&sampleTile[0], 0, tileSize);
                  }

                  for (int desty = srcy; desty >= 0 && desty < (srcy + tileLength) && desty < tifImageLength; desty++)
                  {
                      //bytes_read / samplesPerPixel / tileWidth)
                      int pixelCount = tileWidth; 
                      if (srcx + tileWidth > tifImageWidth)
                      {
                          pixelCount = tileWidth - (srcx + tileWidth - tifImageWidth);
                          if (pixelCount < 0) pixelCount = 0;
                      }
                          
                      BYTE *dest = &pBitmap[desty*paddedScanlineBytes+(srcx*samplesPerPixel)];
                      BYTE *src = &sampleTile[(tileWidth * (desty - srcy) * samplesPerPixel)];
                      memcpy(dest, src, pixelCount * samplesPerPixel);
                      if (srcx + tileWidth >= tifImageWidth)
                      {
                          dest = &pBitmap[(desty*paddedScanlineBytes) - (paddedScanlineBytes - unpaddedScanlineBytes)];
                          memset(dest, 0, paddedScanlineBytes - unpaddedScanlineBytes);
                      }
                  }
              }       
          } 
      }
      else 
      {
          std::vector<BYTE> sampleStrip(TIFFStripSize(tif));
          for (unsigned int row = 0; row < actualHeight; row += rowsPerStrip) {
              int rowsToRead = (row + rowsPerStrip > actualHeight ? 
                                actualHeight - row : rowsPerStrip);
              if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0),
                                       &sampleStrip[0], 
                                       -1) // rowsToRead*unpaddedScanlineBytes) 
                                       == -1)
              {
                  errMsg << "Error decompressing '" << fileName << "'";
                  throw std::runtime_error(errMsg.str());
              } else {
                  for (int rowToCopy = 0; rowToCopy < rowsToRead; rowToCopy++) {
  //                  BYTE *dest = &pBitmap[(actualHeight*paddedScanlineBytes)-
  //                                        ((row+rowToCopy+1)*paddedScanlineBytes)]; */
/*                        BYTE *dest = &pBitmap[(row+rowToCopy)*paddedScanlineBytes];
                      BYTE *src = &sampleStrip[rowToCopy*unpaddedScanlineBytes];
                      if (samplesPerPixel == 3 && bitsPerSample == 8) {
                          unsigned int i;
                          for (i = 0; i < unpaddedScanlineBytes; 
                               i += 3) {
                              dest[i] = src[i+2];
                              dest[i+1] = src[i+1];
                              dest[i+2] = src[i+0];
                          }
                          while (i < paddedScanlineBytes) {
                              dest[i] = 0;
                              i++;
                          }
                      }
                  }
              }
          }
      }
*/
    TIFFClose(mtif);
    mtif = 0;
    mValidObject = true;
  } catch (std::bad_alloc &e) {
    if (mtif) TIFFClose(mtif);
    mtif = 0;
    merrMsg << "Insufficient memory to decompress '" << mfileName;
    merrMsg << "' into memory";
    return false;
  } catch (std::runtime_error &e) {
    if (mtif) TIFFClose(mtif);
    mtif = 0;
    return false;
  }
  return true;
}

