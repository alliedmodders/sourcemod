#pragma semicolon 1
#pragma newdecls required

#include <testing>
#include <files>

static const char g_Source[] = "test_async_file_source.txt";
static const char g_Destination[] = "test_async_file_destination.txt";
static const char g_BinarySource[] = "test_async_file_binary_source.dat";
static const char g_BinaryDestination[] = "test_async_file_binary_destination.dat";
static const char g_Directory[] = "test_async_file_dir";
static const char g_DirectoryFirst[] = "test_async_file_dir/first.txt";
static const char g_DirectorySecond[] = "test_async_file_dir/second.txt";
static bool g_NativeReturned;

static void WriteTestFile(const char[] path, const char[] contents)
{
	File file = OpenFile(path, "wb");
	AssertTrue("open_for_write", file != null);
	file.WriteString(contents, false);
	delete file;
}

static void ReadTestFile(const char[] path, char[] buffer, int maxlength)
{
	File file = OpenFile(path, "rb");
	AssertTrue("open_for_read", file != null);
	file.ReadString(buffer, maxlength);
	delete file;
}

static void WriteBinaryTestFile(const char[] path, const int[] bytes, int count)
{
	File file = OpenFile(path, "wb");
	AssertTrue("open_binary_for_write", file != null);
	AssertTrue("write_binary", file.Write(bytes, count, 1));
	delete file;
}

public void OnPluginStart()
{
	DeleteFile(g_Source);
	DeleteFile(g_Destination);
	DeleteFile(g_BinarySource);
	DeleteFile(g_BinaryDestination);
	DeleteFile(g_DirectoryFirst);
	DeleteFile(g_DirectorySecond);
	RemoveDir(g_Directory);
	WriteTestFile(g_Source, "async copyfile");
	AssertTrue("create_directory", CreateDirectory(g_Directory));
	WriteTestFile(g_DirectoryFirst, "first");
	WriteTestFile(g_DirectorySecond, "second");

	SetTestContext("AsyncCopyCreatesDestination");
	CopyFileAsync(g_Destination, g_Source, OnCopyComplete, 42);
	g_NativeReturned = true;
}

void OnCopyComplete(FileOpResult result, const char[] path, any data)
{
	char buffer[64];

	AssertTrue("callback_after_native", g_NativeReturned);
	AssertEq("copy_result", result, FileOp_Success);
	AssertStrEq("copy_path", path, g_Destination);
	AssertEq("copy_data", data, 42);
	AssertTrue("dest_exists", FileExists(g_Destination));
	ReadTestFile(g_Destination, buffer, sizeof(buffer));
	AssertStrEq("dest_contents", buffer, "async copyfile");

	SetTestContext("AsyncFileExists");
	FileExistsAsync(g_Destination, OnExistsComplete, 43);
}

void OnExistsComplete(bool exists, const char[] path, any data)
{
	AssertTrue("exists", exists);
	AssertStrEq("exists_path", path, g_Destination);
	AssertEq("exists_data", data, 43);

	SetTestContext("AsyncCopyMissingSource");
	CopyFileAsync(g_Destination, "test_async_file_missing.txt", OnMissingCopyComplete, 44);
}

void OnMissingCopyComplete(FileOpResult result, const char[] path, any data)
{
	char buffer[64];

	AssertEq("copy_result", result, FileOp_NotFound);
	AssertStrEq("copy_path", path, g_Destination);
	AssertEq("copy_data", data, 44);
	AssertTrue("dest_preserved", FileExists(g_Destination));
	ReadTestFile(g_Destination, buffer, sizeof(buffer));
	AssertStrEq("dest_contents", buffer, "async copyfile");

	int bytes[] = {0x00, 0x01, 0x41, 0x00, 0xff, 0x7f};
	WriteBinaryTestFile(g_BinarySource, bytes, sizeof(bytes));

	SetTestContext("AsyncReadBinary");
	ReadFileAsync(g_BinarySource, OnReadComplete, 45);
}

void OnReadComplete(FileOpResult result, const char[] path, const char[] contents, int size, any data)
{
	int expected[] = {0x00, 0x01, 0x41, 0x00, 0xff, 0x7f};

	AssertEq("read_result", result, FileOp_Success);
	AssertStrEq("read_path", path, g_BinarySource);
	AssertEq("read_size", size, sizeof(expected));
	AssertEq("read_data", data, 45);
	for (int i = 0; i < sizeof(expected); i++)
	{
		AssertEq("read_byte", contents[i] & 0xff, expected[i]);
	}

	SetTestContext("AsyncWriteBinary");
	WriteFileAsync(g_BinaryDestination, contents, size, OnWriteComplete, 46);
}

void OnWriteComplete(FileOpResult result, const char[] path, any data)
{
	int expected[] = {0x00, 0x01, 0x41, 0x00, 0xff, 0x7f};
	int actual[sizeof(expected)];
	File file;

	AssertEq("write_result", result, FileOp_Success);
	AssertStrEq("write_path", path, g_BinaryDestination);
	AssertEq("write_data", data, 46);
	file = OpenFile(g_BinaryDestination, "rb");
	AssertTrue("open_binary_for_read", file != null);
	AssertEq("read_binary", file.Read(actual, sizeof(actual), 1), sizeof(actual));
	delete file;
	for (int i = 0; i < sizeof(expected); i++)
	{
		AssertEq("written_byte", actual[i], expected[i]);
	}

	SetTestContext("AsyncDirectorySnapshot");
	OpenDirectoryAsync(g_Directory, OnDirectoryComplete, 47);
}

void OnDirectoryComplete(FileOpResult result, const char[] path, DirectoryListing listing, any data)
{
	char name[PLATFORM_MAX_PATH];
	FileType type;
	bool foundFirst;
	bool foundSecond;

	AssertEq("directory_result", result, FileOp_Success);
	AssertStrEq("directory_path", path, g_Directory);
	AssertEq("directory_data", data, 47);
	AssertTrue("directory_listing", listing != null);
	while (listing.GetNext(name, sizeof(name), type))
	{
		if (StrEqual(name, "first.txt"))
		{
			AssertEq("first_type", type, FileType_File);
			foundFirst = true;
		}
		else if (StrEqual(name, "second.txt"))
		{
			AssertEq("second_type", type, FileType_File);
			foundSecond = true;
		}
	}
	delete listing;
	AssertTrue("directory_first", foundFirst);
	AssertTrue("directory_second", foundSecond);

	DeleteFile(g_Source);
	DeleteFile(g_Destination);
	DeleteFile(g_BinarySource);
	DeleteFile(g_BinaryDestination);
	DeleteFile(g_DirectoryFirst);
	DeleteFile(g_DirectorySecond);
	RemoveDir(g_Directory);
	PrintToServer("OK");
}
