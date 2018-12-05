all: jpg2svs svsinfo
.PHONY: all

CXX=g++
SLIDE_CFLAGS= -fstack-protector-all -Wall -O2
OPENCVLIB=/usr/lib64
OPENCV_LFLAGS=-L$(OPENCVLIB) -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_imgcodecs -lopencv_imgproc
SLIDE_LFLAGS=-ljpeg -ltiff $(OPENCV_LFLAGS) -lncurses

CONV_OBJECTS=jpg2svs.o jpgcachesupport.o imagesupport.o jpgsupport.o tiffsupport.o composite.o 

INFO_OBJECTS=imagesupport.o jpgsupport.o tiffsupport.o svsinfo.o

.cpp.o:
	$(CXX) -c -g $(SLIDE_CFLAGS) $<

jpg2svs: $(CONV_OBJECTS)
	$(CXX) -g -o jpg2svs -pthread $(CONV_OBJECTS) \
  	$(SLIDE_LFLAGS) 

svsinfo: $(INFO_OBJECTS)
	$(CXX) -g -o svsinfo -pthread $(INFO_OBJECTS) \
	  $(SLIDE_LFLAGS)

