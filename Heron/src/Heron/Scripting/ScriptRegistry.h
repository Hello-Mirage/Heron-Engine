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

		static void Register(const std::string& name, CreateFn fn);
		static void Unregister(const std::string& name);
		static ScriptableEntity* Create(const std::string& name);

		static std::vector<std::string> GetRegisteredNames() {
			std::vector<std::string> names;
			for (auto& [name, fn] : GetRegistry())
				names.push_back(name);
			return names;
		}

	private:
		static std::unordered_map<std::string, CreateFn>& GetRegistry();
	};

	// Helper struct for auto-registration via static initialization
	struct ScriptRegistrar {
		std::string m_Name;
		ScriptRegistrar(const std::string& name, ScriptRegistry::CreateFn fn) : m_Name(name) {
			ScriptRegistry::Register(name, fn);
		}
		~ScriptRegistrar() {
			ScriptRegistry::Unregister(m_Name);
		}
	};

}

// Macro for easy one-liner registration
// Usage: HERON_REGISTER_SCRIPT(PlayerController)
#define HERON_REGISTER_SCRIPT(ClassName) \
	static Heron::ScriptRegistrar s_Registrar_##ClassName(#ClassName, []() -> Heron::ScriptableEntity* { return new ClassName(); })
