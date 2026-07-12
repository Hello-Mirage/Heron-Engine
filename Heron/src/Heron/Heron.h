#pragma once

// ---- Heron Engine Master Include ----
// Client applications include this single file

#include "Core/Core.h"
#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Timestep.h"
#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/MouseCodes.h"

#include "Layers/Layer.h"

#include "Heron/ImGui/ImGuiLayer.h"

// ---Renderer------------------------
#include "Heron/Renderer/Buffer.h"
#include "Heron/Renderer/VertexArray.h"
#include "Heron/Renderer/Shader.h"
#include "Heron/Renderer/RenderCommand.h"

// ---Scene---------------------------
#include "Heron/Scene/Scene.h"
#include "Heron/Scene/Entity.h"
#include "Heron/Scene/Components.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
