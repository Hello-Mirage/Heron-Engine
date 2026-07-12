#pragma once

// ---- Standard Library ----
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <format>

// ---- Data Structures ----
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>

// ---- GLM ----
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---- Heron Core ----
#include "Heron/Core/Log.h"

// ---- Platform ----
#ifdef HRN_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
