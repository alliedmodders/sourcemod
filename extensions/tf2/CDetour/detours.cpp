#include "detours.h"
#include <asm/asm.h>

ISourcePawnEngine *CDetourManager::spengine = NULL;
IGameConfig *CDetourManager::gameconf = NULL;
int CDetourManager::returnValue = 0;

void CDetourManager::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	CDetourManager::spengine = spengine;
	CDetourManager::gameconf = gameconf;
}

CDetour *CDetourManager::CreateDetour(void *callbackfunction, size_t paramsize, const char *signame)
{
	CDetour *detour = new CDetour(callbackfunction, paramsize, signame);
	if (detour)
	{
		if (!detour->Init(spengine, gameconf))
		{
			delete detour;
			return NULL;
		}

		return detour;
	}

	return NULL;
}

void CDetourManager::DeleteDetour(CDetour *detour)
{
	delete detour;
}

CBlocker * CDetourManager::CreateFunctionBlock( const char *signame, bool isVoid )
{
	CBlocker *block = new CBlocker(signame, isVoid);

	if (block)
	{
		if (!block->Init(spengine, gameconf))
		{
			delete block;
			return NULL;
		}

		return block;
	}

	return NULL;
}

void CDetourManager::DeleteFunctionBlock(CBlocker *block)
{
	delete block;
}

CDetour::CDetour(void *callbackfunction, size_t paramsize, const char *signame)
{
	enabled = false;
	detoured = false;
	detour_address = NULL;
	detour_callback = NULL;
	this->signame = signame;
	this->callbackfunction = callbackfunction;
	spengine = NULL;
	gameconf = NULL;
	this->paramsize = paramsize;
}

CDetour::~CDetour()
{
	DeleteDetour();
}

bool CDetour::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	this->spengine = spengine;
	this->gameconf = gameconf;

	if (!CreateDetour())
	{
		enabled = false;
		return enabled;
	}

	enabled = true;

	return enabled;
}

bool CDetour::IsEnabled()
{
	return enabled;
}

bool CDetour::CreateDetour()
{
	if (!gameconf->GetMemSig(signame, &detour_address))
	{
		g_pSM->LogError(myself, "Could not locate %s - Disabling detour", signame);
		return false;
	}

	if (!detour_address)
	{
		g_pSM->LogError(myself, "Sigscan for %s failed - Disabling detour", signame);
		return false;
	}

	detour_restore.bytes = copy_bytes((unsigned char *)detour_address, NULL, OP_JMP_SIZE);

	/* First, save restore bits */
	for (size_t i=0; i<detour_restore.bytes; i++)
	{
		detour_restore.patch[i] = ((unsigned char *)detour_address)[i];
	}

	//detour_callback = spengine->ExecAlloc(100);
	JitWriter wr;
	JitWriter *jit = &wr;
	jit_uint32_t CodeSize = 0;
	
	wr.outbase = NULL;
	wr.outptr = NULL;

jit_rewind:

	/* Push all our params onto the stack */
	for (size_t i=0; i<paramsize; i++)
	{
#if defined PLATFORM_WINDOWS
		IA32_Push_Rm_Disp8_ESP(jit, (paramsize*4));
#elif defined PLATFORM_LINUX
		IA32_Push_Rm_Disp8_ESP(jit, 4 +(paramsize*4));
#endif
	}

	/* Push thisptr onto the stack */
#if defined PLATFORM_WINDOWS
	IA32_Push_Reg(jit, REG_ECX);
#elif defined PLATFORM_LINUX
	IA32_Push_Rm_Disp8_ESP(jit, 4 + (paramsize*4));
#endif

	jitoffs_t call = IA32_Call_Imm32(jit, 0); 
	IA32_Write_Jump32_Abs(jit, call, callbackfunction);

	/* Pop thisptr */
#if defined PLATFORM_LINUX
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);		//add esp, 4
#elif defined PLATFORM_WINDOWS
	IA32_Pop_Reg(jit, REG_ECX);
