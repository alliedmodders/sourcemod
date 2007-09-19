#(C)2004-2006 SourceMM Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../..
SRCDS = ~/srcds
SOURCEMM = ../../../sourcemm

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = dbi.sqlite

#Uncomment for SourceMM-enabled extensions
#LINK_HL2 = $(HL2LIB)/tier1_i486.a vstdlib_i486.so tier0_i486.so 

OBJECTS = sdk/smsdk_ext.cpp extension.cpp sm_memtable.cpp \
	driver/SqDatabase.cpp driver/SqDriver.cpp driver/SqQuery.cpp \
	driver/SqResults.cpp

OBJECTS += sqlite-source/alter.c sqlite-source/analyze.c \
	sqlite-source/attach.c sqlite-source/auth.c sqlite-source/btree.c \
	sqlite-source/build.c sqlite-source/callback.c sqlite-source/complete.c \
	sqlite-source/date.c sqlite-source/delete.c sqlite-source/expr.c \
	sqlite-source/func.c sqlite-source/hash.c sqlite-source/insert.c \
	sqlite-source/legacy.c sqlite-source/main.c sqlite-source/malloc.c \
	sqlite-source/opcodes.c sqlite-source/os.c sqlite-source/os_unix.c \
	sqlite-source/pager.c sqlite-source/parse.c sqlite-source/pragma.c \
	sqlite-source/prepare.c sqlite-source/printf.c sqlite-source/random.c \
	sqlite-source/select.c sqlite-source/table.c sqlite-source/tokenize.c \
	sqlite-source/trigger.c sqlite-source/update.c sqlite-source/utf.c \
	sqlite-source/util.c sqlite-source/vacuum.c sqlite-source/vdbe.c \
	sqlite-source/vdbeapi.c sqlite-source/vdbeaux.c sqlite-source/vdbeblob.c \
	sqlite-source/vdbefifo.c sqlite-source/vdbemem.c sqlite-source/vtab.c \
	sqlite-source/where.c

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -g -ggdb3
CPP_GCC4_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
CPP = gcc-4.1

HL2PUB = $(HL2SDK)/public
HL2LIB = $(HL2SDK)/linux_sdk
HL2SDK = $(SOURCEMM)/hl2sdk

LINK = $(LINK_HL2) -static-libgcc -lpthread

INCLUDE = -I. -I.. -Isdk -I$(HL2PUB) -I$(HL2PUB)/dlls -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
          -I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1 -I$(SOURCEMM) -I$(SOURCEMM)/sourcehook -I$(SOURCEMM)/sourcemm \
	  -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions \

CFLAGS = -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -fPIC -mfpmath=sse -msse -DSOURCEMOD_BUILD -Wno-uninitialized -DTHREADSAFE -DSQLITE_OMIT_LOAD_EXTENSION
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

BINARY = $(PROJECT).ext.so

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJ_LINUX:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/sdk
	mkdir -p $(BIN_DIR)/driver
	mkdir -p $(BIN_DIR)/sqlite-source
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	$(MAKE) extension

extension: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) $(OBJ_LINUX) $(LINK) -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:	
	$(MAKE) all DEBUG=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/sdk/*.o
	rm -rf Release/sqlite-source/*.o
	rm -rf Release/driver/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/sdk/*.o
	rm -rf Debug/sqlite-source/*.o
	rm -rf Debug/driver/*.o
	rm -rf Debug/$(BINARY)
