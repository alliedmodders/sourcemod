#include <stdio.h>
#include "stub_mm.h"
#include "stub_util.h"
#include "sm_ext.h"

MyExtension g_SMExt;

bool SM_LoadExtension(char *error, size_t maxlength)
{
	if ((smexts = (IExtensionManager *)g_SMAPI->MetaFactory(
			SOURCEMOD_INTERFACE_EXTENSIONS, 
			NULL, 
			NULL))
		== NULL)
	{
		if (error && maxlength)
		{
			UTIL_Format(error, maxlength, SOURCEMOD_INTERFACE_EXTENSIONS " interface not found");
		}
		return false;
	}

	/* This could be more dynamic */
	char path[256];
	g_SMAPI->PathFormat(path, 
		sizeof(path), 
		"addons/myplugin/bin/myplugin%s",
#if defined __linux__
		"_i486.so"
#else
		".dll"
#endif	
		);

	if ((myself = smexts->LoadExternal(&g_SMExt,
			path,
			"myplugin_mm.ext",
			error,
			maxlength))
		== NULL)
	{
		SM_UnsetInterfaces();
		return false;
	}

	return true;
}

void SM_UnloadExtension()
{
	smexts->UnloadExtension(myself);	
}

bool MyExtension::OnExtensionLoad(IExtension *me,
		IShareSys *sys, 
		char *error, 
		size_t maxlength, 
		bool late)
{
	sharesys = sys;
	myself = me;

	/* Get the default interfaces from our configured SDK header */
	if (!SM_AcquireInterfaces(error, maxlength))
	{
		return false;
	}

	return true;	
}

void MyExtension::OnExtensionUnload()
{
	/* Clean up any resources here, and more importantly, make sure 
	 * any listeners/hooks into SourceMod are totally removed, as well 
	 * as data structures like handle types and forwards.
	 */

	//...

	/* Make sure our pointers get NULL'd just in case */
	SM_UnsetInterfaces();
}

void MyExtension::OnExtensionsAllLoaded()
{
	/* Called once all extensions are marked as loaded.
	 * This always called, and always called only once.
	 */
}

void MyExtension::OnExtensionPauseChange(bool pause)
{
}

bool MyExtension::QueryRunning(char *error, size_t maxlength)
{
	/* if something is required that can't be determined during the initial 
	 * load process, print a message here will show a helpful message to 
	 * users when they view the extension's info.
	 */
	return true;
}

bool MyExtension::IsMetamodExtension()
{
	/* Must return false! */
	return false;
}

const char *MyExtension::GetExtensionName()
{
	return mmsplugin->GetName();
}

const char *MyExtension::GetExtensionURL()
{
	return mmsplugin->GetURL();	
}

const char *MyExtension::GetExtensionTag()
{
	return mmsplugin->GetLogTag();
}

const char *MyExtension::GetExtensionAuthor()
{
	return mmsplugin->GetAuthor();
}

const char *MyExtension::GetExtensionVerString()
{
	return mmsplugin->GetVersion();
}

const char *MyExtension::GetExtensionDescription()
{
	return mmsplugin->GetDescription();
}

const char *MyExtension::GetExtensionDateString()
{
	return mmsplugin->GetDate();
}