#endif

	/* Pop params from the stack */
	for (size_t i=0; i<paramsize; i++)
	{
		IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);	
	}

	//If TempDetour returns non-zero we want to load something into eax and return this value

	//test eax, eax
	IA32_Test_Rm_Reg(jit,  REG_EAX, REG_EAX, MOD_REG);

	//jnz _skip
	jitoffs_t jmp = IA32_Jump_Cond_Imm8(jit, CC_NZ, 0);

	/* Patch old bytes in */
	if (wr.outbase != NULL)
	{
		copy_bytes((unsigned char *)detour_address, (unsigned char*)wr.outptr, detour_restore.bytes);
	}
	wr.outptr += detour_restore.bytes;

	/* Return to the original function */
	call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)detour_address + detour_restore.bytes);
	
	//_skip:
	//mov eax, [g_returnvalue]
	//ret
	IA32_Send_Jump8_Here(jit, jmp);
	IA32_Mov_Eax_Mem(jit, (jit_int32_t)&CDetourManager::returnValue);
	IA32_Return(jit);

	if (wr.outbase == NULL)
	{
		CodeSize = wr.get_outputpos();
		wr.outbase = (jitcode_t)spengine->AllocatePageMemory(CodeSize);
		spengine->SetReadWrite(wr.outbase);
		wr.outptr = wr.outbase;
		detour_callback = wr.outbase;
		goto jit_rewind;
	}

	spengine->SetReadExecute(wr.outbase);

	return true;
}

void CDetour::EnableDetour()
{
	if (!detoured)
	{
		DoGatePatch((unsigned char *)detour_address, &detour_callback);
		detoured = true;
	}
}

void CDetour::DeleteDetour()
{
	if (detoured)
	{
		DisableDetour();
	}

	if (detour_callback)
	{
		/* Free the gate */
		spengine->ExecFree(detour_callback);
		detour_callback = NULL;
	}
}

void CDetour::DisableDetour()
{
	if (detoured)
	{
		/* Remove the patch */
		/* This may screw up */
		ApplyPatch(detour_address, 0, &detour_restore, NULL);
		detoured = false;
	}
}

CBlocker::CBlocker( const char *signame, bool isVoid )
{
	this->isVoid = isVoid;
	isEnabled = false;
	isValid = false;

	spengine = NULL;
	gameconf = NULL;
	block_address = NULL;
	block_sig = signame;

	if (isVoid)
	{
		/* Void functions we only patch in a 'ret' (1 byte) */
		block_restore.bytes = 1;
	}
	else
	{
		/* Normal functions need an mov eax, value */
		block_restore.bytes = 6;
	}
}

void CBlocker::EnableBlock( int returnValue )
{
	if (!isValid || isEnabled)
	{
		return;
	}

	/* First, save restore bits */
	for (size_t i=0; i<block_restore.bytes; i++)
	{
		block_restore.patch[i] = ((unsigned char *)block_address)[i];
	}

	JitWriter wr;
	JitWriter *jit = &wr;
	wr.outbase = (jitcode_t)block_address;
	wr.outptr = wr.outbase;

	if (isVoid)
	{
		IA32_Return(jit);
	}
	else
	{
		IA32_Mov_Reg_Imm32(jit, REG_EAX, returnValue);
		IA32_Return(jit);
	}

	isEnabled = true;
}

void CBlocker::DisableBlock()
{
	if (!isValid || !isEnabled)
	{
		return;
	}

	/* First, save restore bits */
	for (size_t i=0; i<block_restore.bytes; i++)
	{
		((unsigned char *)block_address)[i] = block_restore.patch[i];
	}

	isEnabled = false;
}

CBlocker::~CBlocker()
{
	if (!isValid || !isEnabled)
	{
		return;
	}

	DisableBlock();
}

bool CBlocker::Init( ISourcePawnEngine *spengine, IGameConfig *gameconf )
{
	this->spengine = spengine;
	this->gameconf = gameconf;

	if (!gameconf->GetMemSig(block_sig, &block_address))
	{
		g_pSM->LogError(myself, "Could not locate %s - Disabling blocker", block_sig);
		isValid = false;
		return false;
	}

	if (!block_address)
	{
		g_pSM->LogError(myself, "Sigscan for %s failed - Disabling blocker", block_sig);
		isValid = false;
		return false;
	}

	isValid = true;

	return true;
}
