/*******************************************************************************
Copyright (c) 2005-2016, Paul F. Richards

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

#ifndef IMAGE_FILE_H
#define IMAGE_FILE_H

#include <string>
#include <sstream>
#include <vector>
#include <cmath>

#ifndef uint
typedef unsigned int uint;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

// Visual C++ 2005 and below does not contain lroundf

#ifdef _MSC_VER
#ifndef lroundf
inline long int lroundf(float x) { return (long int) floorf(x+(float)0.5); }
#endif
#ifndef lround
inline long int lround(double x) { return (long int) floor(x+0.5); }
#endif
#endif

#ifndef iround
inline int iround(double x) { return (int) lround(x); }
#endif

int dprintf(const char* format, ...);

class Image {
protected:
  int mactualWidth, mactualHeight, mbitCount;
  int msamplesPerPixel;
  double mrenderedWidth, mrenderedHeight;
  bool mupsideDown;
  int mshiftRight, mshiftDown;
  int mpageWidth, mpageHeight;
  int mlevel;
  int mreadWidth, mreadHeight;
  bool mValidObject;
  std::string mfileName;
  std::vector<BYTE> minfo;
  int mbitmapSize;
  BYTE *mpBitmap;
  int mrotation;
  double mzoomPercentage;
  void setFileName(const std::string& newFileName) 
  { mfileName = newFileName; }
  BYTE mbkgColor;
  bool mGrayScale;
public:
  std::ostringstream merrMsg;
  bool mmaintainAspectRatio, mfitToWindow;
  std::string getFileName() { return mfileName; }
  BYTE* bitmapPointer() { return mpBitmap; }
  void alignBytes();
  virtual bool open(const std::string& newFileName, bool setGrayScale = false) = 0;
  virtual bool read(int x, int y, int width, int height, bool setGrayScale = false) = 0;
  bool setZoomPercentage(double);
  double getZoomPercentage() { return mzoomPercentage; }
  bool writeBmp(char*);
  bool rotateCW();
  bool rotateCCW();
  bool rotateAlgorithm1();
  bool rotateAlgorithm2();
  bool rotate24bitCW(); // not used anymore 
  bool rotate24bitCCW(); // not used anymore
  void calcRenderedDims();
  void resetRenderedDims();
  int getSamplesPerPixel() { return msamplesPerPixel; }
  int getActualWidth()  { return (mValidObject) ? mactualWidth : 0; }
  int getActualHeight() { return (mValidObject) ? mactualHeight : 0; }
  int getRenderedWidth() { return (mValidObject) ? (uint) mrenderedWidth : 0; }
  int getRenderedHeight() { return (mValidObject) ? (uint) mrenderedHeight : 0; }
  void autoUpdatePageValues(int hWnd);
  void setupPageValues(int winWidth, int winHeight);
  void setShiftDownPt(int y);
  void setShiftRightPt(int x);
  int getShiftDownPt() { return (mValidObject) ? mshiftDown : 0; }
  int getShiftRightPt() { return (mValidObject) ? mshiftRight : 0; }
  void setPageWidth(int width);
  void setPageHeight(int height);
  int getPageWidth() { return (mValidObject) ? mpageWidth : 0; }
  int getPageHeight() { return (mValidObject) ? mpageHeight : 0; }
  int getReadWidth() { return (mValidObject) ? mreadWidth : 0; }
  int getReadHeight() { return (mValidObject) ? mreadHeight : 0; }
  bool isValidObject() { return mValidObject; }
  void cleanup();
  void getErrMsg(std::string& errStr) { errStr = merrMsg.str(); }
  int paint(int, int, int, int, int);
  Image();
  virtual ~Image();
  void setUnfilledColor(BYTE newColor) { mbkgColor = newColor; }
  bool getGrayScale() { return mGrayScale; }
};

Image* loadImage(const char* fileName, std::string& errStr);

class DummyImage : public Image {
public:
  bool open(const std::string&) { return false; }
  bool read(int, int, int, int) { return false; }
  bool loadAndScale(const std::string&, int, int) 
  { return false; }
  void cleanup() { }
  DummyImage() { mValidObject = false; }
  ~DummyImage() { } 
};

#endif // IMAGE_FILE_H
