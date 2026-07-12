#pragma once

#include "Heron/Core/Core.h"
#include "Heron/Core/Window.h"
#include "Heron/Core/Timestep.h"
#include "Heron/Events/Event.h"
#include "Heron/Events/ApplicationEvent.h"
#include "Heron/Layers/LayerStack.h"
#include "Heron/ImGui/ImGuiLayer.h"

namespace Heron {

	class Application {
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		Window& GetWindow() { return *m_Window; }

		void Close();

		static Application& Get() { return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		bool  m_Running   = true;
		bool  m_Minimized = false;
		float m_LastFrameTime = 0.0f;

		static Application* s_Instance;
	};

	// To be defined by the client (sandbox/game)
	Application* CreateApplication();

}
