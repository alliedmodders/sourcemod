#include <idc.idc>

/* makesig.idc: IDA script to automatically create and wildcard a function signature.
 * Copyright 2012, Asher Baker
 * 
 * This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source distribution.
*/

static main()
{
	Wait(); // We won't work until autoanalysis is complete

	SetStatus(IDA_STATUS_WORK);
	
	auto pAddress = ScreenEA();
	pAddress = GetFunctionAttr(pAddress, FUNCATTR_START);
	if (pAddress == BADADDR)
	{
		Warning("Make sure you are in a function!");							
		SetStatus(IDA_STATUS_READY);
		return;
	}
	
	auto sig;
	auto pFunctionEnd = GetFunctionAttr(pAddress, FUNCATTR_END);
	
	while (pAddress != BADADDR)
	{
		auto pInfo = DecodeInstruction(pAddress);
		if (!pInfo)
		{
			Warning("Something went terribly wrong D:");       
			SetStatus(IDA_STATUS_READY);
			return;
		}
		
		// isCode(GetFlags(pAddress)) == Opcode
		// isTail(GetFlags(pAddress)) == Operand
		// ((GetFlags(pAddress) & MS_CODE) == FF_IMMD) == :iiam:
		
		auto bDone = 0;
		
		if (pInfo.n == 1)
		{
			if (pInfo.Op0.type == o_near || pInfo.Op0.type == o_far)
			{
				sig = sprintf("%s%02X %s", sig, Byte(pAddress), PrintWildcards(GetDTSize(pInfo.Op0.dtyp)));
				bDone = 1;
			}
		}
		
		if (!bDone) { // unknown, just wildcard addresses
			auto i;
			for (i = 0; i < pInfo.size; i++)
			{
				auto pLoc = pAddress + i;
				if (GetFixupTgtType(pLoc) == FIXUP_OFF32)
				{
					sig = sprintf("%s%s", sig, PrintWildcards(4));
					i = i + 3;
				} else {
					sig = sprintf("%s%02X ", sig, Byte(pLoc));
				}
			}
		}
		
		if (IsGoodSig(sig))
			break;
		
		pAddress = NextHead(pAddress, pFunctionEnd);
	}
	
	Message("%s\n", sig);
	
	SetStatus(IDA_STATUS_READY);
	return;
}

static GetDTSize(dtyp)
{
	if (dtyp == dt_byte)
	{
		return 1;
	} else if (dtyp == dt_word) {
		return 2;
	} else if (dtyp == dt_dword) {
		return 4;
	} else if (dtyp == dt_float) {
		return 4;
	} else if (dtyp == dt_double) {
		return 8;
	} else {
		Warning("Unknown type size (%d)", dtyp);
		return -1;
	}
}

static PrintWildcards(count)
{
	auto i, string;
	for (i = 0; i < count; i++)
	{
		string = sprintf("%s? ", string);
	}
	return string;
}

static IsGoodSig(sig)
{
	auto count, addr;
	addr = FindBinary(addr, SEARCH_DOWN|SEARCH_NEXT, sig);
	while (addr != BADADDR)
	{
		count = count + 1;
		addr = FindBinary(addr, SEARCH_DOWN|SEARCH_NEXT, sig);
	}
	return (count == 1);
}