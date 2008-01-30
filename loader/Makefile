#(C)2004-2007 AlliedModders LLC
#
# This sample Makefile is designed for SourceMod extensions that
# do not need to be built against HL2SDK, and do not require 
# running against Metamod:Source.

SMSDK = ..
SOURCEMM = ../../sourcemm-1.6

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = sourcemod_mm

OBJECTS = loader.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -g -ggdb3
CPP_GCC4_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
CPP = gcc-4.1

LINK = -static-libgcc

INCLUDE = -I. -I.. -Isdk -I$(SOURCEMM) -I$(SOURCEMM)/sourcehook -I$(SOURCEMM)/sourcemm \
	-I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions \

CFLAGS = -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp \
	-Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca \
	-Dstrcmpi=strcasecmp -Wall -Werror -fPIC -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H \
	-Wno-uninitialized -m32
CPPFLAGS = -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

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
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

BINARY = $(PROJECT)_i486.so

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/sdk
	$(MAKE) extension

extension: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) $(OBJ_LINUX) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:	
	$(MAKE) all DEBUG=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/sdk/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/sdk/*.o
	rm -rf Debug/$(BINARY)
