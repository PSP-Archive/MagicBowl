TARGET = MagicBowl

SOURCES = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SOURCES)) 

INCDIR =
CFLAGS = -G0 -Wall -g 
CXXFLAGS = $(CFLAGS) -fno-exceptions #-fno-rtti
ASFLAGS = $(CFLAGS)
BUILD_PRX = 1

LIBDIR =
LDFLAGS =
LIBS= -lpspmz2gu -lpsphbc -lstdc++ -lpsputility -lpspaudio -lpspmp3 -lintraFont -lmp4ff -lpspmpeg -lpspmpegbase -lpspgum -lpspgu -lpsprtc -lpsppower -lpng -lz -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP MagicBowl
PSP_EBOOT_ICON=ICON0.PNG
PSP_LARGE_MEM=1
PSP_FW_VERSION=300

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak