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

#ifndef JPGSUPPORT_H
#define JPGSUPPORT_H

#include <cstdio>

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

#include "imagesupport.h"

class Jpg : public Image {
protected:
  uint munpaddedScanlineBytes;	/* row width without padding */
  bool mfullReadDone;
  BYTE* mpFullBitmap;
public:
  bool load(const std::string& newFileName);
  bool read(int x, int y, int width, int height, bool setGrayScale = false);
  bool unbufferedRead(int x, int y, int width, int height);
  bool open(const std::string& newFileName, bool setGrayScale = false);
  void close();
  void initialize();
  Jpg() : Image() { initialize(); }
  ~Jpg() { close(); }
  static bool testHeader(BYTE*, int);
};

#endif
