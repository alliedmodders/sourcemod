/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#include "MemoryUtils.h"
#ifdef PLATFORM_LINUX
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define PAGE_SIZE			4096
#define PAGE_ALIGN_UP(x)	((x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#endif
#ifdef PLATFORM_APPLE
#include <mach-o/dyld_images.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#endif

MemoryUtils::MemoryUtils()
{
#ifdef PLATFORM_APPLE

	/* Get pointer to struct that describes all loaded mach-o images in process */
	struct nlist list[2];
	memset(list, 0, sizeof(list));
	list[0].n_un.n_name = (char *)"_dyld_all_image_infos";
	nlist("/usr/lib/dyld", list);
	m_ImageList = (struct dyld_all_image_infos *)list[0].n_value;

#endif
}

MemoryUtils::~MemoryUtils()
{
#if defined PLATFORM_LINUX || defined PLATFORM_APPLE
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		delete m_SymTables[i];
	}
	m_SymTables.clear();
#endif
}

void *MemoryUtils::FindPatternInFile(int fd, const char *pattern, size_t len, int &matches, bool &atFuncStart)
{
	size_t size;
	size = lseek (fd , 0 , SEEK_END);
	
	matches = 0;
	// TODO: fix this
	atFuncStart = true;

	lseek(fd, 0, SEEK_SET);

	void *map;
	char *ptr, *start, *end;
	bool found = true;
	map = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);

	ptr = (char *)map;
	start = ptr;
	end = ptr + size;
	
	void *firstMatch = NULL;

	while (ptr < end)
	{
		found = true;
		for (register size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			if( !firstMatch )
			{
				firstMatch = ptr;
				if( ptr - start >= 1 )
				{
					unsigned char oneBack = *(unsigned char*)((intptr_t)ptr - 1);
					if( oneBack != 0xC3 && oneBack != 0xCB && oneBack != 0xCC )
					{
						if( ptr - start >= 3 )
						{
							unsigned char threeBack = *(unsigned char*)((intptr_t)ptr - 3);
							if( threeBack != 0xCA && threeBack != 0xC2 )
							{
								atFuncStart = false;
							}
						}
						else
						{
							atFuncStart = false;
						}
					}
				}
			}
			
			++matches;
		}

		++ptr;
	}

	return firstMatch;
}

void *MemoryUtils::FindPattern(const void *libPtr, const char *pattern, size_t len, int &matches, bool &atFuncStart)
{
	DynLibInfo lib;
	bool found;
	char *ptr, *end;
	
	matches = 0;
	// TODO: fix this
	atFuncStart = true;

	memset(&lib, 0, sizeof(DynLibInfo));

	if (!GetLibraryInfo(libPtr, lib))
	{
		return NULL;
	}
	
	void *firstMatch = NULL;
	ptr = reinterpret_cast<char *>(lib.baseAddress);
	end = ptr + lib.memorySize - 1;

	while (ptr < end)
	{
		found = true;
		for (register size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			if( !firstMatch )
				firstMatch = ptr;
			
			++matches;
		}

		++ptr;
	}

	return firstMatch;
}

