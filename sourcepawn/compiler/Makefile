#(C)2004-2007 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../..

### EDIT BELOW FOR OTHER PROJECTS ###

OPT_C_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing -fomit-frame-pointer 
OPT_CPP_FLAGS = 
DEBUG_C_FLAGS = -g -ggdb3
CPP = gcc-4.1
BINARY = spcomp

OBJECTS = libpawnc.c lstring.c memfile.c pawncc.c sc1.c sc2.c sc3.c sc4.c sc5.c \
	  sc6.c sc7.c scexpand.c sci18n.c sclist.c scmemfil.c scstate.c sctracker.c \
	  scvars.c sp_file.c binreloc.c zlib/adler32.c zlib/crc32.c zlib/deflate.c \
	  zlib/gzio.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c \
	  zlib/trees.c zlib/uncompr.c zlib/zutil.c zlib/compress.c

LINK = -lgcc -static-libgcc

INCLUDE = -I. -I$(SMSDK)/public/sourcepawn

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

ifeq "$(GCC_VERSION)" "4"
	OPT_CPP_FLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
endif	

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS = $(DEBUG_C_FLAGS)
else
	BIN_DIR = Release
	CFLAGS = $(OPT_C_FLAGS)
	CPPFLAGS += $(OPT_CPP_FLAGS)
endif

CFLAGS += -DLINUX -DNDEBUG -DHAVE_STDINT_H -DAMX_ANSIONLY 
CPPFLAGS += -Wno-deprecated -fno-exceptions -fno-rtti

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJECTS:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/zlib
	$(MAKE) spcomp

spcomp: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) $(OBJ_LINUX) $(LINK) -ldl -lm -o$(BIN_DIR)/$(BINARY)

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/$(BINARY)

