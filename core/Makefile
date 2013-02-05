# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ..
HL2SDK_ORIG = ../../hl2sdk
HL2SDK_OB = ../../hl2sdk-ob
HL2SDK_CSS = ../../hl2sdk-css
HL2SDK_OB_VALVE = ../../hl2sdk-ob-valve
HL2SDK_L4D = ../../hl2sdk-l4d
HL2SDK_L4D2 = ../../hl2sdk-l4d2
HL2SDK_CSGO = ../../hl2sdk-csgo
MMSOURCE = ../../mmsource-central

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

OBJECTS = AdminCache.cpp CDataPack.cpp ConCmdManager.cpp ConVarManager.cpp CoreConfig.cpp \
	Database.cpp DebugReporter.cpp EventManager.cpp HalfLife2.cpp Logger.cpp \
	PlayerManager.cpp TimerSys.cpp UserMessages.cpp \
	sm_autonatives.cpp sm_srvcmds.cpp sm_stringutil.cpp sm_trie.cpp \
	sourcemm_api.cpp sourcemod.cpp MenuStyle_Base.cpp MenuStyle_Valve.cpp MenuManager.cpp \
	MenuStyle_Radio.cpp ChatTriggers.cpp ADTFactory.cpp MenuVoting.cpp \
	frame_hooks.cpp concmd_cleaner.cpp NextMap.cpp \
	NativeOwner.cpp logic_bridge.cpp ConsoleDetours.cpp
OBJECTS += smn_bitbuffer.cpp smn_console.cpp smn_core.cpp \
	smn_database.cpp smn_entities.cpp smn_events.cpp \
	smn_fakenatives.cpp smn_filesystem.cpp smn_halflife.cpp \
	smn_keyvalues.cpp smn_player.cpp \
	smn_usermsgs.cpp smn_menus.cpp smn_vector.cpp \
	smn_hudtext.cpp smn_nextmap.cpp
OBJECTS += ExtensionSys.cpp 		\
		   ForwardSys.cpp 			\
		   HandleSys.cpp 			\
		   LibrarySys.cpp 			\
		   PluginInfoDatabase.cpp 	\
		   PluginSys.cpp 			\
		   ShareSys.cpp				\
		   NativeInvoker.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

override ENGSET = false

ifneq (,$(filter original orangebox css orangeboxvalve left4dead left4dead2 csgo,$(ENGINE)))
	override ENGSET = true
endif

ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	HL2LIB = $(HL2SDK)/linux_sdk
	CFLAGS += -DSOURCE_ENGINE=1
	METAMOD = $(MMSOURCE)/core-legacy
	INCLUDE += -I$(HL2SDK)/public/dlls
	BINARY = sourcemod.1.ep1.so
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=3
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	BINARY = sourcemod.2.ep2.so
endif
ifeq "$(ENGINE)" "css"
	HL2SDK = $(HL2SDK_CSS)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=6
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server 
	BINARY = sourcemod.2.css.so
endif
ifeq "$(ENGINE)" "orangeboxvalve"
	HL2SDK = $(HL2SDK_OB_VALVE)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=7
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server 
	BINARY = sourcemod.2.ep2v.so
endif
ifeq "$(ENGINE)" "left4dead"
	HL2SDK = $(HL2SDK_L4D)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=8
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	BINARY = sourcemod.2.l4d.so
endif
ifeq "$(ENGINE)" "left4dead2"
	HL2SDK = $(HL2SDK_L4D2)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=9
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	BINARY = sourcemod.2.l4d2.so
endif
ifeq "$(ENGINE)" "csgo"
	HL2SDK = $(HL2SDK_CSGO)
	HL2LIB = $(HL2SDK)/lib/linux
	CFLAGS += -DSOURCE_ENGINE=12
	METAMOD = $(MMSOURCE)/core
	INCLUDE += -I$(HL2SDK)/public/game/server
	BINARY = sourcemod.2.csgo.so
endif

HL2PUB = $(HL2SDK)/public

OS := $(shell uname -s)

ifeq "$(OS)" "Darwin"
	LIB_EXT = dylib
	HL2LIB = $(HL2SDK)/lib/mac
else
	LIB_EXT = so
	ifeq "$(ENGINE)" "original"
		HL2LIB = $(HL2SDK)/linux_sdk
	else
		HL2LIB = $(HL2SDK)/lib/linux
	endif
endif

ifneq (,$(filter original orangebox left4dead,$(ENGINE)))
	LIB_SUFFIX = _i486.$(LIB_EXT)
else
	LIB_PREFIX = lib
	ifneq (,$(filter orangeboxvalve css,$(ENGINE)))
		ifneq "$(OS)" "Darwin"
			LIB_SUFFIX = _srv.$(LIB_EXT)
		else
			LIB_SUFFIX = .$(LIB_EXT)
		endif
	else
		LIB_SUFFIX = .$(LIB_EXT)
	endif
endif

CFLAGS += -DSE_EPISODEONE=1 -DSE_DARKMESSIAH=2 -DSE_ORANGEBOX=3 -DSE_BLOODYGOODTIME=4 -DSE_EYE=5 \
	-DSE_CSS=6 -DSE_ORANGEBOXVALVE=7 -DSE_LEFT4DEAD=8 -DSE_LEFT4DEAD2=9 -DSE_ALIENSWARM=10 \
	-DSE_PORTAL2=11 -DSE_CSGO=12

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a $(LIB_PREFIX)vstdlib$(LIB_SUFFIX) \
	$(LIB_PREFIX)tier0$(LIB_SUFFIX) -static-libgcc

ifeq "$(ENGINE)" "csgo"
	LINK += $(HL2LIB)/interfaces_i486.a
endif
	
INCLUDE += -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/mathlib -I$(HL2PUB)/vstdlib \
	-I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook \
	-I$(SMSDK)/public -I$(SMSDK)/public/sourcepawn

CFLAGS += -D_LINUX -DPOSIX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror \
	-Wno-uninitialized -Wno-unused -Wno-switch -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H \
	-DSM_DEFAULT_THREADER -m32 -DCOMPILER_GCC
CPPFLAGS += -Wno-non-virtual-dtor -Wno-overloaded-virtual -fno-threadsafe-statics -fno-exceptions -fno-rtti

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
	mkdir -p $(BIN_DIR)
	ln -sf $(HL2LIB)/$(LIB_PREFIX)vstdlib$(LIB_SUFFIX)
	ln -sf $(HL2LIB)/$(LIB_PREFIX)tier0$(LIB_SUFFIX)
	$(MAKE) -f Makefile sourcemod

check:
	if [ "$(ENGSET)" = "false" ]; then \
		echo "You must supply one of the following values for ENGINE:"; \
		echo "csgo, left4dead2, left4dead, orangeboxvalve, css, orangebox, or original"; \
		exit 1; \
	fi

sourcemod: check $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -m32 -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean: check
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
