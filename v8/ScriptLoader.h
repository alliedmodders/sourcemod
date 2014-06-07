#pragma once

#include <v8.h>
#include <ISourceMod.h>
#include <string>

namespace SMV8
{
	using namespace SourceMod;
	using namespace v8;
	class SMV8Script
	{
	public:
		SMV8Script(std::string code, std::string path);
		SMV8Script(const SMV8Script& other);
		~SMV8Script();
		std::string GetCode() const;
		std::string GetPath() const;
		std::string GetPackage() const;
		std::string GetVersionedPackage() const;
		bool IsInPackage() const;
	private:
		std::string path;
		std::string code;
	};

	class ScriptLoader
	{
	public:
		ScriptLoader(Isolate *isolate, ISourceMod *sm);
		virtual ~ScriptLoader();
		SMV8Script AutoLoadScript(const std::string& location) const;
		SMV8Script LoadScript(const std::string& location, bool forceCoffee=false) const;
		bool CanLoad(const std::string& location) const;
		bool CanAutoLoad(const std::string& location) const;
	private:
		void LoadCoffeeCompiler(ISourceMod *sm);
		std::string CompileCoffee(const std::string& coffee) const;
		Persistent<Context> coffeeCompilerContext;
		Isolate *isolate;
		ISourceMod *sm;
	};
}
