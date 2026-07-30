#pragma semicolon 1
#pragma newdecls required
#include <testing>
#include <files>

// Only the non-Valve-fs path is exercised here. The hl2sdk-mock file system
// reports IsOk() inverted, so ValveFile::HasError() is always true under the
// mock and the Valve-fs streaming copy cannot be meaningfully tested there.

static void WriteTestFile(const char[] path, const char[] contents)
{
	File f = OpenFile(path, "wb");
	AssertTrue("open_for_write", f != null);
	f.WriteString(contents, false);
	delete f;
}

static void ReadTestFile(const char[] path, char[] buffer, int maxlength)
{
	File f = OpenFile(path, "rb");
	AssertTrue("open_for_read", f != null);
	f.ReadString(buffer, maxlength);
	delete f;
}

public void OnPluginStart()
{
	char src[] = "test_copyfile_src.txt";
	char dst[] = "test_copyfile_dst.txt";
	char buffer[256];

	// Clean up any leftovers from a previous run.
	DeleteFile(src);
	DeleteFile(dst);

	// --------------------------------------------------------------------------------

	SetTestContext("CopyCreatesDestination");

	WriteTestFile(src, "hello copyfile");
	AssertTrue("copy_returns_true", CopyFile(dst, src));
	AssertTrue("dest_exists", FileExists(dst));
	ReadTestFile(dst, buffer, sizeof(buffer));
	AssertStrEq("dest_contents", buffer, "hello copyfile");

	// --------------------------------------------------------------------------------

	SetTestContext("CopyOverwritesDestination");

	WriteTestFile(dst, "OLD CONTENT that is longer than the source");
	AssertTrue("copy_returns_true", CopyFile(dst, src));
	ReadTestFile(dst, buffer, sizeof(buffer));
	AssertStrEq("dest_overwritten", buffer, "hello copyfile");

	// --------------------------------------------------------------------------------

	SetTestContext("CopyMissingSourceFails");

	DeleteFile(dst);
	AssertFalse("copy_returns_false", CopyFile(dst, "test_copyfile_missing.txt"));
	AssertFalse("dest_not_created", FileExists(dst));

	// --------------------------------------------------------------------------------

	DeleteFile(src);
	DeleteFile(dst);

	PrintToServer("OK");
}
