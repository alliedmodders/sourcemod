#include "asm.h"
#include "libudis86/udis86.h"

#ifndef WIN32
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libudis86/udis86.h"

#define REG_EAX			0
#define REG_ECX			1
#define REG_EDX			2
#define REG_EBX			3

#define IA32_MOV_REG_IMM		0xB8	// encoding is +r <imm32>
#endif

/**
* Checks if a call to a fpic thunk has just been written into dest.
* If found replaces it with a direct mov that sets the required register to the value of pc.
*
* @param dest		Destination buffer where a call opcode + addr (5 bytes) has just been written.
* @param pc		The program counter value that needs to be set (usually the next address from the source).
* @noreturn
*/
void check_thunks(unsigned char *dest, unsigned char *pc)
{
#if defined(_WIN32) || defined(__x86_64__)
	return;
#else
	/* Step write address back 4 to the start of the function address */
	unsigned char *writeaddr = dest - 4;
	unsigned char *calloffset = *(unsigned char **)writeaddr;
	unsigned char *calladdr = (unsigned char *)(dest + (unsigned int)calloffset);

	/* Lookup name of function being called */
	if ((*calladdr == 0x8B) && (*(calladdr+2) == 0x24) && (*(calladdr+3) == 0xC3))
	{
		//a thunk maybe?
		char movByte = IA32_MOV_REG_IMM;

		/* Calculate the correct mov opcode */
		switch (*(calladdr+1))
		{
		case 0x04:
			{
				movByte += REG_EAX;
				break;
			}
		case 0x1C:
			{
				movByte += REG_EBX;
				break;
			}
		case 0x0C:
			{
				movByte += REG_ECX;
				break;
			}
		case 0x14:
			{
				movByte += REG_EDX;
				break;
			}
		default:
			{
				printf("Unknown thunk: %c\n", *(calladdr+1));
#ifndef NDEBUG
				abort();
#endif
				break;
			}
		}

		/* Move our write address back one to where the call opcode was */
		writeaddr--;


		/* Write our mov */
		*writeaddr = movByte;
		writeaddr++;

		/* Write the value - The provided program counter value */
		*(void **)writeaddr = (void *)pc;
		writeaddr += 4;
	}
#endif
}

int copy_bytes(unsigned char *func, unsigned char *dest, int required_len)
{
	ud_t ud_obj;
	ud_init(&ud_obj);

#if defined(_WIN64) || defined(__x86_64__)
	ud_set_mode(&ud_obj, 64);
#else
	ud_set_mode(&ud_obj, 32);
#endif

	ud_set_input_buffer(&ud_obj, func, 20);
	unsigned int bytecount = 0;
	
	while (bytecount < required_len && ud_disassemble(&ud_obj))
	{
		unsigned int insn_len = ud_insn_len(&ud_obj);
		bytecount += insn_len;
		
		if (dest)
		{	
			const uint8_t *opcode = ud_insn_ptr(&ud_obj);
			if ((opcode[0] & 0xFE) == 0xE8)	// Fix CALL/JMP offset
			{
				dest[0] = func[0];
				dest++; func++;
				if (ud_insn_opr(&ud_obj, 0)->size == 32)
				{
					*(int32_t *)dest = func + *(int32_t *)func - dest;
					check_thunks(dest+4, func+4);
					dest += sizeof(int32_t);
				}
				else
				{
					*(int16_t *)dest = func + *(int16_t *)func - dest;
					dest += sizeof(int16_t);
				}
				func--;
			}
			else
			{
				memcpy(dest, func, insn_len);
				dest += insn_len;
			}
		}

		func += insn_len;
	}
	
	return bytecount;
}

