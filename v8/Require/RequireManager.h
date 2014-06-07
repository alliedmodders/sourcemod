#ifndef _INCLUDE_V8_DEPENDENCYMANAGER_H_
#define _INCLUDE_V8_DEPENDENCYMANAGER_H_

#include "IRequireProvider.h"
#include "ISourceMod.h"
#include "ILibrarySys.h"
#include "../DependencyManager.h"
#include <vector>

namespace SMV8
{
	namespace Require
	{
		using namespace SourceMod;
		using namespace std;
		class RequireManager
		{
		public:
			RequireManager(ISourceMod *sm, ILibrarySys *libsys, DependencyManager *depMan, ScriptLoader *script_loader);
			virtual ~RequireManager(void);
			string Require(const SMV8Script& requirer, const string& path) const;
		private:
			vector<IRequireProvider *> providers;
			ISourceMod *sm;
			ILibrarySys *libsys;
		};
	}
}

#endif // !_INCLUDE_V8_DEPENDENCYMANAGER_H_
