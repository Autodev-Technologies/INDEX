#include <IndexEngine.h>
#include <Index/Core/EntryPoint.h>

using namespace Index;

class Runtime : public Application
{
    friend class Application;

public:
    explicit Runtime()
        : Application()
    {
    }

    ~Runtime()
    {
    }

    void OnEvent(Event& e) override
    {
        Application::OnEvent(e);

        //if(Input::Get().GetKeyPressed(Index::InputCode::Key::Escape))
        //    Application::SetAppState(AppState::Closing);
    }

    void Init() override
    {
        std::vector<std::string> projectLocations = {
            "ExampleProject/Example.IndexProject",
            "/Users/jmorton/dev/Index/ExampleProject/Example.IndexProject",
            "../ExampleProject/Example.IndexProject",
            OS::Instance()->GetExecutablePath() + "/ExampleProject/Example.IndexProject",
            OS::Instance()->GetExecutablePath() + "/../ExampleProject/Example.IndexProject",
            OS::Instance()->GetExecutablePath() + "/../../ExampleProject/Example.IndexProject"
        };

        bool fileFound = false;
        std::string filePath;
        for(auto& path : projectLocations)
        {
            if(FileSystem::FileExists(path))
            {
                INDEX_LOG_INFO("Loaded Project {0}", path);
                m_ProjectSettings.m_ProjectRoot = StringUtilities::GetFileLocation(path);
                m_ProjectSettings.m_ProjectName = "Example";
                break;
            }
        }

        Application::Init();
        Application::SetEditorState(EditorState::Play);
        Application::Get().GetWindow()->SetWindowTitle("Runtime");
        Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Runtime::OnEvent));
    }

    void OnImGui() override
    {
        ImGui::Begin("Metrics");
        ImGui::Text("FPS : %.2f", (float)Index::Engine::Get().Statistics().FramesPerSecond);
        ImGui::End();
        Application::OnImGui();
    }
};

Index::Application* Index::CreateApplication()
{
    return new ::Runtime();
}
