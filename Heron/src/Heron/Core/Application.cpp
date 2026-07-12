#include "hrnpch.h"
#include "Application.h"
#include "Log.h"
#include "Heron/Renderer/RenderCommand.h"

#include <GLFW/glfw3.h>

namespace Heron {

	Application* Application::s_Instance = nullptr;

	Application::Application() {
		HRN_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(HRN_BIND_EVENT_FN(Application::OnEvent));

		RenderCommand::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		HRN_CORE_INFO("Heron Engine initialized");
	}

	Application::~Application() {
		HRN_CORE_INFO("Heron Engine shutting down");
	}

	void Application::PushLayer(Layer* layer) {
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* overlay) {
		m_LayerStack.PushOverlay(overlay);
	}

	void Application::Close() {
		m_Running = false;
	}

	void Application::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HRN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HRN_BIND_EVENT_FN(Application::OnWindowResize));

		// Propagate events to layers in reverse order (overlays first)
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run() {
		HRN_CORE_INFO("Starting game loop");

		while (m_Running) {
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized) {
				RenderCommand::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
				RenderCommand::Clear();

				// Update all layers
				for (Layer* layer : m_LayerStack) {
					layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack) {
					layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		HRN_CORE_INFO("Window close requested");
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		HRN_CORE_INFO("Window resized to {0}x{1}", e.GetWidth(), e.GetHeight());

		if (e.GetWidth() == 0 || e.GetHeight() == 0) {
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		return false;
	}

}
