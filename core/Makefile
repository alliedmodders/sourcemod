# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ..
SRCDS_BASE = ~/srcds
HL2SDK_ORIG = ../../hl2sdk
HL2SDK_OB = ../../hl2sdk-ob
HL2SDK_OB_VALVE = ../../hl2sdk-ob-valve
HL2SDK_L4D = ../../hl2sdk-l4d
MMSOURCE17 = ../../mmsource-1.7

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

OBJECTS = AdminCache.cpp CDataPack.cpp ConCmdManager.cpp ConVarManager.cpp CoreConfig.cpp \
	Database.cpp DebugReporter.cpp EventManager.cpp GameConfigs.cpp HalfLife2.cpp Logger.cpp \
	MemoryUtils.cpp PlayerManager.cpp TextParsers.cpp TimerSys.cpp Translator.cpp UserMessages.cpp \
	sm_autonatives.cpp sm_memtable.cpp sm_srvcmds.cpp sm_stringutil.cpp sm_trie.cpp \
	sourcemm_api.cpp sourcemod.cpp MenuStyle_Base.cpp MenuStyle_Valve.cpp MenuManager.cpp \
	MenuStyle_Radio.cpp ChatTriggers.cpp ADTFactory.cpp MenuVoting.cpp sm_crc32.cpp \
	frame_hooks.cpp concmd_cleaner.cpp Profiler.cpp PhraseCollection.cpp NextMap.cpp \
	NativeOwner.cpp
OBJECTS += smn_admin.cpp smn_bitbuffer.cpp smn_console.cpp smn_core.cpp \
	smn_datapacks.cpp smn_entities.cpp smn_events.cpp smn_fakenatives.cpp \
	smn_filesystem.cpp smn_float.cpp smn_functions.cpp smn_gameconfigs.cpp smn_halflife.cpp \
	smn_handles.cpp smn_keyvalues.cpp smn_banning.cpp smn_maplists.cpp \
	smn_lang.cpp smn_player.cpp smn_string.cpp smn_sorting.cpp smn_textparse.cpp smn_timers.cpp \
	smn_usermsgs.cpp smn_menus.cpp smn_database.cpp smn_vector.cpp smn_adt_array.cpp \
	smn_adt_trie.cpp smn_hudtext.cpp smn_adt_stack.cpp smn_nextmap.cpp
OBJECTS += ExtensionSys.cpp 		\
		   ForwardSys.cpp 			\
		   HandleSys.cpp 			\
		   LibrarySys.cpp 			\
		   PluginInfoDatabase.cpp 	\
		   PluginSys.cpp 			\
		   ShareSys.cpp				\
		   NativeInvoker.cpp
OBJECTS += thread/ThreadWorker.cpp thread/BaseWorker.cpp thread/PosixThreads.cpp ThreadSupport.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

override ENGSET = false
ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2PUB = $(HL2SDK)/public
	HL2LIB = $(HL2SDK)/linux_sdk
	CFLAGS += -DSOURCE_ENGINE=1
	METAMOD = $(MMSOURCE17)/core-legacy
	INCLUDE += -I$(HL2SDK)/public/dlls
	SRCDS = $(SRCDS_BASE)
	BINARY = sourcemod.1.ep1.so
	override ENGSET = true
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2PUB = $(HL2SDK)/public
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=3
	METAMOD = $(MMSOURCE17)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	SRCDS = $(SRCDS_BASE)/orangebox
	BINARY = sourcemod.2.ep2.so
	override ENGSET = true
endif
ifeq "$(ENGINE)" "orangeboxvalve"
	HL2SDK = $(HL2SDK_OB_VALVE)
	HL2PUB = $(HL2SDK)/public
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=4
	METAMOD = $(MMSOURCE17)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	SRCDS = $(SRCDS_BASE)/orangebox
	BINARY = sourcemod.2.ep2v.so
	override ENGSET = true
endif
ifeq "$(ENGINE)" "left4dead"
	HL2SDK = $(HL2SDK_L4D)
	HL2PUB = $(HL2SDK)/public
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=5
	METAMOD = $(MMSOURCE17)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	SRCDS = $(SRCDS_BASE)/l4d
	BINARY = sourcemod.2.l4d.so
	override ENGSET = true
endif

CFLAGS += -DSE_EPISODEONE=1 -DSE_DARKMESSIAH=2 -DSE_ORANGEBOX=3 -DSE_ORANGEBOXVALVE=4 -DSE_LEFT4DEAD=5

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a vstdlib_i486.so \
	tier0_i486.so -lpthread -static-libgcc
	
INCLUDE += -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/mathlib -I$(HL2PUB)/vstdlib \
	-I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook \
	-I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn

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

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)
OBJ_LINUX := $(OBJ_LINUX:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/%.o: %.c
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all: check
	mkdir -p $(BIN_DIR)/systems
	mkdir -p $(BIN_DIR)/thread
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so;
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so;
	$(MAKE) -f Makefile sourcemod

check:
	if [ "$(ENGSET)" = "false" ]; then \
		echo "You must supply ENGINE=left4dead or ENGINE=orangebox or ENGINE=original"; \
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
	rm -rf $(BIN_DIR)/thread/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
