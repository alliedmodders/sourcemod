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
template <typename T> class AutoT
{
public:
	AutoT(T *t)
	: t_(t)
	{
	}
	~AutoT()
	{
		delete t_;
	}

	operator T *() const {
		return t_;
	}
	bool operator !() const {
		return !t_;
	}
	T * operator ->() const {
		return t_;
	}
private:
	T *t_;
};

Environment *sEnv;

class ShellDebugListener : public IDebugListener
{
public:
	void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error) {
		int n_err = error->GetErrorCode();

		if (n_err != SP_ERROR_NATIVE)
		{
			fprintf(stderr, "plugin error: %s\n", error->GetErrorString());
		}

		if (const char *lastname = error->GetLastNative(NULL))
		{
			if (const char *custerr = error->GetCustomErrorString())
			{
				fprintf(stderr, "Native \"%s\" reported: %s", lastname, custerr);
			} else {
				fprintf(stderr, "Native \"%s\" encountered a generic error.", lastname);
			}
		}

		if (!error->DebugInfoAvailable())
		{
			fprintf(stderr, "Debug info not available!\n");
			return;
		}

		CallStackInfo stk_info;
		int i = 0;
		fprintf(stderr, "Displaying call stack trace:\n");
		while (error->GetTraceInfo(&stk_info))
		{
			fprintf(stderr,
			    "   [%d]  Line %d, %s::%s()\n",
				i++,
				stk_info.line,
				stk_info.filename,
				stk_info.function);
		}
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

static int Execute(const char *file)
{
	ICompilation *co = sEnv->APIv2()->StartCompilation();
	if (!co) {
		fprintf(stderr, "Could not create a compilation context\n");
		return 1;
	}

	int err;
	AutoT<IPluginRuntime> rt(sEnv->APIv2()->LoadPlugin(co, file, &err));
	if (!rt) {
		fprintf(stderr, "Could not load plugin: %s\n", sEnv->GetErrorString(err));
		return 1;
	}

	BindNative(rt, "print", Print);
	BindNative(rt, "printnum", PrintNum);
	BindNative(rt, "printnums", PrintNums);
	BindNative(rt, "printfloat", PrintFloat);
	BindNative(rt, "donothing", DoNothing);

	IPluginFunction *fun = rt->GetFunctionByName("main");
	if (!fun)
		return 0;

	IPluginContext *cx = rt->GetDefaultContext();

	int result = fun->Execute2(cx, &err);
	if (err != SP_ERROR_NONE) {
		fprintf(stderr, "Error executing main(): %s\n", sEnv->GetErrorString(err));
		return 1;
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

