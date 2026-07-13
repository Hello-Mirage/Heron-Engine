#include "hrnpch.h"
#include "ScriptRegistry.h"

namespace Heron {

	std::unordered_map<std::string, ScriptRegistry::CreateFn>& ScriptRegistry::GetRegistry() {
		static std::unordered_map<std::string, CreateFn> s_Registry;
		return s_Registry;
	}

	void ScriptRegistry::Register(const std::string& name, CreateFn fn) {
		GetRegistry()[name] = fn;
		if (Log::GetCoreLogger())
			HRN_CORE_INFO("Registered Script: {0}", name);
	}

	void ScriptRegistry::Unregister(const std::string& name) {
		auto& reg = GetRegistry();
		if (reg.find(name) != reg.end()) {
			reg.erase(name);
			if (Log::GetCoreLogger())
				HRN_CORE_INFO("Unregistered Script: {0}", name);
		}
	}

	ScriptableEntity* ScriptRegistry::Create(const std::string& name) {
		auto& reg = GetRegistry();
		auto it = reg.find(name);
		if (it != reg.end())
			return it->second();
		return nullptr;
	}

}
