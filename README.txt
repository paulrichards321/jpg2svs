#------------------------------------------------------------------------------
# What is this?
#------------------------------------------------------------------------------
# An Olympus/Bacus ini jpg dataset convertor to TIFF/SVS file format readable
# by Aperio ImageScope, the openslide library, and OMERO Virtual Microscopy.
# It is used for converting a directory of jpg files with two or more of their
# accompanying  "FinalScan.ini", "FinalCond.ini", "SlideScan.ini", 
# "SlideCond.ini" files to convert into one large tiff file. The resulting tiff
# will be in Aperio like TIFF file format, but should be readable by any 
# program that can process large tiled TIFF files.

#------------------------------------------------------------------------------
# Instructions for compiling jpg2svs:
#------------------------------------------------------------------------------
# Go to method 1 right below to use msys2 on Windows to build jpg2svs.
# Skip to method 2 if you are using Visual Studio.
# Skip to method 3 if you are using RedHat/CentOS/Scientific Linux.
# Skip to usage notes at the bottom of this document if you are interested
# in how to use the command line program.

# Every line in this file that starts with a # (pound symbol) is not meant to
# be typed into a shell prompt (unix or msys2) or command prompt 
# (windows/visual studio)

# -----------------------------------------------------------------------------
# METHOD 1 - Compiling jpg2svs on Windows with msys2
# -----------------------------------------------------------------------------
# Install MSYS2. Download the package from here: 
# http://www.msys2.org/

# 'pacman' is the command line package manager for MSYS2. 
# Update the MSYS2 system, then close your MSYS2 window:

pacman -Syu

# Reopen the MSYS2 window, and update all the other packages:
pacman -Syu

# Install gcc/g++ compilers:
pacman -S mingw64/mingw-w64-x86_64-gcc

# Install 'make' command:
pacman -S mingw64/mingw-w64-x86_64-make

# and:
pacman -S make

# Install pkg-config:
pacman -S msys/pkg-config

# Install libtiff:
pacman -S mingw64/mingw-w64-x86_64-libtiff

# Install opencv (this will also grab the libtiff and libjpeg-turbo 
dependacies):
pacman -S mingw64/mingw-w64-x86_64-opencv

# Install git if you plan on getting the code through git:
pacman -S git

# If you haven't already downloaded the source code for jpg2svs grab it here:
git clone https://github.com/paulrichards321/jpg2svs.git

# Compile it by typing:
cd jpg2svs
make

# You might get some warnings about unsigned/signed integers but if no errors
# you will have a executable in output/bin/jpg2svs.exe. When moving the
# executable to another system or not executing in a MSYS2 or Mingw shell you
# will need to also supply the MSYS2 dlls for opencv, libjpeg-turbo, and
# libtiff, and some other generic C/C++ runtime MSYS2 dlls.

# -----------------------------------------------------------------------------
# METHOD 2: Compiling jpg2svs with Visual Studio (tested with Visual Studio 
# 2015 on Windows 8.1 and Windows 10)
# -----------------------------------------------------------------------------
# --NOTE ABOUT VCVARS32.BAT, VCVARS64.BAT, CMAKE, AND ARCHITECTURE--
# In the below instructions you will use a program called cmake and a Visual
# Studio batch file called vcvars. Make sure when you set your architecture 
# with cmake and vcvars that you use it consistently through all the packages
#  you will have to build. There's vcvars32.bat and vcvars64.bat for the 
# 32-bit and 64-bit versions. The AMD 64 bit version of Visual Studio 14 2015 
# the vcvars bat file is in 
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat"
 
# --DOWNLOAD AND INSTALL CMAKE--
# Download and install the windows binary package for cmake from 
# https://cmake.org

# --DOWNLOAD AND INSTALL NASM--
# Download and install the windows binary package for nasm 
# (x86/amd64 assembler) from 
# https://www.nasm.us

# --CONFIGURE NASM PATH--
# Add the path of nasm.exe to your PATH environmental variables. Use the control
# panel, advanced system settings, save and then close and reopen your command
# window. Before you run cmake, make sure nasm is in the current PATH otherwise
# libjpeg-turbo will not build properly.

# --DOWNLOAD AND BUILD LIBJPEG-TURBO--
# Download libjpeg-turbo from https://libjpeg-turbo.org

# Use cmake to configure libjpeg-turbo. You can use the gui or the command line,
# whatever you find easier. If you use the command line, cd into the extracted
# root directory of the source and create a build dir, then find the location of
# your cmake.exe (see below for example) and use your version of Visual Studio
# and architecture (Visual Studio 2015 Win64 shown below) and your version of
# MSBuild and type:
cd libjpeg-turbo-1.5.3
mkdir build
cd build
"C:\Program Files\cmake\bin\cmake.exe" -G "Visual Studio 14 2015 Win64" ..
"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" turbojpeg-static.vcxproj

# Be careful with the double quotation marks and two dots after Win64 in the
# above cmake command, they are needed.

