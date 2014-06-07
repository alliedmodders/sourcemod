#ifndef _INCLUDE_V8_DEPENDENCYPROVIDER_H_
#define _INCLUDE_V8_DEPENDENCYPROVIDER_H_

#include "../ScriptLoader.h"

#include <string>

namespace SMV8
{
	namespace Require
	{
		class IRequireProvider
		{
		public:
			virtual bool Provides(const SMV8Script& requirer, const std::string& path) const = 0;
			virtual std::string Require(const SMV8Script& requirer, const std::string& path) const = 0;
			virtual std::string GetName() const = 0;
		};
	}
}

#endif // !_INCLUDE_V8_DEPENDENCYPROVIDER_H_
