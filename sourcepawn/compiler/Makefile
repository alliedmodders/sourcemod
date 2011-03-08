# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../..

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

BINARY = spcomp

OBJECTS = binreloc.c libpawnc.c lstring.c memfile.c pawncc.c sc1.c sc2.c sc3.c sc4.c \
	sc5.c sc6.c sc7.c scexpand.c sci18n.c sclist.c scmemfil.c scstate.c sctracker.c \
	scvars.c sp_file.c sp_symhash.c
OBJECTS += zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzio.c \
	   zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c \
	   zlib/uncompr.c zlib/zutil.c

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing -fomit-frame-pointer
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

LINK = -lgcc -static-libgcc

INCLUDE = -I. -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn

CFLAGS += -DLINUX -DHAVE_STDINT_H -DAMX_ANSIONLY -DENABLE_BINRELOC -Dstricmp=strcasecmp -m32
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(C_OPT_FLAGS)
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)
ifeq "$(GCC_VERSION)" "4"
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/zlib
	$(MAKE) -f Makefile compiler

compiler: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -ldl -lm -o $(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean:
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/zlib/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
