#include "RequireManager.h"
#include "CurrentDirectoryProvider.h"
#include "PackageRepositoryProvider.h"

namespace SMV8
{
	namespace Require
	{
		using namespace std;
		RequireManager::RequireManager(ISourceMod *sm, ILibrarySys *libsys, DependencyManager *depMan, ScriptLoader *script_loader)
			: sm(sm), libsys(libsys)
		{
			providers.push_back(new Providers::CurrentDirectoryProvider(sm, script_loader));
			providers.push_back(new Providers::PackageRepositoryProvider(sm, script_loader, depMan));
		}

		RequireManager::~RequireManager(void)
		{
			for(IRequireProvider *provider: providers)
			{
				delete provider;
			}
		}

		string RequireManager::Require(const SMV8Script& requirer, const string& path) const
		{
			for(IRequireProvider *provider: providers)
			{
				if(provider->Provides(requirer, path))
					return provider->Require(requirer, path);
				if(provider->Provides(requirer, path + "/main"))
					return provider->Require(requirer, path + "/main");
			}

			throw runtime_error("Dependency error: cannot resolve dependency '" + path + "' in script '" + requirer.GetPath() + "'");
		}
	}
}