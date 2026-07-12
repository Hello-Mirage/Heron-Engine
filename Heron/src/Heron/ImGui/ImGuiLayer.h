#pragma once

#include "Heron/Layers/Layer.h"
#include "Heron/Events/ApplicationEvent.h"
#include "Heron/Events/KeyEvent.h"
#include "Heron/Events/MouseEvent.h"

namespace Heron {

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

	private:
		float m_Time = 0.0f;
	};

}
