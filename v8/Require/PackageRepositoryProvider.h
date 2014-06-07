#pragma once

#include "IRequireProvider.h"
#include "..\DependencyManager.h"
#include <ISourceMod.h>

namespace SMV8
{
	namespace Require
	{
		namespace Providers
		{
			using namespace SourceMod;
			class PackageRepositoryProvider : public IRequireProvider
			{
			public:
				PackageRepositoryProvider(ISourceMod *sm, ScriptLoader *script_loader, DependencyManager *depMan);
				virtual ~PackageRepositoryProvider(void);
				virtual bool Provides(const SMV8Script& requirer, const std::string& path) const;
				virtual std::string Require(const SMV8Script& requirer, const std::string& path) const;
				std::string GetName() const;
			private:
				std::string ResolvePath(const SMV8Script& requirer, const string& path) const;
				ISourceMod *sm;
				DependencyManager *depMan;
				ScriptLoader *script_loader;
			};
		}
	}
}
