#include "PackageRepositoryProvider.h"
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
			PackageRepositoryProvider::PackageRepositoryProvider(ISourceMod *sm, ScriptLoader *script_loader, DependencyManager *depMan)
				: sm(sm), depMan(depMan), script_loader(script_loader)
			{
			}

			PackageRepositoryProvider::~PackageRepositoryProvider(void)
			{
			}

			std::string PackageRepositoryProvider::ResolvePath(const SMV8Script& requirer, const string& path) const
			{
				string package = requirer.IsInPackage() ? requirer.GetVersionedPackage() : "__nopak__" + requirer.GetPath();
				return DependencyManager::packages_root + depMan->ResolvePath(package,path);
			}

			bool PackageRepositoryProvider::Provides(const SMV8Script& requirer, const string& path) const
			{
				return script_loader->CanAutoLoad(ResolvePath(requirer, path));
			}
			
			string PackageRepositoryProvider::Require(const SMV8Script& requirer, const string& path) const
			{
				string fullpath(ResolvePath(requirer, path));

				if(!script_loader->CanAutoLoad(fullpath))
					throw logic_error("Can't open required path " + path + ". This should not happen.");

				return fullpath;
			}

			std::string PackageRepositoryProvider::GetName() const
			{
				return "Package repository";
			}
		}
	}
}
