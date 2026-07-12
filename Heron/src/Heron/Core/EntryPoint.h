#pragma once

#include "Heron/Core/Core.h"
#include "Heron/Core/Application.h"

#ifdef HRN_PLATFORM_WINDOWS

extern Heron::Application* Heron::CreateApplication();

int main(int argc, char** argv) {
	Heron::Log::Init();
	HRN_CORE_INFO("=== Heron Engine ===");
	HRN_CORE_INFO("Initializing...");

	auto app = Heron::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif
