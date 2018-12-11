#ifndef JPGCACHESUPPORT_FILE_H
#define JPGCACHESUPPORT_FILE_H

#include <vector>
#include "imagesupport.h"
#include "jpgsupport.h"

class JpgCache
{
public:
  JpgCache(int maxOpen) { mMaxOpen = maxOpen; }
  JpgCache() { mMaxOpen=12; }
  ~JpgCache() { releaseAll(); }
  void releaseAll();
  void release(Jpg *pjpg);
  Jpg* open(const std::string& newFileName, bool setGrayScale);
protected:
  unsigned int mMaxOpen;
  std::vector<Jpg*> mjpgs;
  std::vector<bool> mGrayScaleSettings;
};

#endif
