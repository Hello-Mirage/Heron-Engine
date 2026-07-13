#include "Heron/Heron.h"
#include <iostream>

class PlayerController : public Heron::ScriptableEntity {
public:
	void OnCreate() override {
		std::cout << "PlayerController::OnCreate() from GameProject DLL!" << std::endl;
	}

	void OnDestroy() override {
		std::cout << "PlayerController::OnDestroy() from GameProject DLL!" << std::endl;
	}

	void OnUpdate(float ts) override {
		auto& transform = GetComponent<Heron::TransformComponent>();
		
		float speed = 5.0f * ts;
		if (Heron::Input::IsKeyPressed(Heron::Key::W))
			transform.Translation[2] -= speed;
		if (Heron::Input::IsKeyPressed(Heron::Key::S))
			transform.Translation[2] += speed;
		if (Heron::Input::IsKeyPressed(Heron::Key::A))
			transform.Translation[0] -= speed;
		if (Heron::Input::IsKeyPressed(Heron::Key::D))
			transform.Translation[0] += speed;
		if (Heron::Input::IsKeyPressed(Heron::Key::Space))
			transform.Translation[1] += speed;
		if (Heron::Input::IsKeyPressed(Heron::Key::LeftShift))
			transform.Translation[1] -= speed;
	}
};

HERON_REGISTER_SCRIPT(PlayerController);

extern "C" __declspec(dllexport) void InitializeGame() {
    // This function can be called by the Editor to verify DLL is loaded.
    std::cout << "GameProject DLL Initialized." << std::endl;
}
