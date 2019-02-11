/**************************************************************************
Initial author: Paul F. Richards (paulrichards321@gmail.com) 2005-2017
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
#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <new>
#include <cstdio>
#include <memory>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <iostream>
#include "jpgsupport.h"


int reset_error_mjr(j_common_ptr cinfo)
{
  return 0;
}

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */


/*
 * Here's the routine that will replace the standard error_exit method:
 */

void my_jpeg_error_exit(j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  //struct jpeg_error_mgr* pjerr = cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  /* (*cinfo->err->output_message) (cinfo); */

  /* Return control to the setjmp point */
  throw cinfo->err; 
  //longjmp(myerr->setjmp_buffer, 1);
}


bool Jpg::testHeader(BYTE* header, int)
{
  if (header[0] == 0xFF && header[1] == 0xD8)
    return true;
  else
    return false;
}


bool Jpg::open(const std::string& newFileName, bool setGrayScale)
{
  FILE *infile;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW* pjSampleRows = 0; 
  
  mValidObject = false;
  msamplesPerPixel = 3;
  mGrayScale = false;
  if (mpFullBitmap != 0)
  {
    delete[] mpFullBitmap;
    mpFullBitmap = 0;
  }
  memset(&cinfo, 0, sizeof(cinfo));
  memset(&jerr, 0, sizeof(jerr));

  mfileName = newFileName;
  merrMsg.str("");

  infile = fopen(mfileName.c_str(), "rb");
  if (infile == 0) 
  {
    merrMsg << "Error opening '" << mfileName << "': " << std::strerror(errno);
    return false;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit = my_jpeg_error_exit;

  try 
  {
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    if (setGrayScale || cinfo.num_components==1) 
    {
      mGrayScale = true;
      msamplesPerPixel = 1;
      cinfo.out_color_space = JCS_GRAYSCALE;
    }
    jpeg_start_decompress(&cinfo);
    mrenderedWidth = mactualWidth = cinfo.output_width;
    mrenderedHeight = mactualHeight = cinfo.output_height;
    mbitCount = msamplesPerPixel * 8;

    mpFullBitmap = new BYTE[mactualWidth * mactualHeight * msamplesPerPixel];
    memset(mpFullBitmap, mbkgColor, mactualWidth * mactualHeight * msamplesPerPixel);
    pjSampleRows = new JSAMPROW[mactualHeight];
    for (unsigned int y = 0; y < mactualHeight; y++)
    {
      pjSampleRows[y] = &mpFullBitmap[mactualWidth * y * msamplesPerPixel];
    }
    int totalScanlines=0;
    while (cinfo.output_scanline < cinfo.output_height) 
    {
       totalScanlines+=jpeg_read_scanlines(&cinfo, &pjSampleRows[totalScanlines], mactualHeight-totalScanlines);
       
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    infile = 0;
    delete[] pjSampleRows;
    mValidObject=true;
    mrenderedHeight=mactualHeight=totalScanlines;
  }
  catch (jpeg_error_mgr* pJerr) 
  {
    merrMsg << "Error decompressing '" << mfileName << "': ";
    merrMsg << pJerr->jpeg_message_table[pJerr->msg_code];
    jpeg_destroy_decompress(&cinfo);
    if (infile) fclose(infile);
    cleanup();
    if (pjSampleRows)
    {
      delete[] pjSampleRows;
    }
    return false;
  } 
  catch (std::bad_alloc &e) 
  {
    mpFullBitmap = 0;
    jpeg_destroy_decompress(&cinfo);
    if (infile) fclose(infile);
    cleanup();
    if (pjSampleRows)
    {
      delete[] pjSampleRows;
    }
    merrMsg << "Insufficient memory to decompress '" << mfileName;
    merrMsg << "' into memory";
    return false;
  }
  return true;
}


bool Jpg::read(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool passedGrayscale)
{
  if (mpBitmap != 0)
  {
    delete[] mpBitmap;
    mpBitmap = 0;
  }
  if (mpFullBitmap==0)
  {
    std::cerr << "Error: Jpg not loaded into memory yet." << std::endl;
    return false;
  }
  mreadWidth=0;
  mreadHeight=0;
  merrMsg.str("");
  if (x<0 || y<0 || x>mactualWidth || y>mactualHeight || height<=0 || width<=0) 
  {
    std::cerr << "In jpeg::read parameters out of bounds." << std::endl;
    return false;
  }
  if (x+width > mactualWidth)
  {
    width = mactualWidth - x;
    std::cerr << "In jpeg::read, width truncated." << std::endl;
  }
  if (y+height > mactualHeight)
  {
    height = mactualHeight - y;
    std::cerr << "In jpeg::read, height truncated." << std::endl;
  }
  int samplesPerPixel=3;
  if (passedGrayscale || mGrayScale)
  {
    samplesPerPixel=1;
  }
  try
  {
    unsigned int bitmapSize = width*height*samplesPerPixel;
    unsigned int tileRowSize = width * samplesPerPixel;
    mpBitmap = new BYTE[bitmapSize];
//    std::cout << "In jpgsupport.cpp: width=" << width << " height=" << height << " samplesPerPixel" << samplesPerPixel << std::endl;
    memset(mpBitmap, mbkgColor, bitmapSize);
    unsigned int row=y;
    for (unsigned int yDest=0; yDest<height && row<(y+height); yDest++) 
    {
      memcpy(&mpBitmap[yDest*tileRowSize], &mpFullBitmap[(mactualWidth*row*msamplesPerPixel)+(x*msamplesPerPixel)], width*samplesPerPixel);
      row++;
    } 
    mreadHeight=height;
    mreadWidth=width;
    mValidObject = true;
  }
  catch (std::bad_alloc &e) 
  {
    mpBitmap = 0;
    cleanup();
    merrMsg << "Insufficient memory to decompress '" << mfileName;
    merrMsg << "' into memory";
    return false;
  }
  return true;
}


bool Jpg::unbufferedRead(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
  FILE *infile = NULL;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  memset(&cinfo, 0, sizeof(cinfo));
  memset(&jerr, 0, sizeof(jerr));

  //if (mValidObject) cleanup();
  if (mpBitmap != 0)
  {
    delete[] mpBitmap;
    mpBitmap = 0;
  }
 
  mreadWidth=0;
  mreadHeight=0;
  merrMsg.str("");
  if (x<0 || y<0 || x>mactualWidth || y>mactualHeight || height<=0 || width<=0) 
  {
    std::cerr << "In jpeg::read parameters out of bounds." << std::endl;
    return false;
  }
  if (x+width > mactualWidth)
  {
    width = mactualWidth;
    std::cerr << "In jpeg::read, width truncated." << std::endl;
  }
  if (y+height > mactualHeight)
  {
    height = mactualHeight;
    std::cerr << "In jpeg::read, height truncated." << std::endl;
  }
  try
  {
    infile = fopen(mfileName.c_str(), "rb");
    if (infile == 0) 
    {
      merrMsg << "Error opening '" << mfileName << "': " << strerror(errno);
      return false;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_jpeg_error_exit;

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_stdio_src(&cinfo, infile);

    /* Step 3: read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, TRUE);
    /* We can ignore the return value from jpeg_read_header since
    *   (a) suspension is not possible with the stdio data source, and
    *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
    * See libjpeg.doc for more info.
    */

    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Step 5: Start decompressor */
    jpeg_start_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */ 
    /* JSAMPLEs per row in output buffer */
    mactualWidth = cinfo.output_width; //cinfo.image_width;
    mactualHeight = cinfo.output_height; //cinfo.image_height;
    calcRenderedDims();
    mbitCount = 3 * 8;
    munpaddedScanlineBytes = cinfo.output_width * 3;
   /* Make a one-row-high sample array that will go away when done with image */
    //buffer = (*cinfo.mem->alloc_sarray)
    //  ((j_common_ptr) &cinfo, JPOOL_IMAGE, paddedScanlineBytes, 1);
    mbitmapSize = width*height*3;
    uint tileRowSize = width * 3;
    mpBitmap = new BYTE[mbitmapSize];
//        dprintf("jpeg tile size: %i\n", bitmapSize);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    if (cinfo.output_components == 3) 
    {
        std::vector<JSAMPLE> jSamples(cinfo.output_width * 3);
        JSAMPROW pjSampleRow = &jSamples[0];
        JSAMPARRAY pjSampleArray = &pjSampleRow;
        unsigned int yDest = 0;
        while (cinfo.output_scanline < cinfo.output_height) 
        {
            jpeg_read_scanlines(&cinfo, pjSampleArray, 1);
            if (width > cinfo.output_width)
            {
              width = cinfo.output_width;
            }
            if (cinfo.output_scanline >= y && cinfo.output_scanline <= y+height && yDest < height)
            {
              memcpy(&mpBitmap[yDest*tileRowSize], &jSamples[x*3], width*3);
              yDest++;
            }
        }
    } 
    else if (cinfo.output_components == 1) 
    {
        std::vector<JSAMPLE> jSamples(cinfo.output_width);
        JSAMPROW pjSampleRow = &jSamples[0];
        JSAMPARRAY pjSampleArray = &pjSampleRow;
        int yDest = 0;
        while (cinfo.output_scanline < cinfo.output_height) 
        {
            /* jpeg_read_scanlines expects an array of pointers to scanlines.
             * Here the array is only one element long, but you could ask for
             * more than one scanline at a time if that's more convenient.
             */
            /* Assume put_scanline_someplace wants a pointer and sample count. */
            jpeg_read_scanlines(&cinfo, pjSampleArray, 1);
    
            JSAMPLE *dest = &mpBitmap[yDest * tileRowSize];
            // Copy loop for RGB data
            // Note: the windows GDI bitmap functions take bitmaps in BGR order
            /*
            if (cinfo.output_components == 3) {
                unsigned int i;
                for (i = 0; i < unpaddedScanlineBytes; i+=3) {
                    dest[i] = jsamples[i+2];
                    dest[i+1] = jsamples[i+1];
                    dest[i+2] = jsamples[i];
                }
                while (i < paddedScanlineBytes) {
                    dest[i] = 0;
                    i++;
                }
            */
            // Copy loop for grey scale data
            uint srci, desti;
            for (srci = x, desti = 0; srci < mactualWidth; srci++, desti+=3) 
            {
                dest[desti+2] = dest[desti+1] = dest[desti] = jSamples[srci];
            }
        }
    }
    mreadHeight=height;
    mreadWidth=width;
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);
    infile = 0;

    mValidObject = true;
  } 
  catch (jpeg_error_mgr* pJerr) 
  {
  /* If we get here, the JPEG library has signaled an error.
   * We need to clean up the JPEG object, close the input file, and return.
   */
    merrMsg << "Error decompressing '" << mfileName << "': ";
    merrMsg << pJerr->jpeg_message_table[pJerr->msg_code];
    jpeg_destroy_decompress(&cinfo);
    if (infile) fclose(infile);
    cleanup();
    return false;
  } 
  catch (std::bad_alloc &e) 
  {
    jpeg_destroy_decompress(&cinfo);
    if (infile) fclose(infile);
    cleanup();
    merrMsg << "Insufficient memory to decompress '" << mfileName;
    merrMsg << "' into memory";
    return false;
  }
  return true;
}


void Jpg::close()
{
  if (mpFullBitmap != 0)
  {
    delete[] mpFullBitmap;
    mpFullBitmap = 0;
  }
  // pBitmap is deallocated in cleanup 
  cleanup();
  initialize();
}


void Jpg::initialize()
{
  munpaddedScanlineBytes = 0; 
  mValidObject = false;
  mpFullBitmap = 0;
  mpBitmap = 0;
}

