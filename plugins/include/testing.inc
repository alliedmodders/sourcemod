/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod (C)2004-2014 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 *
 * Version: $Id$
 */

static int TestNumber = 0;
static char TestContext[255];

stock void SetTestContext(const char[] context)
{
	strcopy(TestContext, sizeof(TestContext), context);
}

stock void AssertEq(const char[] text, int cell1, int cell2)
{
	TestNumber++;
	if (cell1 == cell2) {
		PrintToServer("[%d] %s: %s == %d OK", TestNumber, TestContext, text, cell2);
	} else {
		PrintToServer("[%d] %s FAIL: %s should be %d, got %d", TestNumber, TestContext, text, cell2, cell1);
		ThrowError("test %d (%s in %s) failed", TestNumber, text, TestContext);
	}
}

stock void AssertFalse(const char[] text, bool value)
{
	TestNumber++;
	if (!value) {
		PrintToServer("[%d] %s: %s == false OK", TestNumber, TestContext, text, value);
	} else {
		PrintToServer("[%d] %s FAIL: %s should be false, got true", TestNumber, TestContext, text);
		ThrowError("test %d (%s in %s) failed", TestNumber, text, TestContext);
	}
}

stock void AssertTrue(const char[] text, bool value)
{
	TestNumber++;
	if (value) {
		PrintToServer("[%d] %s: %s == true OK", TestNumber, TestContext, text, value);
	} else {
		PrintToServer("[%d] %s FAIL: %s should be true, got false", TestNumber, TestContext, text);
		ThrowError("test %d (%s in %s) failed", TestNumber, text, TestContext);
	}
}