# --DOWNLOAD AND BUILD LIBTIFF-- 
# I have successfully used version 4.0.7 and 4.0.9. The libtiff site is here: 
# http://www.simplesystems.org/libtiff/
# Unzip or untar the libtiff archive. You will need to edit the nmake.opt 
# file in the tiff root directory to include jpeg support by specifying the
# jpeg headers and resulting built jpeg library directory. You can search 
# JPEG_SUPPORT and remove the # (pound sign) in front of the JPEG = lines:
JPEG_SUPPORT = 1
JPEGDIR = ../libjpeg-turbo
JPEG_INCLUDE = -I../libjpeg-turbo_dist -I$(JPEGDIR)
JPEG_LIB = ../libjpeg-turbo_dist/Debug/turbojpeg-static.lib

# After you edited the above four lines, then you can build libtiff using nmake.
# Open a command window and run the right
# vcvars bat file for your architecture and cd into the right version of libtiff
# that you downloaded by typing:
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat"
cd tiff-4.0.9
nmake /f Makefile.vc

#--DOWNLOAD AND EXTRACT PREBUILT OPENCV--
# Download and extract the opencv library for your version of Visual Studio:
# https://opencv.org
# There should be a version that is already built that just includes the header
# files and library files.

# --CONFIGURE INCLUDE AND LIB LOCATIONS FOR jpg2svs--
# Download the source for jpg2svs via git or unzip or untar it. Run the vcvars
# bat file for your architecture and then with the command window still open
# cd into the jpg2svs directory, then edit nmake.opt with your favorite editor
# and update the INCLUDE and LIB variables at the top of the file to point to 
# the INCLUDE of the libjpeg-turbo build and source directory, the libtiff 
# source directory, and the opencv include directory. Then modify (if needed) 
# the location of the LIB files for libjpeg-turbo, libtiff, and opencv. The 
# variables in nmake.opt that need to be tailored depending upon the versions 
# of libtiff, libjpeg-turbo, and opencv are these:

# --FINALLY! BUILD jpg2svs--
# Then type:
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat"
cd jpg2svs
nmake /f Makefile.vc

# --FINAL NOTES--
# You will properly get some warning messages about signed and unsigned 
# integers but it will still work properly once it is built. If it builds 
# without any errors, jpg2svs.exe will be in the current directory.

# You will also need the dll for opencv (at this point opencv_world342.dll) in
# the same directory as jpg2svs.exe when you run it. The Visual C++ 2015 C/C++
# Runtime Package (which will already be installed with the Compiler) might also
# be needed to run it.

# If you are still lost, please be patient. I am going to script this whole 
# process with PowerShell soon.

# -----------------------------------------------------------------------------
# METHOD 3: Compiling jpg2svs on Redhat 7/CentOS 7/Scientific Linux
# -----------------------------------------------------------------------------
# Install git and development tools if you haven't done so already
sudo yum install git
sudo yum groupinstall 'Development Tools'

# Install libjpeg, libtiff, and ncurses by typing:
sudo yum install libjpeg-turbo-devel libtiff-devel ncurses-devel

# -- BUILD OPENCV OR DOWNLOAD DEVELOPMENT RPM FOR IT --
# You will need opencv >= 3. You can either find a place to grab or build it using these directions:
sudo yum install cmake pkgconfig
sudo yum install libpng-devel jasper-devel openexr-devel libwebp-devel
sudo yum install libdc1394-devel libv4l-devel gstreamer-plugins-base-devel
sudo yum install tbb-devel eigen3-devel

# Now grab the opencv source:
git clone https://github.com/opencv/opencv.git
cd opencv

# Now specify a stable version of opencv. 3.4.3 works fine:
git checkout 3.4.3
cd ..
mkdir opencv-build
cd opencv-build
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local ../opencv/
make
sudo make install

# When you build opencv manually, it places it's library files in 
# /usr/local/lib64. You will need /usr/local/lib64 in your LD Path if it 
# isn't already there. You can add this by dropped adding a file in 
# /etc/ld.so.conf called usrlocallib64.conf:
sudo sh -c 'echo "/usr/local/lib64" > /etc/ld.so.conf.d/usrlocallib64.conf'

# Run ldconfig:
sudo ldconfig

# Now go back to your jpg2svs directory:
cd ..

# -- GRAB THE JPG2SVS IF YOU HAVEN'T ALREADY AND COMPILE --
git clone https://github.com/paulrichards321/jpg2svs
cd jpg2svs
make -f Makefile.unix

# You might get some warnings about signed/unsigned integer, you can ignore
# those. 
# --INSTALL --
# You can then copy jpg2svs to somewhere in your path:
cp jpg2svs /usr/local/bin

#------------------------------------------------------------------------------
# HOW TO USE THE COMMAND LINE PROGRAM
#------------------------------------------------------------------------------
# syntax: jpg2svs -h[0,1] -x[bestXOffset] -y[bestYOffset] -z[0,1] -q[0-100] <inputfolder> <outputfile> 
# Flags:	
# -h highlight visible areas with a black border on the top pyramid
#    level. Default on, set to 0 to turn off.
#	-x and -y are Optional: set best X, Y offset of image if upper and lower 
#    pyramid levels are not aligned.
#	-z Process Z-stack. Set to 0 to turn off. Default off.
# -q Set minimal jpeg quality percentage. Default 90.

