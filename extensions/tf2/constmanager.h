#pragma once

#include "smsdk_ext.h"

class ConstManager : public IPluginsListener
{
public:
	ConstManager() = default;
public: // IPluginsListener
	virtual void OnPluginLoaded(IPlugin *plugin) override;
};