void *MemoryUtils::ResolveSymbol(void *handle, const char *symbol)
{
#ifdef PLATFORM_WINDOWS

	return GetProcAddress((HMODULE)handle, symbol);
	
#elif defined PLATFORM_LINUX

	struct link_map *dlmap;
	struct stat dlstat;
	int dlfile;
	uintptr_t map_base;
	Elf32_Ehdr *file_hdr;
	Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
	Elf32_Sym *symtab;
	const char *shstrtab, *strtab;
	uint16_t section_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table;
	AddrTable *table2;
	Symbol *symbol_entry;

	dlmap = (struct link_map *)handle;
	symtab_hdr = NULL;
	strtab_hdr = NULL;
	table = NULL;
	table2 = NULL;
	
	/* See if we already have a symbol table for this library */
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		libtable = m_SymTables[i];
		if (libtable->lib_base == dlmap->l_addr)
		{
			table = &libtable->table;
			table2 = &libtable->table2;
			break;
		}
	}

	/* If we don't have a symbol table for this library, then create one */
	if (table == NULL)
	{
		libtable = new LibSymbolTable();
		libtable->table.Initialize();
		libtable->table2.Initialize();
		libtable->lib_base = dlmap->l_addr;
		libtable->last_pos = 0;
		table = &libtable->table;
		table2 = &libtable->table2;
		m_SymTables.push_back(libtable);
	}

	/* See if the symbol is already cached in our table */
	symbol_entry = table->FindSymbol(symbol, strlen(symbol));
	if (symbol_entry != NULL)
	{
		return symbol_entry->address;
	}

	/* If symbol isn't in our table, then we have open the actual library */
	dlfile = open(dlmap->l_name, O_RDONLY);
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
	{
		close(dlfile);
		return NULL;
	}

	/* Map library file into memory */
	file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	map_base = (uintptr_t)file_hdr;
	if (file_hdr == MAP_FAILED)
	{
		close(dlfile);
		return NULL;
	}
	close(dlfile);

	if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
	section_count = file_hdr->e_shnum;
	/* Get ELF section header string table */
	shstrtab_hdr = &sections[file_hdr->e_shstrndx];
	shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

	/* Iterate sections while looking for ELF symbol table and string table */
	for (uint16_t i = 0; i < section_count; i++)
	{
		Elf32_Shdr &hdr = sections[i];
		const char *section_name = shstrtab + hdr.sh_name;

		if (strcmp(section_name, ".symtab") == 0)
		{
			symtab_hdr = &hdr;
		}
		else if (strcmp(section_name, ".strtab") == 0)
		{
			strtab_hdr = &hdr;
		}
	}

	/* Uh oh, we don't have a symbol table or a string table */
	if (symtab_hdr == NULL || strtab_hdr == NULL)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
	strtab = (const char *)(map_base + strtab_hdr->sh_offset);
	symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

	/* Iterate symbol table starting from the position we were at last time */
	for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
	{
		Elf32_Sym &sym = symtab[i];
		unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
		const char *sym_name = strtab + sym.st_name;
		Symbol *cur_sym;

		/* Skip symbols that are undefined or do not refer to functions or objects */
		if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
		{
			continue;
		}

		/* Caching symbols as we go along */
		cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
		table2->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			symbol_entry = cur_sym;
			libtable->last_pos = ++i;
	//		break;
		}
	}

	libtable->last_pos = symbol_count;

	munmap(file_hdr, dlstat.st_size);
	return symbol_entry ? symbol_entry->address : NULL;

#elif defined PLATFORM_APPLE
	
	uintptr_t dlbase, linkedit_addr;
	uint32_t image_count;
	struct mach_header *file_hdr;
	struct load_command *loadcmds;
	struct segment_command *linkedit_hdr;
	struct symtab_command *symtab_hdr;
	struct nlist *symtab;
	const char *strtab;
	uint32_t loadcmd_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table;
	Symbol *symbol_entry;
	
	dlbase = 0;
	image_count = m_ImageList->infoArrayCount;
	linkedit_hdr = NULL;
	symtab_hdr = NULL;
	table = NULL;
	
	/* Loop through mach-o images in process.
	 * We can skip index 0 since that is just the executable.
	 */
	for (uint32_t i = 1; i < image_count; i++)
	{
		const struct dyld_image_info &info = m_ImageList->infoArray[i];
		
		/* "Load" each one until we get a matching handle */
		void *h = dlopen(info.imageFilePath, RTLD_NOLOAD);
		if (h == handle)
		{
			dlbase = (uintptr_t)info.imageLoadAddress;
			dlclose(h);
			break;
		}
		
		dlclose(h);
	}
	
	if (!dlbase)
	{
		/* Uh oh, we couldn't find a matching handle */
		return NULL;
	}
	
	/* See if we already have a symbol table for this library */
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		libtable = m_SymTables[i];
		if (libtable->lib_base == dlbase)
		{
			table = &libtable->table;
			break;
		}
	}
	
	/* If we don't have a symbol table for this library, then create one */
	if (table == NULL)
	{
		libtable = new LibSymbolTable();
		libtable->table.Initialize();
		libtable->lib_base = dlbase;
		libtable->last_pos = 0;
		table = &libtable->table;
		m_SymTables.push_back(libtable);
	}
	
	/* See if the symbol is already cached in our table */
	symbol_entry = table->FindSymbol(symbol, strlen(symbol));
	if (symbol_entry != NULL)
	{
		return symbol_entry->address;
	}
	
	/* If symbol isn't in our table, then we have to locate it in memory */
	
	file_hdr = (struct mach_header *)dlbase;
	loadcmds = (struct load_command *)(dlbase + sizeof(struct mach_header));
	loadcmd_count = file_hdr->ncmds;
	
	/* Loop through load commands until we find the ones for the symbol table */
	for (uint32_t i = 0; i < loadcmd_count; i++)
	{
		if (loadcmds->cmd == LC_SEGMENT && !linkedit_hdr)
		{
			struct segment_command *seg = (struct segment_command *)loadcmds;
			if (strcmp(seg->segname, "__LINKEDIT") == 0)
			{
				linkedit_hdr = seg;
				if (symtab_hdr)
				{
					break;
				}
			}
		}
		else if (loadcmds->cmd == LC_SYMTAB)
		{
			symtab_hdr = (struct symtab_command *)loadcmds;
			if (linkedit_hdr)
			{
				break;
			}
		}

		/* Load commands are not of a fixed size which is why we add the size */
		loadcmds = (struct load_command *)((uintptr_t)loadcmds + loadcmds->cmdsize);
	}
	
	if (!linkedit_hdr || !symtab_hdr || !symtab_hdr->symoff || !symtab_hdr->stroff)
	{
		/* Uh oh, no symbol table */
		return NULL;
	}

	linkedit_addr = dlbase + linkedit_hdr->vmaddr;
	symtab = (struct nlist *)(linkedit_addr + symtab_hdr->symoff - linkedit_hdr->fileoff);
	strtab = (const char *)(linkedit_addr + symtab_hdr->stroff - linkedit_hdr->fileoff);
	symbol_count = symtab_hdr->nsyms;
	
	/* Iterate symbol table starting from the position we were at last time */
	for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
	{
		struct nlist &sym = symtab[i];
		/* Ignore the prepended underscore on all symbols, so +1 here */
		const char *sym_name = strtab + sym.n_un.n_strx + 1;
		Symbol *cur_sym;
		
		/* Skip symbols that are undefined */
		if (sym.n_sect == NO_SECT)
		{
			continue;
		}
		
		/* Caching symbols as we go along */
		cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlbase + sym.n_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			symbol_entry = cur_sym;
			libtable->last_pos = ++i;
			break;
		}
	}
	
	return symbol_entry ? symbol_entry->address : NULL;

