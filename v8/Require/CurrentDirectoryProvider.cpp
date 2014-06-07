#include "CurrentDirectoryProvider.h"
#include <sm_platform.h>
#include <fstream>

namespace SMV8
{
	namespace Require
	{
		namespace Providers
		{
			using namespace SourceMod;
			using namespace std;
			CurrentDirectoryProvider::CurrentDirectoryProvider(ISourceMod *sm, ScriptLoader *script_loader)
				: sm(sm), script_loader(script_loader)
			{
			}

			CurrentDirectoryProvider::~CurrentDirectoryProvider(void)
			{
			}

			std::string CurrentDirectoryProvider::ResolvePath(const SMV8Script& requirer, const string& path) const
			{
				string reqpath = requirer.GetPath();
				string requirer_dir = reqpath.substr(0,reqpath.find_last_of('/') + 1);

				return requirer_dir + path;
			}

			bool CurrentDirectoryProvider::Provides(const SMV8Script& requirer, const string& path) const
			{
				return script_loader->CanAutoLoad(ResolvePath(requirer, path));
			}
			
			string CurrentDirectoryProvider::Require(const SMV8Script& requirer, const string& path) const
			{
				string fullpath = ResolvePath(requirer, path);

				if(!script_loader->CanAutoLoad(fullpath))
					throw logic_error("Can't open required path " + path + ". This should not happen.");

				return fullpath;
			}

			std::string CurrentDirectoryProvider::GetName() const
			{
				return "Current directory";
			}
		}
	}
}