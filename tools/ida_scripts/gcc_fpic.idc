// -----------------------------------------------------------------------------
// - IDA Pro Script -
// Name: gcc_fpic.idc
// By: Damaged Soul
// Desc: Add references for strings, variables, and other data that seem mangled
//       due to GCC's -fPIC option and the .got section of an x86 ELF binary.
// 
// Version History
// 1.0 [2007-11-22]
//     - Initial Version
// 1.1 [2008-05-02]
//     - Now works with GCC 4.x compiled binaries
// 1.2 [2008-11-06]
//     - Now works with GCC 4.3 compiled binaries
//     - Fixed: Redefining alignment blocks as data caused IDA to pop up
//       an annoying warning
// -----------------------------------------------------------------------------

#include <idc.idc>

#define REG_NONE 0
#define REG_EAX 1
#define REG_EBX 2
#define REG_ECX 3
#define REG_EDX 4

#define OP_ADD 1
#define OP_SUB 2

#define OPFORMAT_STRING 1
#define OPFORMAT_DEREF  2
#define OPFORMAT_NORMAL 3

static main()
{
	auto filetype, compiler, demang, strPrefix;
	auto seg, codeStart, codeEnd, roStart, roEnd, gotStart, gotEnd, opformat;
	auto addr, funcend, whichop, reg, tempstr;
	auto operand1, operand2, opval, opstr, dataAddr, flags, count;
	
	SetStatus(IDA_STATUS_WORK);
	Message("Starting scan for -fPIC code...\n");
	
	/* Check file type and compiler */
	filetype = GetShortPrm(INF_FILETYPE);
	if (filetype != FT_ELF)
	{
		Message("Scan aborted. Input file must be using ELF binary format!\n");
		SetStatus(IDA_STATUS_READY);
		return;
	}
	compiler = GetCharPrm(INF_COMPILER);
	if (compiler != COMP_GNU)
	{
		Message("Scan aborted. Input file must have been compiled with GNU GCC/G++!\n");
		SetStatus(IDA_STATUS_READY);
		return;
	}

	/* 
	 * If the GCC v3.x names option is not set, then set it first.
	 *
	 * :TODO: Need to change this if GCC 2.95 support is to be added.
	 */
	demang = GetCharPrm(INF_DEMNAMES);
	if ((demang & DEMNAM_GCC3) == 0)
	{
		SetCharPrm(INF_DEMNAMES, demang | DEMNAM_GCC3);
	}
	
	/* Get string prefix */
	strPrefix = GetCharPrm(INF_ASCIIPREF);

	/* Get address of first section in binary */
	seg = FirstSeg();
	
	/* Iterate through all sections and get address of .text, .rodata, and .got */
	while (seg != BADADDR)
	{
		if (SegName(seg) == ".text")
		{
			codeStart = seg;
			codeEnd = NextSeg(seg);
			Message("%08.8Xh - %08.8Xh: .text\n", codeStart, codeEnd);
		}
		else if (SegName(seg) == ".rodata")
		{
			roStart = seg;
			roEnd = NextSeg(seg);
			Message("%08.8Xh - %08.8Xh: .rodata\n", roStart, roEnd);
		}
		else if (SegName(seg) == "abs")
		{
			addr = FindText(seg, SEARCH_DOWN|SEARCH_CASE|SEARCH_NOSHOW, 0, 0, "_GLOBAL_OFFSET_TABLE_");
			gotStart = Dword(addr);
			gotEnd = NextSeg(gotStart);
			Message("%08.8Xh - %08.8Xh: .got\n", gotStart, gotEnd);
		}
		
		seg = NextSeg(seg);
	}
	
	addr = codeStart;
	funcend = -1;
	count = 0;
	
	/**
	 * Go through .text section while looking for anything like [e?x+blah] or [e?x-blah].
	 *
	 * The eax, ebx, ecx, or edx registers are used for storing the address of the .got
	 * section with -fPIC code. An offset, either negative or positive is added to e?x.
	 * This results in the address of a string, variable, or other type of data.
	 *
	 * In order to determine which register .got will be stored in, one can look at which
	 * __i686.get_pc_thunk.? function is called near the beginning of each function. The
	 * suffix on this function is either ax, bx, cx, or dx and corresponds to eax, ebx, 
	 * ecx, or edx.
	 */
	while (addr <= codeEnd)
	{		
		operand1 = GetOpnd(addr, 0);
		operand2 = GetOpnd(addr, 1);
		whichop = -1;
		
		/* Get function end */
		if (FindFuncEnd(addr) != funcend)
		{
			reg = REG_NONE;
			funcend = FindFuncEnd(addr);
		}
		
		/* Get current PIC register */
		reg = GetPICRegister(addr, reg, funcend);
		
		if (reg != REG_NONE)
		{
			/* Search first operand for substring containing PIC register and either a plus or minus sign */
			if (strstr(operand1, GetPICSearchString(reg, OP_ADD)) != -1 || strstr(operand1, GetPICSearchString(reg, OP_SUB)) != -1)
			{
				whichop = 0;
			}
		
			/* Search second operand for substring containing PIC register and either a plus or minus sign */
			if (strstr(operand2, GetPICSearchString(reg, OP_ADD)) != -1 || strstr(operand2, GetPICSearchString(reg, OP_SUB)) != -1)
			{
				whichop = 1;
			}
		}
		
		if (whichop != -1)
		{
			/* Get .got offset */
			opval = GetOperandValue(addr, whichop);
			
			/* Get address inside .got */
			dataAddr = gotStart + opval;
			
			/* Get name at address if it exists */
			opstr = Name(dataAddr);
			
			/* If name doesn't exist then... */
			if (opstr == "")
			{	
				/* 
				 * Check address to see if it falls in .rodata section.
				 * If it does, then try to make it a string which will automatically give it a name.
				 */
				if (dataAddr >= roStart && dataAddr <= roEnd && MakeStr(dataAddr, BADADDR))
				{
					/* Get automatically created name */
					opstr = Name(dataAddr);
					opformat = OPFORMAT_STRING;
					
					/*
					 * Sometimes IDA creates a string successfully but not exactly in the right place.
					 * Uncertain as to why this is (perhaps an IDA bug?), but usually the string in 
					 * question is a bunch of garbage.
					 */
					if (opstr == "")
					{
						/* Create a name based on the address */
						opstr = form("unk_%X", dataAddr);
						if (strstr(GetDisasm(dataAddr), "align") != -1)
						{
							MakeUnkn(dataAddr, DOUNK_SIMPLE);
						}
						MakeNameEx(dataAddr, opstr, SN_NOCHECK|SN_NOLIST|SN_NOWARN);
						opformat = OPFORMAT_DEREF;
					}
				}
				else
				{
					/* 
					 * If address didn't fall into .rodata and the string creation was unsuccessful,
					 * then try to read the address at 'addr' and get the name of that.
					 */
					opstr = Name(Dword(dataAddr));
					if (opstr == "")
					{
						/* If name doesn't exist for that, then create name based on address */
						opstr = form("unk_%X", dataAddr);
						if (strstr(GetDisasm(dataAddr), "align") != -1)
						{
							MakeUnkn(dataAddr, DOUNK_SIMPLE);
						}
						MakeNameEx(dataAddr, opstr, SN_NOCHECK|SN_NOLIST|SN_NOWARN);
						opformat = OPFORMAT_DEREF;
					}
					else
					{
						/* If the name did exist at this point, then use it */
						opformat = OPFORMAT_NORMAL;
					}
				}
			}
			else
			{
				/* If the name at the original address does exist then ... */
				
				flags = GetFlags(dataAddr);
				
				/* 
				 * If this address falls into .rodata section and is considered an existing string
				 * then the replacement operand needs to shown as a string.
				 */ 
				if (dataAddr >= roStart && dataAddr <= roEnd && strPrefix != "" && strstr(opstr, strPrefix) == 0)
				{
					opformat = OPFORMAT_STRING;
				}
				else
				{
					opformat = OPFORMAT_DEREF;
				}
			}

			/*
			 * Try to demangle the name that was found or created above and print it as sort of
			 * a status message to show the the script is still doing work as this usually
			 * can take awhile.
			 */
			tempstr = Demangle(opstr, INF_LONG_DN);
			if (tempstr != "")
			{
				Message("%8.8Xh: %s\n", addr, tempstr);
			}
			else
			{
				Message("%8.8Xh: %s\n", addr, opstr);
			}
			
			/*
			 * The operand that was found to have the PIC register will now be replaced
			 * with more descriptive text. The format of this text depends upon the value
			 * of opformat.
			 */
			OpAlt(addr, whichop, DoOperandFormat(opformat, opstr));
			
			count++;
		}
	
		addr++;
	}
	
	Message("Scan for PIC code is complete! Found %d data items referenced via PIC register.\n", count);
	Message("Please re-open the database so that newly found strings will appear in the Strings window\n");
	SetStatus(IDA_STATUS_READY);
}

