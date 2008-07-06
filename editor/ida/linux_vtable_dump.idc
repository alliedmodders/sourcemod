
// *****************************************************************************
// - IDA Pro script -
// Name: ida_vtables.idc
// Desc: Recreates the methods of a class from a vtable
// 
// Ver: 1.0b - July 20, 2006 - By Sirmabus   
// Ver: 2.0 - July 7, 2006 by BAILOPAN
// 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�-컴-
//
// -----------------------------------------------------------------------------

#include <idc.idc>

static main()
{
	auto pAddress, iIndex;
	auto szFilePath, hFile;
	auto skipAmt;

	SetStatus(IDA_STATUS_WORK);

	// User selected vtable block
	pAddress = ScreenEA();
	
	if (pAddress == BADADDR)
	{	
		Message("** No vtable selected! Aborted **");
		Warning("No vtable selected!\nSelect vtable block first.");							
		SetStatus(IDA_STATUS_READY);
		return;
	}

	skipAmt = AskLong(1, "Number of vtable entries to ignore for indexing:");

	// Request output header file
	SetStatus(IDA_STATUS_WAITING);
	if ((szFilePath = AskFile(1, "*.txt", "Select output dump file:")) == 0)
	{		
		Message("Aborted.");
		SetStatus(IDA_STATUS_READY);
		return;
	}
	
	// And create it..
	if ((hFile = fopen(szFilePath, "wb")) != 0)
	{
		auto szFuncName, szFullName, BadHits;
		
		BadHits = 0;

		// Create the header
		fprintf(hFile, "// Auto reconstructed from vtable block @ 0x%08X\n// from \"%s\", by ida_vtables.idc\n", pAddress, GetInputFile());
		
		/* For linux, skip the first entry */
		if (Dword(pAddress) == 0)
		{
			pAddress = pAddress + 8;
		}
		
		pAddress = pAddress + (skipAmt * 4);

		// Loop through the vtable block
		while (pAddress != BADADDR)
		{
			auto real_addr;
			real_addr = Dword(pAddress);
				
			szFuncName = Name(real_addr);
			if (strlen(szFuncName) == 0)
			{
				break;
			}
			szFullName = Demangle(szFuncName, INF_LONG_DN);
			if (szFullName == "")
			{
				szFullName = szFuncName;
			}
			if (strstr(szFullName, "_ZN") != -1)
			{
				fclose(hFile);
				Warning("You must toggle GCC v3.x demangled names!\n");
				break;
			}
			fprintf(hFile, "%d\t%s\n", iIndex, szFullName);
						
			pAddress = pAddress + 4;
			iIndex++;
		};

		fclose(hFile);
		Message("Successfully wrote %d vtable entries.\n", iIndex);
	}
	else
	{		
		Message("** Error opening \"%s\"! Aborted **\n", szFilePath);
		Warning("Error creating \"%s\"!\n", szFilePath);
	}

	Message("\nDone.\n\n");
	SetStatus(IDA_STATUS_READY);
}