#if 0
//if dest is NULL, returns minimum number of bytes needed to be copied
//if dest is not NULL, it will copy the bytes to dest as well as fix CALLs and JMPs
//http://www.devmaster.net/forums/showthread.php?t=2311
int copy_bytes(unsigned char *func, unsigned char* dest, int required_len) {
	int bytecount = 0;

	while(bytecount < required_len && *func != 0xCC)
	{
		// prefixes F0h, F2h, F3h, 66h, 67h, D8h-DFh, 2Eh, 36h, 3Eh, 26h, 64h and 65h
		int operandSize = 4;
		int FPU = 0;
		int twoByte = 0;
		unsigned char opcode = 0x90;
		unsigned char modRM = 0xFF;
		while(*func == 0xF0 ||
			  *func == 0xF2 ||
			  *func == 0xF3 ||
			 (*func & 0xFC) == 0x64 ||
			 (*func & 0xF8) == 0xD8 ||
			 (*func & 0x7E) == 0x62)
		{
			if(*func == 0x66)
			{
				operandSize = 2;
			}
			else if((*func & 0xF8) == 0xD8)
			{
				FPU = *func;
				if (dest)
						*dest++ = *func++;
				else
						func++;
				bytecount++;
				break;
			}

			if (dest)
				*dest++ = *func++;
			else
				func++;
			bytecount++;
		}

		// two-byte opcode byte
		if(*func == 0x0F)
		{
			twoByte = 1;
			if (dest)
				*dest++ = *func++;
			else
				func++;
			bytecount++;
		}

		// opcode byte
		opcode = *func++;
		if (dest) *dest++ = opcode;
		bytecount++;

		// mod R/M byte
		modRM = 0xFF;
		if(FPU)
		{
			if((opcode & 0xC0) != 0xC0)
			{
				modRM = opcode;
			}
		}
		else if(!twoByte)
		{
			if((opcode & 0xC4) == 0x00 ||
			   ((opcode & 0xF4) == 0x60 && ((opcode & 0x0A) == 0x02 || (opcode & 0x09) == 0x09)) ||
			   (opcode & 0xF0) == 0x80 ||
			   ((opcode & 0xF8) == 0xC0 && (opcode & 0x0E) != 0x02) ||
			   (opcode & 0xFC) == 0xD0 ||
			   (opcode & 0xF6) == 0xF6)
			{
				modRM = *func++;
				if (dest) *dest++ = modRM;
				bytecount++;
			}
		}
		else
		{
			if(((opcode & 0xF0) == 0x00 && (opcode & 0x0F) >= 0x04 && (opcode & 0x0D) != 0x0D) ||
			   (opcode & 0xF0) == 0x30 ||
			   opcode == 0x77 ||
			   (opcode & 0xF0) == 0x80 ||
			   ((opcode & 0xF0) == 0xA0 && (opcode & 0x07) <= 0x02) ||
			   (opcode & 0xF8) == 0xC8)
			{
				// No mod R/M byte
			}
			else
			{
				modRM = *func++;
				if (dest) *dest++ = modRM;
				bytecount++;
			}
		}

		// SIB
		if((modRM & 0x07) == 0x04 &&
		   (modRM & 0xC0) != 0xC0)
		{
			if (dest)
				*dest++ = *func++;   //SIB
			else
				func++;
			bytecount++;
		}

		// mod R/M displacement

		// Dword displacement, no base
	if((modRM & 0xC5) == 0x05) {
		if (dest) {
			*(unsigned int*)dest = *(unsigned int*)func;
			dest += 4;
		}
		func += 4;
		bytecount += 4;
	}

		// Byte displacement
	if((modRM & 0xC0) == 0x40) {
		if (dest)
			*dest++ = *func++;
		else
			func++;
		bytecount++;
	}

		// Dword displacement
	if((modRM & 0xC0) == 0x80) {
		if (dest) {
			*(unsigned int*)dest = *(unsigned int*)func;
			dest += 4;
		}
		func += 4;
		bytecount += 4;
	}

		// immediate
		if(FPU)
		{
			// Can't have immediate operand
		}
		else if(!twoByte)
		{
			if((opcode & 0xC7) == 0x04 ||
			   (opcode & 0xFE) == 0x6A ||   // PUSH/POP/IMUL
			   (opcode & 0xF0) == 0x70 ||   // Jcc
			   opcode == 0x80 ||
			   opcode == 0x83 ||
			   (opcode & 0xFD) == 0xA0 ||   // MOV
			   opcode == 0xA8 ||			// TEST
			   (opcode & 0xF8) == 0xB0 ||   // MOV
			   (opcode & 0xFE) == 0xC0 ||   // RCL
			   opcode == 0xC6 ||			// MOV
			   opcode == 0xCD ||			// INT
			   (opcode & 0xFE) == 0xD4 ||   // AAD/AAM
			   (opcode & 0xF8) == 0xE0 ||   // LOOP/JCXZ
			   opcode == 0xEB ||
			   (opcode == 0xF6 && (modRM & 0x30) == 0x00))   // TEST
			{
				if (dest)
					*dest++ = *func++;
				else
					func++;
				bytecount++;
			}
		else if((opcode & 0xF7) == 0xC2) // RET
			{
				if (dest) {
					*(unsigned short*)dest = *(unsigned short*)func;
					dest += 2;
				}
				func += 2;
				bytecount += 2;
			}
			else if((opcode & 0xFC) == 0x80 ||
					(opcode & 0xC7) == 0x05 ||
					(opcode & 0xF8) == 0xB8 ||
					(opcode & 0xFE) == 0xE8 ||	  // CALL/Jcc
					(opcode & 0xFE) == 0x68 ||
					(opcode & 0xFC) == 0xA0 ||
					(opcode & 0xEE) == 0xA8 ||
					opcode == 0xC7 ||
					(opcode == 0xF7 && (modRM & 0x30) == 0x00))
			{
				if (dest) {
					//Fix CALL/JMP offset
					if ((opcode & 0xFE) == 0xE8) {
						if (operandSize == 4)
						{
							*(long*)dest = ((func + *(long*)func) - dest);

							//pRED* edit. func is the current address of the call address, +4 is the next instruction, so the value of $pc
							check_thunks(dest+4, func+4);
						}
						else
							*(short*)dest = ((func + *(short*)func) - dest);

					} else {
						if (operandSize == 4)
							*(unsigned long*)dest = *(unsigned long*)func;
						else
							*(unsigned short*)dest = *(unsigned short*)func;
					}
					dest += operandSize;
				}
				func += operandSize;
				bytecount += operandSize;

			}
		}
		else
		{
			if(opcode == 0xBA ||			// BT
			   opcode == 0x0F ||			// 3DNow!
			   (opcode & 0xFC) == 0x70 ||   // PSLLW
			   (opcode & 0xF7) == 0xA4 ||   // SHLD
			   opcode == 0xC2 ||
			   opcode == 0xC4 ||
			   opcode == 0xC5 ||
			   opcode == 0xC6)
			{
				if (dest)
					*dest++ = *func++;
				else
					func++;
			}
			else if((opcode & 0xF0) == 0x80) // Jcc -i
			{
				if (dest) {
					if (operandSize == 4)
						*(unsigned long*)dest = *(unsigned long*)func;
					else
						*(unsigned short*)dest = *(unsigned short*)func;

					dest += operandSize;
				}
				func += operandSize;
				bytecount += operandSize;
			}
		}
	}

	return bytecount;
}
#endif