/*
 * Tries to determine the current PIC register given the current address being processed
 * and the previous PIC register.
 */
static GetPICRegister(addr, previous, funcend)
{
	auto assemblyStr, idx, reg, ab;
	assemblyStr = GetDisasm(addr);
	
	if ((idx = strstr(assemblyStr, "call    __i686_get_pc_thunk_")) != -1)
	{
		/* 28 is the length of the above string */ 
		reg = substr(assemblyStr, idx + 28, 30);
	}
	else if (strstr(assemblyStr, "call    $+5") != -1)
	{
		assemblyStr = GetDisasm(NextHead(addr, funcend));
		reg = substr(assemblyStr, 9, 11);
	}
	
	if (reg == "ax")
	{
		return REG_EAX;
	}
	else if (reg == "bx")
	{
		return REG_EBX;
	}
	else if (reg == "cx")
	{
		return REG_ECX;
	}
	else if (reg == "dx")
	{
		return REG_EDX;
	}
	
	return previous;
}

/*
 * Returns a string that is used as a substring search containing the specified
 * PIC register and operator (+ or -).
 */
static GetPICSearchString(reg, operator)
{
	if (reg == REG_EAX)
	{
		if (operator == OP_ADD)
		{
			return "[eax+";
		}
		else if (operator == OP_SUB)
		{
			return "[eax-";
		}
	}
	else if (reg == REG_EBX)
	{
		if (operator == OP_ADD)
		{
			return "[ebx+";
		}
		else if (operator == OP_SUB)
		{
			return "[ebx-";
		}
	}
	else if (reg == REG_ECX)
	{
		if (operator == OP_ADD)
		{
			return "[ecx+";
		}
		else if (operator == OP_SUB)
		{
			return "[ecx-";
		}
	}
	else if (reg == REG_EDX)
	{
		if (operator == OP_ADD)
		{
			return "[edx+";
		}
		else if (operator == OP_SUB)
		{
			return "[edx-";
		}
	}
}

/*
 * Returns a formatted string depending upon the value of the format param.
 * This will be the replacement for the operand containing the PIC register.
 *
 * OPFORMAT_STRING: The referenced data address is a string in the .rodata section.
 * OPFORMAT_DEREF:  The referenced data address has a name. The PIC register operand is
 *                  deferenced so the dereference brackets are shown in the returned string.
 * OPFORMAT_NORMAL: The referenced data address does not have a name. But upon reading
 *                  the address at the referenced data address, it is discovered that that
 *                  address does have a name. There are no dereference brackets because the
 *                  referenced data address had to be read in order to discover a name.
 */
static DoOperandFormat(format, str)
{
	if (format == OPFORMAT_STRING)
	{
		return form("offset %s", str);
	}
	else if (format == OPFORMAT_DEREF)
	{
		return form("[ds:%s]", str);
	}
	else if (format == OPFORMAT_NORMAL)
	{
		return form("ds:%s", str);
	}
	else
	{
		return str;
	}
}
