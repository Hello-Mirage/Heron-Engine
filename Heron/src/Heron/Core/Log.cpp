#include "hrnpch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include "spdlog/sinks/basic_file_sink.h"

namespace Heron {

	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
		if (s_CoreLogger) return;

		// Pattern: [timestamp] logger_name: message
		spdlog::set_pattern("%^[%T] %n: %v%$");

		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("heron_crash_log.txt", true));

		try {
			s_CoreLogger = std::make_shared<spdlog::logger>("HERON", begin(logSinks), end(logSinks));
			spdlog::register_logger(s_CoreLogger);
		} catch (const spdlog::spdlog_ex& ex) {
			s_CoreLogger = spdlog::get("HERON");
		}
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		try {
			s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
			spdlog::register_logger(s_ClientLogger);
		} catch (const spdlog::spdlog_ex& ex) {
			s_ClientLogger = spdlog::get("APP");
		}
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}

	Ref<spdlog::logger>& Log::GetCoreLogger() { return s_CoreLogger; }
	Ref<spdlog::logger>& Log::GetClientLogger() { return s_ClientLogger; }

}