#endif
}

bool MemoryUtils::GetLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	uintptr_t baseAddr;

	if (libPtr == NULL)
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<uintptr_t>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

#elif defined PLATFORM_LINUX

	Dl_info info;
	Elf32_Ehdr *file;
	Elf32_Phdr *phdr;
	uint16_t phdrCount;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<uintptr_t>(info.dli_fbase);
	file = reinterpret_cast<Elf32_Ehdr *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	/* Check ELF architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->e_ident[EI_CLASS] != ELFCLASS32 || file->e_machine != EM_386 || file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<Elf32_Phdr *>(baseAddr + file->e_phoff);

	for (uint16_t i = 0; i < phdrCount; i++)
	{
		Elf32_Phdr &hdr = phdr[i];

		/* We only really care about the segment with executable code */
		if (hdr.p_type == PT_LOAD && hdr.p_flags == (PF_X|PF_R))
		{
			/* From glibc, elf/dl-load.c:
			 * c->mapend = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1) 
			 *             & ~(GLRO(dl_pagesize) - 1));
			 *
			 * In glibc, the segment file size is aligned up to the nearest page size and
			 * added to the virtual address of the segment. We just want the size here.
			 */
			lib.memorySize = PAGE_ALIGN_UP(hdr.p_filesz);
			break;
		}
	}

#elif defined PLATFORM_APPLE

	Dl_info info;
	struct mach_header *file;
	struct segment_command *seg;
	uint32_t cmd_count;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = (uintptr_t)info.dli_fbase;
	file = (struct mach_header *)baseAddr;

	/* Check Mach-O magic */
	if (file->magic != MH_MAGIC)
	{
		return false;
	}

	/* Check architecture (32-bit/x86) */
	if (file->cputype != CPU_TYPE_I386 || file->cpusubtype != CPU_SUBTYPE_I386_ALL)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if (file->filetype != MH_DYLIB)
	{
		return false;
	}

	cmd_count = file->ncmds;
	seg = (struct segment_command *)(baseAddr + sizeof(struct mach_header));
	
	/* Add up memory sizes of mapped segments */
	for (uint32_t i = 0; i < cmd_count; i++)
	{		
		if (seg->cmd == LC_SEGMENT)
		{
			lib.memorySize += seg->vmsize;
		}
		
		seg = (struct segment_command *)((uintptr_t)seg + seg->cmdsize);
	}

#endif

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}

const char* MemoryUtils::ResolveAddr(void *handle, void* addr)
{
	struct link_map *dlmap;
	dlmap = (struct link_map *)handle;

	LibSymbolTable *libtable;
	AddrTable *table2;

	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		libtable = m_SymTables[i];
		if (libtable->lib_base == dlmap->l_addr)
		{
			table2 = &libtable->table2;
			break;
		}
	}

	if (table2 == NULL) return NULL;
	Symbol *sym = table2->FindAddr(addr);
	return sym ? sym->buffer() : NULL;
}
