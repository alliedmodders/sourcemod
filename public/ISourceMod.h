#ifndef _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_

#include <IShareSys.h>

#define SMINTERFACE_SOURCEMOD_NAME		"ISourceMod"
#define SMINTERFACE_SOURCEMOD_VERSION	1

namespace SourceMod
{
	enum PathType
	{
		Path_None = 0,
		Path_Game,
		Path_SM,
	};

	class ISourceMod : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_SOURCEMOD_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_SOURCEMOD_VERSION;
		}
	public:
		/**
		 * @brief Returns the full path to the mod directory.
		 *
		 * @return			A string containing the full mod path.
		 */
		virtual const char *GetModPath() =0;

		/**
		 * @brief Returns the full path to the SourceMod directory.
		 *
		 * @return			A string containing the full SourceMod path.
		 */
		virtual const char *GetSourceModPath() =0;

		/**
		 * @brief Builds a platform path for a specific target base path.
		 *
		 * @param type		Type of path to use as a base.
		 * @param buffer	Buffer to write to.
		 * @param maxlength	Size of buffer.
		 * @param format	Format string.
		 * @param ...		Format arguments.
		 * @return			Number of bytes written.
		 */
		virtual size_t BuildPath(PathType type, char *buffer, size_t maxlength, char *format, ...) =0;

		/**
		 * @brief Logs a message to the SourceMod logs.
		 *
		 * @param format	Message format.
		 * @param ...		Message format parameters.
		 */
		virtual void LogMessage(IExtension *pExt, const char *format, ...) =0;

		/**
		 * @brief Logs a message to the SourceMod error logs.
		 *
		 * @param format	Message format.
		 * @param ...		Message format parameters.
		 */
		virtual void LogError(IExtension *pExt, const char *format, ...) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
