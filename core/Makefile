# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ..
SRCDS_BASE = ~/srcds
HL2SDK_ORIG = ../../hl2sdk
HL2SDK_OB = ../../hl2sdk-ob
SOURCEMM14 = ../../sourcemm-1.4
SOURCEMM16 = ../../sourcemm-1.6

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

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
	smn_adt_trie.cpp smn_hudtext.cpp
OBJECTS += systems/ExtensionSys.cpp systems/ForwardSys.cpp systems/HandleSys.cpp \
	systems/LibrarySys.cpp systems/PluginInfoDatabase.cpp systems/PluginSys.cpp \
	systems/ShareSys.cpp vm/sp_vm_basecontext.cpp vm/sp_vm_engine.cpp \
	vm/sp_vm_function.cpp
OBJECTS += thread/ThreadWorker.cpp thread/BaseWorker.cpp thread/PosixThreads.cpp ThreadSupport.cpp
OBJECTS += zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzio.c \
	   zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c \
	   zlib/uncompr.c zlib/zutil.c

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -s -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc-4.1

override ENGSET = false
ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2PUB = $(HL2SDK_ORIG)/public
	HL2LIB = $(HL2SDK_ORIG)/linux_sdk
	METAMOD = $(SOURCEMM14)
	INCLUDE += -I$(HL2SDK)/public/dlls
	SRCDS = $(SRCDS_BASE)
	BINARY = sourcemod.1.ep1.so
	override ENGSET = true
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2PUB = $(HL2SDK_OB)/public
	HL2LIB = $(HL2SDK_OB)/linux_sdk
	CFLAGS += -DORANGEBOX_BUILD
	METAMOD = $(SOURCEMM16)
	INCLUDE += -I$(HL2SDK)/public/game/server
	SRCDS = $(SRCDS_BASE)/orangebox
	BINARY = sourcemod.2.ep2.so
	override ENGSET = true
endif

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a vstdlib_i486.so \
	tier0_i486.so -lpthread -static-libgcc

INCLUDE_SMSDK = -I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn

INCLUDE_SM16 = -I. -I.. -I$(SOURCEMM16)/sourcehook $(INCLUDE_SMSDK)
	
INCLUDE += -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/mathlib -I$(HL2PUB)/vstdlib \
	-I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook \
	-I$(METAMOD)/sourcemm -Isystems $(INCLUDE_SMSDK)

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror \
	-Wno-uninitialized -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -DSM_DEFAULT_THREADER -m32
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti

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
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%vm_engine.cpp=$(BIN_DIR)/%vm_engine.o)
OBJ_LINUX := $(OBJ_LINUX:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJ_LINUX:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%vm_engine.o: %vm_engine.cpp
	$(CPP) $(INCLUDE_SM16) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all: check
	mkdir -p $(BIN_DIR)/systems
	mkdir -p $(BIN_DIR)/vm
	mkdir -p $(BIN_DIR)/zlib
	mkdir -p $(BIN_DIR)/thread
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so;
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so;
	$(MAKE) -f Makefile sourcemod

check:
	if [ "$(ENGSET)" == "false" ]; then \
		echo "You must supply ENGINE=orangebox or ENGINE=original"; \
		exit 1; \
	fi

sourcemod: check $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean: check
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/systems/*.o
	rm -rf $(BIN_DIR)/zlib/*.o
	rm -rf $(BIN_DIR)/vm/*.o
	rm -rf $(BIN_DIR)/thread/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
