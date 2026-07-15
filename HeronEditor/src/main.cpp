#include <QApplication>
#include "EditorWindow.h"
#include <Heron/Core/Log.h>
#include <Heron/Core/Input.h>
#include <Heron/Core/KeyCodes.h>
#include <Heron/Scene/Components.h>
#include <Heron/Scripting/ScriptableEntity.h>
#include <Heron/Scripting/ScriptRegistry.h>

// ============================================================
//  BUILT-IN SCRIPTS — Available in the Editor's Script dropdown
// ============================================================

class PlayerController : public Heron::ScriptableEntity
{
public:
    float Speed = 5.0f;

    void OnCreate() override {
        HRN_CORE_INFO("PlayerController: OnCreate");
    }

    void OnUpdate(float dt) override {
        auto& transform = GetComponent<Heron::TransformComponent>();

        if (Heron::Input::IsKeyPressed(Heron::Key::W))
            transform.Translation[2] -= Speed * dt;
        if (Heron::Input::IsKeyPressed(Heron::Key::S))
            transform.Translation[2] += Speed * dt;
        if (Heron::Input::IsKeyPressed(Heron::Key::A))
            transform.Translation[0] -= Speed * dt;
        if (Heron::Input::IsKeyPressed(Heron::Key::D))
            transform.Translation[0] += Speed * dt;
        if (Heron::Input::IsKeyPressed(Heron::Key::Space))
            transform.Translation[1] += Speed * dt;
        if (Heron::Input::IsKeyPressed(Heron::Key::LeftShift))
            transform.Translation[1] -= Speed * dt;
    }

    void OnDestroy() override {
        HRN_CORE_INFO("PlayerController: OnDestroy");
    }
};
HERON_REGISTER_SCRIPT(PlayerController);

class RotateScript : public Heron::ScriptableEntity
{
public:
    void OnUpdate(float dt) override {
        auto& transform = GetComponent<Heron::TransformComponent>();
        transform.Rotation[1] += 2.0f * dt; // spin around Y axis
    }
};
HERON_REGISTER_SCRIPT(RotateScript);

int main(int argc, char *argv[])
{
    freopen("stdout.log", "w", stdout);
    freopen("stderr.log", "w", stderr);
    Heron::Log::Init();
    QApplication a(argc, argv);
    
    // Set a dark fusion theme
    a.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    EditorWindow w;
    w.show();
    return a.exec();
}
