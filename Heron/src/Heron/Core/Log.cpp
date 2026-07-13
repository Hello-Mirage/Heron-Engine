#include "hrnpch.h"
#include "Log.h"

namespace Heron {

	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
		// Pattern: [timestamp] logger_name: message
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("HERON");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	Ref<spdlog::logger>& Log::GetCoreLogger() { return s_CoreLogger; }
	Ref<spdlog::logger>& Log::GetClientLogger() { return s_ClientLogger; }

}
