#(C)2004-2008 Metamod:Source Development Team
# Makefile written by David "BAILOPAN" Anderson

#Local paths
HL2SDK_ORIG = ../../hl2sdk
HL2SDK_OB = ../../hl2sdk-ob
SOURCEMM14 = ../../sourcemm-1.4
SOURCEMM16 = ../../sourcemm-1.6
SRCDS_BASE = ~/srcds
SMSDK = ..

#Project options
OPT_FLAGS = -O3 -funroll-loops -s -pipe -fno-strict-aliasing
GCC4_FLAGS = -fvisibility=hidden 
GCC4_CPP_FLAGS = -fvisibility-inlines-hidden
DEBUG_FLAGS = -g -ggdb3
CPP = gcc-4.1


OBJECTS = AdminCache.cpp CDataPack.cpp ConCmdManager.cpp ConVarManager.cpp CoreConfig.cpp \
	Database.cpp DebugReporter.cpp EventManager.cpp GameConfigs.cpp HalfLife2.cpp Logger.cpp \
	MemoryUtils.cpp PlayerManager.cpp TextParsers.cpp TimerSys.cpp Translator.cpp UserMessages.cpp \
	sm_autonatives.cpp sm_memtable.cpp sm_srvcmds.cpp sm_stringutil.cpp sm_trie.cpp \
	sourcemm_api.cpp sourcemod.cpp MenuStyle_Base.cpp MenuStyle_Valve.cpp MenuManager.cpp \
	MenuStyle_Radio.cpp ChatTriggers.cpp ADTFactory.cpp MenuVoting.cpp sm_crc32.cpp \
	frame_hooks.cpp concmd_cleaner.cpp Profiler.cpp
OBJECTS += smn_admin.cpp smn_bitbuffer.cpp smn_console.cpp smn_core.cpp \
	smn_datapacks.cpp smn_entities.cpp smn_events.cpp smn_fakenatives.cpp \
	smn_filesystem.cpp smn_float.cpp smn_functions.cpp smn_gameconfigs.cpp smn_halflife.cpp \
	smn_handles.cpp smn_keyvalues.cpp smn_banning.cpp smn_maplists.cpp \
	smn_lang.cpp smn_player.cpp smn_string.cpp smn_sorting.cpp smn_textparse.cpp smn_timers.cpp \
	smn_usermsgs.cpp smn_menus.cpp smn_database.cpp smn_vector.cpp smn_adt_array.cpp \
	smn_adt_trie.cpp
OBJECTS += systems/ExtensionSys.cpp systems/ForwardSys.cpp systems/HandleSys.cpp \
	systems/LibrarySys.cpp systems/PluginInfoDatabase.cpp systems/PluginSys.cpp \
	systems/ShareSys.cpp vm/sp_vm_basecontext.cpp vm/sp_vm_function.cpp
OBJECTS += thread/ThreadWorker.cpp thread/BaseWorker.cpp thread/PosixThreads.cpp ThreadSupport.cpp
OBJECTS_C = zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzio.c \
	   zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c \
	   zlib/uncompr.c zlib/zutil.c
OBJECTS_SM16 = vm/sp_vm_engine.cpp

ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2PUB = $(HL2SDK_ORIG)/public
	HL2LIB = $(HL2SDK)/linux_sdk
	METAMOD = $(SOURCEMM14)
	INCLUDE += -I$(HL2PUB)/dlls
	SRCDS = $(SRCDS_BASE)
	BINARY = sourcemod.1.ep1.so
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2PUB = $(HL2SDK_OB)/public
	HL2LIB = $(HL2SDK)/linux_sdk
	CFLAGS += -DORANGEBOX_BUILD
	METAMOD = $(SOURCEMM16)
	INCLUDE += -I$(HL2PUB)/game/server
	SRCDS = $(SRCDS_BASE)/orangebox
	BINARY = sourcemod.2.ep2.so
endif
ifeq "$(BINARY)" ""
	echo "You must supply ENGINE=orangebox or ENGINE=original"
	false
endif

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a vstdlib_i486.so \
	tier0_i486.so -lpthread -static-libgcc

INCLUDE_SMSDK = -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn -I$(SMSDK)/public/extensions 

INCLUDE += -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/dlls -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 \
	-I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook -I$(METAMOD)/sourcemm \
	-I$(HL2PUB)/mathlib -Isystems $(INCLUDE_SMSDK)

SM16_INCLUDE = -I. -I.. -I$(SOURCEMM16)/sourcehook $(INCLUDE_SMSDK)

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug.$(ENGINE)
	CFLAGS += $(DEBUG_FLAGS)
else
	BIN_DIR = Release.$(ENGINE)
	CFLAGS += $(OPT_FLAGS)
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)

CFLAGS += -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp \
	-Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca \
	-Dstrcmpi=strcasecmp -Wall -Wno-uninitialized -Werror -mfpmath=sse -msse \
	-DSOURCEMOD_BUILD -DSM_DEFAULT_THREADER -m32
CPPFLAGS = -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

ifeq "$(GCC_VERSION)" "4"
	CFLAGS += $(GCC4_FLAGS)
	CPPFLAGS += $(GCC4_CPP_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.ox)
OBJ_LINUX_C := $(OBJECTS_C:%.c=$(BIN_DIR)/%.oc)
OBJ_LINUX_SM16 := $(OBJECTS_SM16:%.cpp=$(BIN_DIR)/%.ok)

$(BIN_DIR)/%.ox: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.oc: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

$(BIN_DIR)/%.ok: %.cpp
	$(CPP) $(SM16_INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)/systems
	mkdir -p $(BIN_DIR)/vm
	mkdir -p $(BIN_DIR)/zlib
	mkdir -p $(BIN_DIR)/thread
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	$(MAKE) -f Makefile sourcemod

sourcemod: $(OBJ_LINUX) $(OBJ_LINUX_C) $(OBJ_LINUX_SM16)
	$(CPP) $(OBJ_LINUX) $(OBJ_LINUX_C) $(OBJ_LINUX_SM16) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

default: all

clean:
	rm -rf $(BIN_DIR)/*.ox
	rm -rf $(BIN_DIR)/systems/*.ox
	rm -rf $(BIN_DIR)/zlib/*.oc
	rm -rf $(BIN_DIR)/vm/*.ox
	rm -rf $(BIN_DIR)/thread/*.ox
	rm -rf $(BIN_DIR)/$(BINARY)