//insert a specific JMP instruction at the given location
void inject_jmp(void* src, void* dest) {
	*(unsigned char*)src = OP_JMP;
	*(long*)((unsigned char*)src+1) = (long)((unsigned char*)dest - ((unsigned char*)src + OP_JMP_SIZE));
}

//fill a given block with NOPs
void fill_nop(void* src, unsigned int len) {
	unsigned char* src2 = (unsigned char*)src;
	while (len) {
		*src2++ = OP_NOP;
		--len;
	}
}

void* eval_jump(void* src) {
	unsigned char* addr = (unsigned char*)src;

	if (!addr) return 0;

	//import table jump
	if (addr[0] == OP_PREFIX && addr[1] == OP_JMP_SEG) {
		addr += 2;
		addr = *(unsigned char**)addr;
		//TODO: if addr points into the IAT
		return *(void**)addr;
	}

	//8bit offset
	else if (addr[0] == OP_JMP_BYTE) {
		addr = &addr[OP_JMP_BYTE_SIZE] + *(char*)&addr[1];
		//mangled 32bit jump?
		if (addr[0] == OP_JMP) {
			addr = addr + *(int*)&addr[1];
		}
		return addr;
	}
	/*
	//32bit offset
	else if (addr[0] == OP_JMP) {
		addr = &addr[OP_JMP_SIZE] + *(int*)&addr[1];
	}
	*/

	return addr;
}
/*
from ms detours package
static bool detour_is_imported(PBYTE pbCode, PBYTE pbAddress)
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((PVOID)pbCode, &mbi, sizeof(mbi));
	__try {
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)mbi.AllocationBase;
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			return false;
		}

		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
														  pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
			return false;
		}

		if (pbAddress >= ((PBYTE)pDosHeader +
						  pNtHeader->OptionalHeader
						  .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress) &&
			pbAddress < ((PBYTE)pDosHeader +
						 pNtHeader->OptionalHeader
						 .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress +
						 pNtHeader->OptionalHeader
						 .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size)) {
			return true;
		}
		return false;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}
*/
