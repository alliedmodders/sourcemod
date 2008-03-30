#(C)2004-2008 Metamod:Source Development Team
# Makefile written by David "BAILOPAN" Anderson

HL2SDK_ORIG = ../../../hl2sdk
HL2SDK_OB = ../../../hl2sdk-ob
SOURCEMM14 = ../../../sourcemm-1.4
SOURCEMM16 = ../../../sourcemm-1.6
SRCDS_BASE = ~/srcds
SMSDK = ../..

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = sdktools

OBJECTS = sdk/smsdk_ext.cpp extension.cpp vdecoder.cpp vcallbuilder.cpp vcaller.cpp \
	vnatives.cpp vsound.cpp tenatives.cpp trnatives.cpp tempents.cpp vstringtable.cpp \
	vhelpers.cpp vglobals.cpp voice.cpp inputnatives.cpp teamnatives.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -g -ggdb3
CPP_GCC4_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
CPP = gcc-4.1

ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2PUB = $(HL2SDK_ORIG)/public
	HL2LIB = $(HL2SDK_ORIG)/linux_sdk
	METAMOD = $(SOURCEMM14)
	INCLUDE += -I$(HL2SDK)/public/dlls
	SRCDS = $(SRCDS_BASE)
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2PUB = $(HL2SDK_OB)/public
	HL2LIB = $(HL2SDK_OB)/linux_sdk
	CFLAGS += -DORANGEBOX_BUILD
	METAMOD = $(SOURCEMM16)
	INCLUDE += -I$(HL2SDK)/public/game/server -I$(HL2SDK)/common
	SRCDS = $(SRCDS_BASE)/orangebox
endif
ifeq "$(ENGINE)" ""
	echo "You must supply ENGINE=orangebox or ENGINE=original"
	false
endif

LINK_HL2 = $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a vstdlib_i486.so tier0_i486.so

LINK += $(LINK_HL2) -static-libgcc

INCLUDE += -I. -I.. -Isdk -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
	-I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook -I$(METAMOD)/sourcemm \
	-I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions \

CFLAGS += -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -Wno-switch \
	-Wno-unused -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fno-threadsafe-statics 

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug.$(ENGINE)
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release.$(ENGINE)
	CFLAGS += $(C_OPT_FLAGS)
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)
ifeq "$(GCC_VERSION)" "4"
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

BINARY = $(PROJECT).ext.so

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/sdk
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	$(MAKE) -f Makefile extension

extension: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean:
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/sdk/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
