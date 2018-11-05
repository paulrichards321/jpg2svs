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
#include "tiffsupport.h"
#include <iostream>

int main(int argc, char** argv)
{
  Tiff tif;
  if (argc < 2)
  {
    std::cerr << "syntax: svsinfo filename" << std::endl;
    return 1;
  }
  if (tif.load(argv[1])==false)
  {
    std::string errMsg;
    tif.getErrMsg(errMsg);
    std::cerr << "Failed to open " << argv[1] << ": " << errMsg << std::endl;
    return 2;
  }
  else
  {
    tif.close();
  }
  return 0;
}

