#pragma once

#include "Heron/Core/Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace Heron {

	class Log {
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger()   { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger()  { return s_ClientLogger; }

	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros (engine internals)
#define HRN_CORE_TRACE(...)    ::Heron::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HRN_CORE_INFO(...)     ::Heron::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HRN_CORE_WARN(...)     ::Heron::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HRN_CORE_ERROR(...)    ::Heron::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HRN_CORE_CRITICAL(...) ::Heron::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros (game code)
#define HRN_TRACE(...)         ::Heron::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HRN_INFO(...)          ::Heron::Log::GetClientLogger()->info(__VA_ARGS__)
#define HRN_WARN(...)          ::Heron::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HRN_ERROR(...)         ::Heron::Log::GetClientLogger()->error(__VA_ARGS__)
#define HRN_CRITICAL(...)      ::Heron::Log::GetClientLogger()->critical(__VA_ARGS__)
