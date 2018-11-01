/*******************************************************************************
Copyright (c) 2005-2008, Paul F. Richards

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/
#ifndef TIFFSUPPORT_H
#define TIFFSUPPORT_H

#include <string>
#include "imagesupport.h"

extern "C" {
namespace TIFFLIB {
#include "tiffio.h"
}
}


class Tiff : public Image {
public:
	bool read(int, int, int, int, bool setGrayScale = false) { return false; }
  bool open(const std::string&, bool setGrayScale = false) { return false; }
  bool load(const std::string& newFileName);
  bool createFile(const std::string& newFilename);
  bool setThumbNail();
  void close() { if (mtif) { TIFFClose(mtif); mtif=0; } } 
	bool setTileAttributes(int newSamplesPerPixel, int newBitsPerSample, int newImageWidth, int newImageHeight, int newTileWidth, int newTileHeight, int newTileDepth, int quality, std::string& strAttributes);
  bool writeDirectory();
  bool writeEncodedTile(BYTE* buff, int x, int y, int z);
  Tiff() { mtif=0; }
	~Tiff() { }
	static bool testHeader(BYTE*, int);
protected:
  TIFFLIB::TIFF* mtif;
  int mtileWidth, mtileHeight;
};

#endif

