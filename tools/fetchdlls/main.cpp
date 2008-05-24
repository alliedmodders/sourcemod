#include <stdio.h>
#include <windows.h>
#include <DbgHelp.h>

void mdmp_string(void *mdmp, RVA addr, char *buffer, size_t maxlength)
{
	int len;
	MINIDUMP_STRING *str;

	str = (MINIDUMP_STRING *)((char *)mdmp + addr);
	len = WideCharToMultiByte(CP_UTF8, 
		0, 
		str->Buffer, 
		str->Length / sizeof(WCHAR), 
		buffer, 
		maxlength,
		NULL,
		NULL);
	buffer[len] = '\0';
}

int find_revision(const char *version, MINIDUMP_MODULE *module, const char *name)
{
	FILE *fp;
	char msg[255];
	size_t i, len;
	char path[255];
	char buffer[3000];
	int last_revision;

	len = _snprintf(path, sizeof(path), "%s", name);
	for (i = 0; i < len; i++)
	{
		if (path[i] == '\\')
		{
			path[i] = '/';
		}
	}

	DeleteFile("_svnlog.txt");
	_snprintf(buffer, 
		sizeof(buffer),
		"svn log svn://svn.alliedmods.net/svnroot/Packages/sourcemod/sourcemod-%d.%d/windows/base/addons/sourcemod/%s > _svnlog.txt",
		(module->VersionInfo.dwFileVersionMS >> 16),
		(module->VersionInfo.dwFileVersionMS & 0xFFFF),
		path);
	system(buffer);

	if ((fp = fopen("_svnlog.txt", "rt")) == NULL)
	{
		return -1;
	}

	_snprintf(msg, 
		sizeof(msg), 
		"sourcemod-%s",
		version);

	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if (buffer[0] == 'r')
		{
			last_revision = atoi(&buffer[1]);
		}
		else if (strstr(buffer, msg) != NULL)
		{
			fclose(fp);

			_snprintf(buffer,
				sizeof(buffer),
				"svn export -q -r %d svn://svn.alliedmods.net/svnroot/Packages/sourcemod/sourcemod-%d.%d/windows/base/addons/sourcemod/%s",
				last_revision,
				(module->VersionInfo.dwFileVersionMS >> 16),
				(module->VersionInfo.dwFileVersionMS & 0xFFFF),
				path);
			system(buffer);

			return last_revision;
		}
	}

	fclose(fp);

	return -1;
}

int main(int argc, char **argv)
{
	int rev;
	ULONG32 m;
	LPVOID mdmp;
	HANDLE hFile;
	const char *name;
	ULONG stream_size;
	char name_buf[255];
	HANDLE hFileMapping;
	MINIDUMP_MODULE *module;
	MINIDUMP_DIRECTORY *dir;
	MINIDUMP_MODULE_LIST *modules;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: <build> <mdmp>\n");
		exit(-1);
	}

	if ((hFile = CreateFile(argv[2],
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL))
		== INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Could not open file (error %d)\n", GetLastError());
		exit(-1);
	}

	hFileMapping = CreateFileMapping(
		hFile,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);
	if (hFileMapping == NULL)
	{
		fprintf(stderr, "Could not open file mapping (error %d)\n", GetLastError());
		CloseHandle(hFile);
		exit(-1);
	}

	mdmp = MapViewOfFile(hFileMapping,
		FILE_MAP_READ,
		0,
		0,
		0);
	if (mdmp == NULL)
	{
		fprintf(stderr, "Could not create map view (error %d)\n", GetLastError());
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		exit(-1);
	}

	if (!MiniDumpReadDumpStream(mdmp,
			ModuleListStream,
			&dir,
			(void **)&modules,
			&stream_size))
	{
		fprintf(stderr, "Could not read the module list stream.\n");
		UnmapViewOfFile(mdmp);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
	}

	for (m = 0; m < modules->NumberOfModules; m++)
	{
		module = &modules->Modules[m];

		mdmp_string(mdmp, module->ModuleNameRva, name_buf, sizeof(name_buf));

		if ((name = strstr(name_buf, "sourcemod\\")) != NULL)
		{
			name += 10;
			fprintf(stdout, 
				"looking for: %s (%d.%d.%d.%d of build %s)... ", 
				name, 
				(module->VersionInfo.dwFileVersionMS >> 16),
				(module->VersionInfo.dwFileVersionMS & 0xFFFF),
				(module->VersionInfo.dwFileVersionLS >> 16),
				(module->VersionInfo.dwFileVersionLS & 0xFFFF),
				argv[1]
				);
			fflush(stdout);
			if ((rev = find_revision(argv[1], module, name)) == -1)
			{
				fprintf(stdout, "not found :(\n");
				continue;
			}
			fprintf(stdout, "downloaded! (pkgrev %d)\n", rev);
		}
	}

	UnmapViewOfFile(mdmp);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);

	return 0;
}
