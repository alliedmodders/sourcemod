#(C)2004-2008 Metamod:Source Development Team
# Makefile written by David "BAILOPAN" Anderson

SOURCEMM = ../../../sourcemm-1.6
SMSDK = ../..

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = regex

OBJECTS = sdk/smsdk_ext.cpp extension.cpp CRegEx.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -g -ggdb3
CPP_GCC4_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
CPP = gcc-4.1

LINK = lib_linux/libpcre.a -static-libgcc

INCLUDE += -I. -I.. -Isdk -I$(HL2PUB) -I$(SOURCEMM)/sourcehook -I$(SMSDK)/public \
	-I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions

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
