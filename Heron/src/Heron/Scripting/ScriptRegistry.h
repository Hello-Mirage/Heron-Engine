#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Heron {

	class ScriptableEntity;

	class ScriptRegistry {
	public:
		using CreateFn = std::function<ScriptableEntity*()>;

		static void Register(const std::string& name, CreateFn fn) {
			GetRegistry()[name] = fn;
		}

		static ScriptableEntity* Create(const std::string& name) {
			auto& reg = GetRegistry();
			auto it = reg.find(name);
			if (it != reg.end())
				return it->second();
			return nullptr;
		}

		static std::vector<std::string> GetRegisteredNames() {
			std::vector<std::string> names;
			for (auto& [name, fn] : GetRegistry())
				names.push_back(name);
			return names;
		}

	private:
		static std::unordered_map<std::string, CreateFn>& GetRegistry() {
			static std::unordered_map<std::string, CreateFn> s_Registry;
			return s_Registry;
		}
	};

	// Helper struct for auto-registration via static initialization
	struct ScriptRegistrar {
		ScriptRegistrar(const std::string& name, ScriptRegistry::CreateFn fn) {
			ScriptRegistry::Register(name, fn);
		}
	};

}

// Macro for easy one-liner registration
// Usage: HERON_REGISTER_SCRIPT(PlayerController)
#define HERON_REGISTER_SCRIPT(ClassName) \
	static Heron::ScriptRegistrar s_Registrar_##ClassName(#ClassName, []() -> Heron::ScriptableEntity* { return new ClassName(); })
