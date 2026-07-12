#pragma once

#include <memory>

// Platform detection
// HRN_PLATFORM_WINDOWS is defined via the project preprocessor definitions
#ifndef HRN_PLATFORM_WINDOWS
	#ifdef _WIN32
		#ifdef _WIN64
			#define HRN_PLATFORM_WINDOWS
		#else
			#error "x86 builds are not supported!"
		#endif
	#elif defined(__APPLE__) || defined(__MACH__)
		#error "macOS is not supported yet!"
	#elif defined(__linux__)
		#error "Linux is not supported yet!"
	#else
		#error "Unknown platform!"
	#endif
#endif

// Debug break
#ifdef HRN_PLATFORM_WINDOWS
	#define HRN_DEBUGBREAK() __debugbreak()
#else
	#define HRN_DEBUGBREAK()
#endif

// Assertions
#ifdef HRN_ENABLE_ASSERTS
	#define HRN_ASSERT(x, ...)      { if(!(x)) { HRN_ERROR("Assertion Failed: {0}", __VA_ARGS__); HRN_DEBUGBREAK(); } }
	#define HRN_CORE_ASSERT(x, ...) { if(!(x)) { HRN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); HRN_DEBUGBREAK(); } }
#else
	#define HRN_ASSERT(x, ...)
	#define HRN_CORE_ASSERT(x, ...)
#endif

// Bit field helper
#define BIT(x) (1 << x)

// Event binding helper
#define HRN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Heron {

	// Smart pointer aliases
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}
