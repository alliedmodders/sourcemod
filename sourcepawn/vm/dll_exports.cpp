/**
 * vim: set ts=4 sts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
 *
 * Version: $Id$
 */

#include <sp_vm_api.h>
#include <stdlib.h>
#include <stdarg.h>
#include <am-utility.h> // Replace with am-cxx later.
#include "dll_exports.h"
#include "environment.h"
#include "stack-frames.h"

using namespace ke;
using namespace sp;
using namespace SourcePawn;

class SourcePawnFactory : public ISourcePawnFactory
{
public:
	int ApiVersion() KE_OVERRIDE {
		return SOURCEPAWN_API_VERSION;
	}
	ISourcePawnEnvironment *NewEnvironment() KE_OVERRIDE {
		return Environment::New();
	}
	ISourcePawnEnvironment *CurrentEnvironment() KE_OVERRIDE {
		return Environment::get();
	}
} sFactory;

#ifdef SPSHELL
Environment *sEnv;

static void
DumpStack(IFrameIterator &iter)
{
	int index = 0;
	for (; !iter.Done(); iter.Next(), index++) {
		const char *name = iter.FunctionName();
		if (!name) {
			fprintf(stdout, "  [%d] <unknown>\n", index);
			continue;
		}

		if (iter.IsScriptedFrame()) {
			const char *file = iter.FilePath();
			if (!file)
				file = "<unknown>";
			fprintf(stdout, "  [%d] %s::%s, line %d\n", index, file, name, iter.LineNumber());
		} else {
			fprintf(stdout, "  [%d] %s()\n", index, name);
		}
	}
}

class ShellDebugListener : public IDebugListener
{
public:
	void ReportError(const IErrorReport &report, IFrameIterator &iter) KE_OVERRIDE {
		fprintf(stdout, "Exception thrown: %s\n", report.Message());
		DumpStack(iter);
	}

	void OnDebugSpew(const char *msg, ...) {
#if !defined(NDEBUG) && defined(DEBUG)
		va_list ap;
		va_start(ap, msg);
		vfprintf(stderr, msg, ap);
		va_end(ap);
#endif
	}
};

static cell_t Print(IPluginContext *cx, const cell_t *params)
{
	char *p;
	cx->LocalToString(params[1], &p);

	return printf("%s", p);
}

static cell_t PrintNum(IPluginContext *cx, const cell_t *params)
{
	return printf("%d\n", params[1]);
}

static cell_t PrintNums(IPluginContext *cx, const cell_t *params)
{
	for (size_t i = 1; i <= size_t(params[0]); i++) {
		int err;
		cell_t *addr;
		if ((err = cx->LocalToPhysAddr(params[i], &addr)) != SP_ERROR_NONE)
			return cx->ThrowNativeErrorEx(err, "Could not read argument");
		fprintf(stdout, "%d", *addr);
		if (i != size_t(params[0]))
			fprintf(stdout, ", ");
	}
	fprintf(stdout, "\n");
	return 1;
}

static cell_t DoNothing(IPluginContext *cx, const cell_t *params)
{
	return 1;
}

static void BindNative(IPluginRuntime *rt, const char *name, SPVM_NATIVE_FUNC fn)
{
	int err;
	uint32_t index;
	if ((err = rt->FindNativeByName(name, &index)) != SP_ERROR_NONE)
		return;

	rt->UpdateNativeBinding(index, fn, 0, nullptr);
}

static cell_t PrintFloat(IPluginContext *cx, const cell_t *params)
{
	return printf("%f\n", sp_ctof(params[1]));
}

static cell_t DoExecute(IPluginContext *cx, const cell_t *params)
{
	int32_t ok = 0;
	for (size_t i = 0; i < size_t(params[2]); i++) {
		if (IPluginFunction *fn = cx->GetFunctionById(params[1])) {
			if (fn->Execute(nullptr) != SP_ERROR_NONE)
				continue;
			ok++;
		}
	}
	return ok;
}

static cell_t DoInvoke(IPluginContext *cx, const cell_t *params)
{
	for (size_t i = 0; i < size_t(params[2]); i++) {
		if (IPluginFunction *fn = cx->GetFunctionById(params[1])) {
			if (!fn->Invoke())
				return 0;
		}
	}
	return 1;
}

static cell_t DumpStackTrace(IPluginContext *cx, const cell_t *params)
{
	FrameIterator iter;
	DumpStack(iter);
	return 0;
}

static int Execute(const char *file)
{
	char error[255];
	AutoPtr<IPluginRuntime> rt(sEnv->APIv2()->LoadBinaryFromFile(file, error, sizeof(error)));
	if (!rt) {
		fprintf(stderr, "Could not load plugin: %s\n", error);
		return 1;
	}

	BindNative(rt, "print", Print);
	BindNative(rt, "printnum", PrintNum);
	BindNative(rt, "printnums", PrintNums);
	BindNative(rt, "printfloat", PrintFloat);
	BindNative(rt, "donothing", DoNothing);
	BindNative(rt, "execute", DoExecute);
	BindNative(rt, "invoke", DoInvoke);
	BindNative(rt, "dump_stack_trace", DumpStackTrace);

	IPluginFunction *fun = rt->GetFunctionByName("main");
	if (!fun)
		return 0;

	IPluginContext *cx = rt->GetDefaultContext();

	int result;
	{
		ExceptionHandler eh(cx);
		if (!fun->Invoke(&result)) {
			fprintf(stderr, "Error executing main: %s\n", eh.Message());
			return 1;
		}
	}

	return result;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: <file>\n");
		return 1;
	}

	if ((sEnv = Environment::New()) == nullptr) {
		fprintf(stderr, "Could not initialize ISourcePawnEngine2\n");
		return 1;
	}

	if (getenv("DISABLE_JIT"))
		sEnv->SetJitEnabled(false);

	ShellDebugListener debug;
	sEnv->SetDebugger(&debug);
	sEnv->InstallWatchdogTimer(5000);

	int errcode = Execute(argv[1]);

	sEnv->SetDebugger(NULL);
	sEnv->Shutdown();
	delete sEnv;

	return errcode;
}

#else

#define MIN_API_VERSION 0x0207

EXPORTFUNC ISourcePawnFactory *
GetSourcePawnFactory(int apiVersion)
{
	if (apiVersion < MIN_API_VERSION || apiVersion > SOURCEPAWN_API_VERSION)
		return nullptr;
	return &sFactory;
}
#endif

#if defined __linux__ || defined __APPLE__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif

