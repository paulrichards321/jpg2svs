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
#include <vector>
#include <algorithm>
#include "jpgcachesupport.h"

void JpgCache::release(Jpg *pjpg)
{
  for (std::vector<Jpg*>::iterator it=mjpgs.begin(); it<mjpgs.end(); it++)
  {
    if (*it == pjpg)
    {
      mjpgs.erase(it);
      break;
    }
  }
  delete pjpg;
}

void JpgCache::releaseAll()
{
  for (std::vector<Jpg*>::iterator it=mjpgs.begin(); it <mjpgs.end(); it++)
  {
    delete *it;
    *it=NULL;
  }
  mjpgs.clear();
}

Jpg* JpgCache::open(const std::string& newFileName, bool setGrayScale)
{
  Jpg *jpg = 0;
  std::vector<Jpg*>::iterator it;
  if (mjpgs.size() > 0)
  {
    for (it=mjpgs.begin(); it < mjpgs.end(); it++)
    {
      if ((*it)->getFileName()==newFileName)
      {
        jpg = *it;  
        it=mjpgs.erase(it);
        if (mjpgs.size() > 0)
        {
          mjpgs.insert(mjpgs.begin(), jpg);
        }
        else
        {
          mjpgs.push_back(jpg);
        }
        break;
      }
    }
  }
  if (jpg == 0)
  {
    jpg = new Jpg;
    jpg->open(newFileName, setGrayScale);
    if (mjpgs.size()+1 > mMaxOpen)
    {
      Jpg *lastJpg=mjpgs.back();
      delete lastJpg;
      mjpgs.pop_back();
    }
    if (mjpgs.size() > 0)
    {
      mjpgs.insert(mjpgs.begin(), jpg);
    }
    else
    {
      mjpgs.push_back(jpg);
    }
  }
  return jpg;
}

