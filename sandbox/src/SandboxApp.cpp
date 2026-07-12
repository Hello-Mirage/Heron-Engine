#include "Heron/Heron.h"
#include "Heron/Core/EntryPoint.h"
#include <glad/gl.h>
#include <imgui.h>

class ExampleLayer : public Heron::Layer {
public:
	ExampleLayer()
		: Layer("Example")
	{
		m_VertexArray = Heron::VertexArray::Create();

		float vertices[3 * 3] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.0f,  0.5f, 0.0f
		};

		Heron::Ref<Heron::VertexBuffer> vertexBuffer = Heron::VertexBuffer::Create(vertices, sizeof(vertices));

		Heron::BufferLayout layout = {
			{ Heron::ShaderDataType::Float3, "a_Position" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[3] = { 0, 1, 2 };
		Heron::Ref<Heron::IndexBuffer> indexBuffer = Heron::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
			#version 330 core
			layout(location = 0) in vec3 a_Position;
			void main() {
				gl_Position = vec4(a_Position, 1.0);
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			layout(location = 0) out vec4 color;
			void main() {
				color = vec4(0.8, 0.2, 0.3, 1.0);
			}
		)";

		m_Shader = Heron::Shader::Create(vertexSrc, fragmentSrc);

		// ECS & Physics Test
		m_ActiveScene = Heron::CreateRef<Heron::Scene>();

		m_FloorEntity = m_ActiveScene->CreateEntity("Floor");
		auto& floorRb = m_FloorEntity.AddComponent<Heron::RigidBody3DComponent>();
		floorRb.Type = b3_staticBody;
		m_FloorEntity.GetComponent<Heron::TransformComponent>().Translation[1] = -5.0f;

		m_BoxEntity = m_ActiveScene->CreateEntity("Box");
		auto& boxRb = m_BoxEntity.AddComponent<Heron::RigidBody3DComponent>();
		boxRb.Type = b3_dynamicBody;
		m_BoxEntity.GetComponent<Heron::TransformComponent>().Translation[1] = 10.0f;
	}

	void OnUpdate(float dt) override {
		// Step Scene (Physics)
		m_ActiveScene->OnUpdateRuntime(dt);

		// Log Box Position every few frames to avoid spam
		static int frames = 0;
		if (frames++ % 60 == 0) {
			auto pos = m_BoxEntity.GetComponent<Heron::TransformComponent>().Translation;
			HRN_INFO("Box Position: [ {0}, {1}, {2} ]", pos[0], pos[1], pos[2]);
		}

		m_Shader->Bind();
		m_VertexArray->Bind();
		Heron::RenderCommand::DrawIndexed(m_VertexArray);
	}

	void OnEvent(Heron::Event& event) override {
		Heron::EventDispatcher dispatcher(event);
		dispatcher.Dispatch<Heron::KeyPressedEvent>([](Heron::KeyPressedEvent& e) {
			HRN_INFO("Key pressed: {0}", e.GetKeyCode());
			return false;
		});
	}

	void OnImGuiRender() override {
		ImGui::Begin("Settings");
		ImGui::Text("Hello from Heron Engine!");
		ImGui::End();
	}
private:
	Heron::Ref<Heron::Shader> m_Shader;
	Heron::Ref<Heron::VertexArray> m_VertexArray;

	Heron::Ref<Heron::Scene> m_ActiveScene;
	Heron::Entity m_BoxEntity;
	Heron::Entity m_FloorEntity;
};

class Sandbox : public Heron::Application {
public:
	Sandbox() {
		PushLayer(new ExampleLayer());
		HRN_INFO("Sandbox application created");
	}

	~Sandbox() {}
};

// Engine calls this to create the application
Heron::Application* Heron::CreateApplication() {
	return new Sandbox();
}
