#ifndef __ASM_H__
#define __ASM_H__

#define OP_JMP				0xE9
#define OP_JMP_SIZE			5
#define X64_ABS_SIZE		14

#define OP_NOP				0x90
#define OP_NOP_SIZE			1

#define OP_PREFIX			0xFF
#define OP_JMP_SEG			0x25

#define OP_JMP_BYTE			0xEB
#define OP_JMP_BYTE_SIZE	2

#ifdef __cplusplus
extern "C" {
#endif

void check_thunks(unsigned char *dest, unsigned char *pc);

//if dest is NULL, returns minimum number of bytes needed to be copied
//if dest is not NULL, it will copy the bytes to dest as well as fix CALLs and JMPs
//http://www.devmaster.net/forums/showthread.php?t=2311
int copy_bytes(unsigned char *func, unsigned char* dest, int required_len);

//insert a specific JMP instruction at the given location
void inject_jmp(void* src, void* dest);

//fill a given block with NOPs
void fill_nop(void* src, unsigned int len);

//evaluate a JMP at the target
void* eval_jump(void* src);

#ifdef __cplusplus
}
#endif

#endif //__ASM_H__
