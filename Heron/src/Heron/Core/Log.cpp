#include "hrnpch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace Heron {

	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
		if (s_CoreLogger) return;

		// Pattern: [timestamp] logger_name: message
		spdlog::set_pattern("%^[%T] %n: %v%$");

		try {
			s_CoreLogger = spdlog::stdout_color_mt("HERON");
		} catch (const spdlog::spdlog_ex& ex) {
			s_CoreLogger = spdlog::get("HERON");
		}
		s_CoreLogger->set_level(spdlog::level::trace);

		try {
			s_ClientLogger = spdlog::stdout_color_mt("APP");
		} catch (const spdlog::spdlog_ex& ex) {
			s_ClientLogger = spdlog::get("APP");
		}
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	Ref<spdlog::logger>& Log::GetCoreLogger() { return s_CoreLogger; }
	Ref<spdlog::logger>& Log::GetClientLogger() { return s_ClientLogger; }

}
