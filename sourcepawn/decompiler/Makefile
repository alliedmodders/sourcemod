# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../..
SOURCEHOOK = ../../../sourcemm-1.6/sourcehook

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = spdecomp

OBJECTS = main.cpp		\
	decompiler.cpp		\
	code_analyzer.cpp	\
	file_format.cpp		\
	platform_util.cpp	\
	zlib/adler32.c		\
	zlib/compress.c		\
	zlib/crc32.c		\
	zlib/deflate.c		\
	zlib/gzio.c			\
	zlib/infback.c		\
	zlib/inffast.c		\
	zlib/inflate.c		\
	zlib/inftrees.c		\
	zlib/trees.c		\
	zlib/uncompr.c		\
	zlib/zutil.c

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc-4.1

LINK = -static-libgcc

INCLUDE = -I. -I.. -I$(SMSDK)/public -I$(SMSDK)/public/jit -I$(SMSDK)/public/jit/x86 \
	-I$(SMSDK)/public/sourcepawn -I$(SOURCEHOOK) -I$(SMSDK)/knight/shared -Ix86

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -DHAVE_STDINT_H \
	-m32 -Wno-uninitialized -Werror
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

BINARY = $(PROJECT)

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJ_LINUX:%.c=$(BIN_DIR)/%.o)

default: all

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/x86
	mkdir -p $(BIN_DIR)/zlib
	mkdir -p $(BIN_DIR)/knight
	$(MAKE) -f Makefile jit

jit: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

clean:
	rm -rf $(BIN_DIR)/zlib/*.o
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/$(BINARY)

