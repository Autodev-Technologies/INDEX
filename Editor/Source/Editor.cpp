#include "Editor.h"
#include "SceneViewPanel.h"
#include "GameViewPanel.h"
#include "ConsolePanel.h"
#include "HierarchyPanel.h"
#include "InspectorPanel.h"
#include "ProfilerPanel.h"
//#include "GraphicsInfoPanel.h"
#include "SceneSettingsPanel.h"
#include "TextEditPanel.h"
#include "ProjectBroswerPanel.h"
#include "ImGUIConsoleSink.h"
#include "SettingsPanel.h"
//#include "ProjectSettingsPanel.h"

#include <Index/Graphics/Camera/EditorCamera.h>
#include <Index/Utilities/Timer.h>
#include <Index/Core/Application.h>
#include <Index/Core/OS/Input.h>
#include <Index/Core/OS/FileSystem.h>
#include <Index/Core/OS/OS.h>
#include <Index/Core/Version.h>
#include <Index/Core/Engine.h>
#include <Index/Audio/AudioManager.h>
#include <Index/Scene/Scene.h>
#include <Index/Scene/SceneManager.h>
#include <Index/Scene/Entity.h>
#include <Index/Scene/EntityManager.h>
#include <Index/Events/ApplicationEvent.h>
#include <Index/Scene/Component/Components.h>
#include <Index/Scene/Component/ModelComponent.h>
#include <Index/Scripting/Lua/LuaScriptComponent.h>
#include <Index/Physics/IndexPhysicsEngine/IndexPhysicsEngine.h>
#include <Index/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Index/Graphics/MeshFactory.h>
#include <Index/Graphics/Sprite.h>
#include <Index/Graphics/AnimatedSprite.h>
#include <Index/Graphics/Light.h>
#include <Index/Graphics/RHI/Texture.h>
#include <Index/Graphics/Camera/Camera.h>
#include <Index/Graphics/RHI/GraphicsContext.h>
#include <Index/Graphics/Renderers/GridRenderer.h>
#include <Index/Graphics/Renderers/DebugRenderer.h>
#include <Index/Graphics/Model.h>
#include <Index/Graphics/Environment.h>
#include <Index/Scene/EntityFactory.h>
#include <Index/Scene/Scene.h>
#include <Index/ImGui/IconsMaterialDesignIcons.h>
#include <Index/Embedded/EmbedAsset.h>
#include <Index/Scene/Component/ModelComponent.h>
#include <imgui/Plugins/imcmd_command_palette.h>

#include <imgui/imgui_internal.h>
#include <imgui/Plugins/ImGuizmo.h>
#include <imgui/Plugins/ImGuiAl/button/imguial_button.h>
#include <imgui/Plugins/ImTextEditor.h>
#include <imgui/Plugins/ImFileBrowser.h>
#include <iomanip>
#include <glm/gtx/matrix_decompose.hpp>

#include <cereal/version.hpp>

#include "EditorPanel.h"
#include <Embedded/INDEXHeader.inl>
#include <Embedded/WelcomeIndex.inl>
#include <Embedded/LargeIndexIcon.inl>

#include <Embedded/VideoPlayer.inl>
#include <Embedded/IndexAssteStore.inl>
#include <Embedded/IndexForums.inl>
#include <Embedded/LearnDocs.inl>
#include <Embedded/IndexAnswers.inl>

namespace Index
{

	Editor* Editor::s_Editor = nullptr;

	Editor::Editor()
		: Application()
		//, m_SelectedEntity(entt::null)
		//, m_CopiedEntity(entt::null)
		, m_IniFile("")
	{
		spdlog::sink_ptr sink = std::make_shared<ImGuiConsoleSink_mt>();

		Index::Debug::Log::AddSink(sink);
	}

	Editor::~Editor()
	{
	}

	void Editor::OnQuit()
	{
		bool SaveAndCloseWin = true;

		SaveEditorSettings();

		for(auto panel : m_Panels)
			panel->DestroyGraphicsResources();

		m_GridRenderer.reset();
		// m_PreviewRenderer.reset();
		m_PreviewTexture.reset();
		m_PreviewSphere.reset();
		m_Panels.clear();
		INDEXHeaderLogo.reset();
		WelcomeIndexLogo.reset();
		LargeIndexIconLogo.reset();
		VideoPlayerIcon.reset();
		IndexBasicsIcon.reset();
		IndexAnswersIcon.reset();
		IndexForumsIcon.reset();
		IndexAssteStoreIcon.reset();

		Application::OnQuit();
	}

	bool show_demo_window = false;
	bool show_command_palette = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImCmd::Command toggle_demo_cmd;

	void Editor::Init()
	{
		INDEX_PROFILE_FUNCTION();
		ImCmd::CreateContext();
		toggle_demo_cmd.Name = "Toggle ImGui demo window";
		toggle_demo_cmd.InitialCallback = [&]() {
			show_demo_window = !show_demo_window;
		};

		ImCmd::Command example_cmd;
		example_cmd.Name = "Open Project";
		example_cmd.InitialCallback = [&]() {
			ImGui::OpenPopup("Open Project");
		};

		ImCmd::Command add_example_cmd_cmd;
		add_example_cmd_cmd.Name = "Add 'Example command'";
		add_example_cmd_cmd.InitialCallback = [&]() {
			ImCmd::AddCommand(example_cmd);
		};

		ImCmd::Command remove_example_cmd_cmd;
		remove_example_cmd_cmd.Name = "Remove 'Example command'";
		remove_example_cmd_cmd.InitialCallback = [&]() {
			ImCmd::RemoveCommand(example_cmd.Name.c_str());
		},

		ImCmd::AddCommand(example_cmd); // Copy intentionally
		ImCmd::AddCommand(std::move(add_example_cmd_cmd));
		ImCmd::AddCommand(std::move(remove_example_cmd_cmd));

		auto& guizmoStyle = ImGuizmo::GetStyle();
		guizmoStyle.HatchedAxisLineThickness = -1.0f;

#ifdef INDEX_PLATFORM_IOS
		m_TempSceneSaveFilePath = OS::Instance()->GetAssetPath();
#else
#	ifdef INDEX_PLATFORM_LINUX
		m_TempSceneSaveFilePath = std::filesystem::current_path().string();
#	else
		m_TempSceneSaveFilePath = std::filesystem::temp_directory_path().string();
#	endif

		m_TempSceneSaveFilePath += "Index/";
		if(!FileSystem::FolderExists(m_TempSceneSaveFilePath))
			std::filesystem::create_directory(m_TempSceneSaveFilePath);

		std::vector<std::string> iniLocation = {
			StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "Editor.ini",
			StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../Editor.ini"};
		bool fileFound = false;
		std::string filePath;
		for(auto& path : iniLocation)
		{
			if(FileSystem::FileExists(path))
			{
				INDEX_LOG_INFO("Loaded Editor Ini file {0}", path);
				filePath = path;
				m_IniFile = IniFile(filePath);
				// ImGui::GetIO().IniFilename = ini[i];
				fileFound = true;
				LoadEditorSettings();
				break;
			}
		}

		if(!fileFound)
		{
			INDEX_LOG_INFO("Editor Ini not found");
#	ifdef INDEX_PLATFORM_MACOS
			filePath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../Editor.ini";
#	else
			filePath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "Editor.ini";
#	endif
			INDEX_LOG_INFO("Creating Editor Ini {0}", filePath);

			//  FileSystem::WriteTextFile(filePath, "");
			m_IniFile = IniFile(filePath);
			AddDefaultEditorSettings();
			// ImGui::GetIO().IniFilename = "editor.ini";
		}
#endif

		Application::Init();
		Application::SetEditorState(EditorState::Preview);
		Application::Get().GetWindow()->SetEventCallback(BIND_EVENT_FN(Editor::OnEvent));

		m_EditorCamera = CreateSharedPtr<Camera>(-20.0f,
			-40.0f,
			glm::vec3(-31.0f, 12.0f, 51.0f),
			60.0f,
			0.01f,
			m_Settings.m_CameraFar,
			(float)Application::Get().GetWindowSize().x / (float)Application::Get().GetWindowSize().y);
		m_CurrentCamera = m_EditorCamera.get();

		glm::mat4 viewMat = glm::inverse(glm::lookAt(glm::vec3(-31.0f, 12.0f, 51.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		m_EditorCameraTransform.SetLocalTransform(viewMat);
		m_EditorCameraTransform.SetWorldMatrix(glm::mat4(1.0f));

		m_ComponentIconMap[typeid(Graphics::Light).hash_code()] = ICON_MDI_LIGHTBULB;
		m_ComponentIconMap[typeid(Camera).hash_code()] = ICON_MDI_CAMERA;
		m_ComponentIconMap[typeid(SoundComponent).hash_code()] = ICON_MDI_VOLUME_HIGH;
		m_ComponentIconMap[typeid(Graphics::Sprite).hash_code()] = ICON_MDI_IMAGE;
		m_ComponentIconMap[typeid(Maths::Transform).hash_code()] = ICON_MDI_AXIS_ARROW;
		m_ComponentIconMap[typeid(RigidBody2DComponent).hash_code()] = ICON_MDI_SQUARE_OUTLINE;
		m_ComponentIconMap[typeid(RigidBody3DComponent).hash_code()] = ICON_MDI_CUBE_OUTLINE;
		m_ComponentIconMap[typeid(Graphics::ModelComponent).hash_code()] = ICON_MDI_BORDER_NONE;
		m_ComponentIconMap[typeid(Graphics::Model).hash_code()] = ICON_MDI_VECTOR_POLYGON;
		m_ComponentIconMap[typeid(LuaScriptComponent).hash_code()] = ICON_MDI_LANGUAGE_LUA;
		m_ComponentIconMap[typeid(Graphics::Environment).hash_code()] = ICON_MDI_EARTH;
		m_ComponentIconMap[typeid(Editor).hash_code()] = ICON_MDI_SQUARE;
		m_ComponentIconMap[typeid(TextComponent).hash_code()] = ICON_MDI_TEXT;
		m_ComponentIconMap[typeid(DefaultCameraController).hash_code()] = ICON_MDI_CAMERA_CONTROL;
		m_ComponentIconMap[typeid(TextureMatrixComponent).hash_code()] = ICON_MDI_VECTOR_ARRANGE_ABOVE;
		m_ComponentIconMap[typeid(Graphics::AnimatedSprite).hash_code()] = ICON_MDI_ANIMATION;
		m_ComponentIconMap[typeid(AxisConstraintComponent).hash_code()] = ICON_MDI_VECTOR_LINE;

		m_Panels.emplace_back(CreateSharedPtr<SceneViewPanel>());
		m_Panels.emplace_back(CreateSharedPtr<GameViewPanel>());
		m_Panels.emplace_back(CreateSharedPtr<InspectorPanel>());
		m_Panels.emplace_back(CreateSharedPtr<HierarchyPanel>());
		m_Panels.emplace_back(CreateSharedPtr<ProfilerPanel>());
		m_Panels.back()->SetActive(false);
		m_Panels.emplace_back(CreateSharedPtr<SettingsPanel>());
		m_Panels.back()->SetActive(false);
		m_Panels.emplace_back(CreateSharedPtr<SceneSettingsPanel>());
		m_Panels.back()->SetActive(false);
		m_Panels.emplace_back(CreateSharedPtr<ConsolePanel>());
#ifndef INDEX_PLATFORM_IOS
		m_Panels.emplace_back(CreateSharedPtr<ProjectBroswerPanel>());
#endif

		for(auto& panel : m_Panels)
			panel->SetEditor(this);

		CreateGridRenderer();

		m_Settings.m_ShowImGuiDemo = true;

		m_SelectedEntities.clear();
		// m_SelectedEntity = entt::null;
		m_PreviewTexture = nullptr;

		Application::Get().GetSystem<IndexPhysicsEngine>()->SetDebugDrawFlags(m_Settings.m_Physics3DDebugFlags);
		Application::Get().GetSystem<B2PhysicsEngine>()->SetDebugDrawFlags(m_Settings.m_Physics2DDebugFlags);

		ImGuiUtilities::SetTheme(m_Settings.m_Theme);
		OS::Instance()->SetTitleBarColour(ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
		OS::Instance()->SetTitleBarColour(ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg]);
		OS::Instance()->SetTitleBarColour(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
		Application::Get().GetWindow()->SetWindowTitle("Index Editor");

		ImGuizmo::SetGizmoSizeClipSpace(m_Settings.m_ImGuizmoScale);
		// ImGuizmo::SetGizmoSizeScale(Application::Get().GetWindowDPI());
	}
	
	bool Editor::IsTextFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "txt" || extension == "glsl" || extension == "shader" || extension == "vert"
			|| extension == "frag" || extension == "lua" || extension == "Lua" || extension == "cs" || extension == "CS")
			return true;

		return false;
	}

	bool Editor::IsFontFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "ttf")
			return true;

		return false;
	}

	bool Editor::IsAudioFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "ogg" || extension == "wav")
			return true;

		return false;
	}

	bool Editor::IsShaderFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "vert" || extension == "frag" || extension == "comp")
			return true;

		return false;
	}

	bool Editor::IsSceneFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "index")
			return true;

		return false;
	}

	bool Editor::IsModelFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);

		if(extension == "obj" || extension == "gltf" || extension == "glb" || extension == "fbx" || extension == "FBX")
			return true;

		return false;
	}

	bool Editor::IsTextureFile(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		std::string extension = StringUtilities::GetFilePathExtension(filePath);
		extension = StringUtilities::ToLower(extension);
		if(extension == "png" || extension == "tga" || extension == "jpg")
			return true;

		return false;
	}

	void Editor::OnImGui()
	{
		INDEX_PROFILE_FUNCTION();

		DrawMenuBar();

		BeginDockSpace(m_Settings.m_FullScreenOnPlay && Application::Get().GetEditorState() == EditorState::Play);

		for(auto& panel : m_Panels)
		{
			if(panel->Active())
				panel->OnImGui();
		}

		if(m_Settings.m_ShowImGuiDemo)
			ImGui::ShowDemoWindow(&m_Settings.m_ShowImGuiDemo);

		m_Settings.m_View2D = m_CurrentCamera->IsOrthographic();

		m_FileBrowserPanel.OnImGui();
		auto& io = ImGui::GetIO();

		auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
		if(ctrl && Input::Get().GetKeyPressed(Index::InputCode::Key::P))
		{
			show_command_palette = !show_command_palette;
		}
		if(show_command_palette)
		{
			ImCmd::CommandPaletteWindow("CommandPalette", &show_command_palette);
		}

		if(Application::Get().GetEditorState() == EditorState::Preview)
			Application::Get().GetSceneManager()->GetCurrentScene()->UpdateSceneGraph();

		EndDockSpace();

		Application::OnImGui();
	}

	Graphics::RenderAPI StringToRenderAPI(const std::string& name)
	{
#ifdef INDEX_RENDER_API_VULKAN
		if(name == "Vulkan")
			return Graphics::RenderAPI::VULKAN;
#endif
#ifdef INDEX_RENDER_API_OPENGL
		if(name == "OpenGL")
			return Graphics::RenderAPI::OPENGL;
#endif
#ifdef INDEX_RENDER_API_DIRECT3D
		if(name == "Direct3D11")
			return Graphics::RenderAPI::DIRECT3D;
#endif

		INDEX_LOG_ERROR("Unsupported Graphics API");

		return Graphics::RenderAPI::OPENGL;
	}

	void Editor::OpenFile()
	{
		INDEX_PROFILE_FUNCTION();

		// Set filePath to working directory
		auto path = OS::Instance()->GetExecutablePath();
		std::filesystem::current_path(path);
		m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(Editor::FileOpenCallback));
		m_FileBrowserPanel.Open();
	}

	void Editor::EmbedFile()
	{
		m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(Editor::FileEmbedCallback));
		m_FileBrowserPanel.Open();
	}

	static std::string projectLocation = "../";
	static bool reopenNewProjectPopup = false;
	static bool locationPopupOpened = false;

	void Editor::NewProjectLocationCallback(const std::string& path)
	{
		projectLocation = path;
		m_NewProjectPopupOpen = false;
		reopenNewProjectPopup = true;
		locationPopupOpened = false;
	}

	void Editor::DrawMenuBar()
	{
		INDEX_PROFILE_FUNCTION();

		// INDEX BUG REPORTER FUNCION
		bool INDEX_BUG_REPORTER = false;

		bool ExistProjectPopupOpen = false;
		bool openSaveScenePopup = false;
		bool SaveAndCloseWin = false;
		bool SaveAndClose = false;
		bool openNewScenePopup = false;
		bool openReloadScenePopup = false;
		bool openAboutIndex = false;
		bool openWelcomeINDEX = false;
		bool openCheckforUpdates = false;
		bool openProjectLoadPopup = !m_ProjectLoaded;

		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
		if(ImGui::BeginMainMenuBar())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 0));

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "CTRL+N"))
				{
					openNewScenePopup = true;
				}

				if (ImGui::MenuItem("Reload Scene", "CTRL+R"))
				{
					openReloadScenePopup = true;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save Scene", "CTRL+S"))
				{
					openSaveScenePopup = true;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("New Project"))
				{
					reopenNewProjectPopup = false;
					openProjectLoadPopup = true;
				}

				if (ImGui::MenuItem("Open Project"))
				{
					ExistProjectPopupOpen = true;
				}

				if(ImGui::MenuItem("Open File"))
				{
					m_FileBrowserPanel.SetCurrentPath(m_ProjectSettings.m_ProjectRoot);
					m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(Editor::FileOpenCallback));
					m_FileBrowserPanel.Open();
				}

				ImGui::Separator();

				if(ImGui::MenuItem("Exit"))
				{
					SaveAndClose = true;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
			    if(ImGui::MenuItem("Undo", "CTRL+Z"))
			    {

			    }

			    if(ImGui::MenuItem("Redo", "CTRL+Y", false, false))
				{
				}

				ImGui::Separator();

				bool enabled = !m_SelectedEntities.empty();

				if(ImGui::MenuItem("Cut", "CTRL+X", false, enabled))
			    {
					for(auto entity : m_SelectedEntities)
						SetCopiedEntity(entity, true);
				}

				if(ImGui::MenuItem("Copy", "CTRL+C", false, enabled))
				{
					for(auto entity : m_SelectedEntities)
						SetCopiedEntity(entity, false);
				}

				enabled = !m_CopiedEntities.empty();

				if(ImGui::MenuItem("Paste", "CTRL+V", false, enabled))
				{
					for(auto entity : m_CopiedEntities)
					{
						Application::Get().GetCurrentScene()->DuplicateEntity({entity, Application::Get().GetCurrentScene()});
						if(entity != entt::null)
						{
							/// if(entity == m_SelectedEntity)
							///  m_SelectedEntity = entt::null;
							Entity(entity, Application::Get().GetCurrentScene()).Destroy();
						}
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Assets"))
			{
				auto scene = Application::Get().GetSceneManager()->GetCurrentScene();

				if (ImGui::MenuItem("CreateEmpty"))
				{
					scene->CreateEntity();
				}

				if (ImGui::MenuItem("Cube"))
				{
					auto entity = scene->CreateEntity("Cube");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);
				}

				if (ImGui::MenuItem("Sphere"))
				{
					auto entity = scene->CreateEntity("Sphere");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
				}

				if (ImGui::MenuItem("Pyramid"))
				{
					auto entity = scene->CreateEntity("Pyramid");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Pyramid);
				}

				if (ImGui::MenuItem("Plane"))
				{
					auto entity = scene->CreateEntity("Plane");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Plane);
				}

				if (ImGui::MenuItem("Cylinder"))
				{
					auto entity = scene->CreateEntity("Cylinder");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cylinder);
				}

				if (ImGui::MenuItem("Capsule"))
				{
					auto entity = scene->CreateEntity("Capsule");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Capsule);
				}

				//if (ImGui::MenuItem("Terrain"))
				//{
				//	auto entity = scene->CreateEntity("Terrain");
				//	entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Terrain);
				//}

				if (ImGui::MenuItem("Light Cube"))
				{
					EntityFactory::AddLightCube(Application::Get().GetSceneManager()->GetCurrentScene(), glm::vec3(0.0f), glm::vec3(0.0f));
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("GameObject"))
			{
				auto scene = Application::Get().GetSceneManager()->GetCurrentScene();

				if (ImGui::MenuItem("Create Empty", "Ctrl+Shift+N"))
				{
					scene->CreateEntity();
				}

				if (ImGui::MenuItem("Cube"))
				{
					auto entity = scene->CreateEntity("Cube");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);
				}

				if (ImGui::MenuItem("Sphere"))
				{
					auto entity = scene->CreateEntity("Sphere");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
				}

				if (ImGui::MenuItem("Pyramid"))
				{
					auto entity = scene->CreateEntity("Pyramid");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Pyramid);
				}

				if (ImGui::MenuItem("Plane"))
				{
					auto entity = scene->CreateEntity("Plane");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Plane);
				}

				if (ImGui::MenuItem("Cylinder"))
				{
					auto entity = scene->CreateEntity("Cylinder");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cylinder);
				}

				if (ImGui::MenuItem("Capsule"))
				{
					auto entity = scene->CreateEntity("Capsule");
					entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Capsule);
				}

				//if (ImGui::MenuItem("Terrain"))
				//{
				//	auto entity = scene->CreateEntity("Terrain");
				//	entity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Terrain);
				//}

				if (ImGui::MenuItem("Light Cube"))
				{
					EntityFactory::AddLightCube(Application::Get().GetSceneManager()->GetCurrentScene(), glm::vec3(0.0f), glm::vec3(0.0f));
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scenes"))
			{
				auto scenes = Application::Get().GetSceneManager()->GetSceneNames();

				for (size_t i = 0; i < scenes.size(); i++)
				{
					auto name = scenes[i];
					if (ImGui::MenuItem(name.c_str()))
					{
						Application::Get().GetSceneManager()->SwitchScene(name);
					}
				}

				ImGui::EndMenu();
			}

			//if (ImGui::BeginMenu("Graphics"))
			//{
			//	if (ImGui::MenuItem("Compile Shaders"))
			//	{
			//		RecompileShaders();
			//	}
			//	if (ImGui::MenuItem("Embed Shaders"))
			//	{
			//		std::string coreDataPath;
			//		VFS::Get().ResolvePhysicalPath("//CoreShaders", coreDataPath, true);
			//		auto shaderPath = std::filesystem::path(coreDataPath + "/CompiledSPV/");
			//		int shaderCount = 0;
			//		if (std::filesystem::is_directory(shaderPath))
			//		{
			//			for (auto entry : std::filesystem::directory_iterator(shaderPath))
			//			{
			//				auto extension = StringUtilities::GetFilePathExtension(entry.path().string());
			//				if (extension == "spv")
			//				{
			//					EmbedShader(entry.path().string());
			//					shaderCount++;
			//				}
			//			}
			//		}
			//		INDEX_LOG_INFO("Embedded {0} shaders. Recompile to use", shaderCount);
			//	}
			//	if (ImGui::MenuItem("Embed File"))
			//	{
			//		EmbedFile();
			//	}

			//	if (ImGui::BeginMenu("GPU Index"))
			//	{
			//		uint32_t gpuCount = Graphics::Renderer::GetRenderer()->GetGPUCount();

			//		if (gpuCount == 1)
			//		{
			//			ImGui::TextUnformatted("Default");
			//			ImGuiUtilities::Tooltip("Only default GPU selectable");
			//		}
			//		else
			//		{
			//			int8_t currentDesiredIndex = Application::Get().GetProjectSettings().DesiredGPUIndex;
			//			int8_t newIndex = currentDesiredIndex;

			//			if (ImGui::Selectable("Default", currentDesiredIndex == -1))
			//			{
			//				newIndex = -1;
			//			}

			//			for (uint32_t index = 0; index < gpuCount; index++)
			//			{
			//				if (ImGui::Selectable(std::to_string(index).c_str(), index == uint32_t(currentDesiredIndex)))
			//				{
			//					newIndex = index;
			//				}
			//			}

			//			Application::Get().GetProjectSettings().DesiredGPUIndex = newIndex;
			//		}
			//		ImGui::EndMenu();
			//	}
			//	ImGui::EndMenu();
			//}

			if (ImGui::BeginMenu("Window"))
			{
				for (auto& panel : m_Panels)
				{
					if (ImGui::MenuItem(panel->GetName().c_str(), "", &panel->Active(), true))
					{
						panel->SetActive(true);
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{

				if (ImGui::MenuItem("About Index...", "", ImGuiWindowFlags_Popup))
				{
					openAboutIndex = true;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Index Manual...", "", ImGuiWindowFlags_Popup))
				{
					Index::OS::Instance()->OpenURL("https://github.com/INDEV-Technologies/INDEX-1.x/wiki");
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Index Forums...", "", ImGuiWindowFlags_Popup))
				{
					Index::OS::Instance()->OpenURL("https://github.com/INDEV-Technologies/INDEX-1.x/issues");
				}

				if (ImGui::MenuItem("Welcome To INDEX", "", ImGuiWindowFlags_Popup))
				{
					openWelcomeINDEX = true;
				}

				if (ImGui::MenuItem("Check for Updates", NULL, ImGuiWindowFlags_Popup))
				{
					openCheckforUpdates = true;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Download Source Code"))
				{
					Index::OS::Instance()->OpenURL("https://github.com/INDEV-Technologies/INDEX-1.x");
				}

				if (ImGui::MenuItem("Report a bug"))
				{
					INDEX_BUG_REPORTER = true;
				}

				ImGui::EndMenu();
			}

			static Engine::Stats stats = {};
			static double timer = 1.1;
			timer += Engine::GetTimeStep().GetSeconds();

			if(timer > 1.0)
			{
				timer = 0.0;
				stats = Engine::Get().Statistics();
			}

			bool setNewValue = false;
			static std::string RenderAPI = "";

			auto renderAPI = (Graphics::RenderAPI)m_ProjectSettings.RenderAPI;

			bool needsRestart = false;
			if(renderAPI != Graphics::GraphicsContext::GetRenderAPI())
			{
				needsRestart = true;
			}

			switch(renderAPI)
			{
#ifdef INDEX_RENDER_API_OPENGL
			case Graphics::RenderAPI::OPENGL:
				RenderAPI = "OpenGL";
				break;
#endif

#ifdef INDEX_RENDER_API_VULKAN
			case Graphics::RenderAPI::VULKAN:
				RenderAPI = "Vulkan";
				break;
#endif

#ifdef INDEX_RENDER_API_DIRECT3D
			case DIRECT3D:
				RenderAPI = "Direct3D";
				break;
#endif
			default:
				break;
			}

			int numSupported = 0;
#ifdef INDEX_RENDER_API_OPENGL
			numSupported++;
#endif
#ifdef INDEX_RENDER_API_VULKAN
			numSupported++;
#endif
#ifdef INDEX_RENDER_API_DIRECT3D11
			numSupported++;
#endif
			const char* api[] = {"OpenGL", "Vulkan", "Direct3D11"};
			const char* current_api = RenderAPI.c_str();
			if(needsRestart)
				RenderAPI = "*" + RenderAPI;

			if(needsRestart)
				ImGuiUtilities::Tooltip("Restart needed to switch Render API");

			if(setNewValue)
			{
				m_ProjectSettings.RenderAPI = int(StringToRenderAPI(current_api));
			}

			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar();
			ImGui::EndMainMenuBar();
		}
		ImGui::PopStyleColor();

		if (openSaveScenePopup)
			ImGui::OpenPopup("Save Scene");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Save Current Scene Changes?\n\n");
			ImGui::Separator();

			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/", false);
				Graphics::Renderer::GetRenderer()->SaveScreenshot(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/Cache/" + Application::Get().GetSceneManager()->GetCurrentScene()->GetSceneName() + ".png", m_RenderPasses->GetForwardData().m_RenderTexture);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// INDEX BUG REPORTER FUNCION

		LargeIndexIconLogo = Graphics::Texture2D::CreateFromSource(LargeIndexIconWidth, LargeIndexIconHeight, (void*)LargeIndexIcon);

		int my_image_width = 66;
		int my_image_height = 74;

		if (INDEX_BUG_REPORTER)
			ImGui::OpenPopup( ICON_MDI_BUG "Index Bug Reporter");

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
		if (ImGui::BeginPopupModal( ICON_MDI_BUG "Index Bug Reporter", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::TextUnformatted(" ");
			ImGui::TextUnformatted(" Please take a minute to describe the problem in");
			ImGui::TextUnformatted(" as mush detail as possible. This makes it possible ");
			ImGui::TextUnformatted(" for us to fix it.");
			ImGui::Separator();

			ImGui::TextUnformatted(" Type of problem: ");
			ImGui::SameLine();

			static const char* TypeOfProblem[]{ "Please Specify", "A problem with the Editor", "A problem with the Player", "I'd like to request a feature", "Documentation", "Crash Bug"};
			static int selectedTypeOfProblem = 0;
			static const char* HowOthenHappen[]{ "Please Specify", "Always", "Sometimes, but not always", "This is the first time" };
			static int selectedHowOthenHappen = 0;

			ImGui::Combo(" ", &selectedTypeOfProblem, TypeOfProblem, IM_ARRAYSIZE(TypeOfProblem));

			ImGui::TextUnformatted(" How Othen Does it Happen:");
			ImGui::SameLine();

			ImGui::Combo("  ", &selectedHowOthenHappen, HowOthenHappen, IM_ARRAYSIZE(HowOthenHappen));

			ImGui::TextUnformatted(" Your Github Name:");
			ImGui::SameLine();

			static std::string GithubName = " ";
			ImGuiUtilities::InputText(GithubName);

			ImGui::TextUnformatted(" Problem Detailes");

			ImGui::TextUnformatted("                                           ");
			ImGui::SameLine();

			if (ImGui::Button("Send Error Report"))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();

			if (ImGui::Button("Don't Send"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		ImGui::SetWindowSize( ICON_MDI_BUG "Index Bug Reporter", ImVec2(410, 446));

		if(locationPopupOpened)
		{
			// Cancel clicked on project location popups
			if(!m_FileBrowserPanel.IsOpen())
			{
				m_NewProjectPopupOpen = false;
				locationPopupOpened = false;
				reopenNewProjectPopup = true;
			}
		}
		if(openNewScenePopup)
			ImGui::OpenPopup("New Scene");

		if(ImGui::BeginPopupModal("New Scene", NULL))
		{
			ImGui::Text(" ");
			ImGui::Text(" ");
			ImGui::SameLine();
			ImGui::Image(LargeIndexIconLogo, ImVec2(my_image_width, my_image_height));
			ImGui::SameLine();
			ImGui::Text("Do you want to save the changes you made in the scene?");
			ImGui::Text("                  ");
			ImGui::SameLine();
			ImGui::Text("Your changes will be lost if you don't save them");

			ImGui::Text("                      ");
			ImGui::Text("                                                      ");
			ImGui::SameLine();
			if (ImGui::Button("Save", ImVec2(73, 0)))
			{
				Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/", false);
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(73, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();

			ImGui::Text(" ");
			ImGui::Text(" ");
			ImGui::SameLine();
			ImGui::Image(LargeIndexIconLogo, ImVec2(my_image_width, my_image_height));
			ImGui::SameLine();
			ImGui::Text("Do you want to make a scene?\n\n");

			static bool defaultSetup = false;

			static std::string newSceneName = "Untitled";
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Name : ");
			ImGui::SameLine();
			ImGuiUtilities::InputText(newSceneName);

			ImGui::Checkbox("Set Default Setup", &defaultSetup);

			ImGui::Separator();

			if(ImGui::Button("Create", ImVec2(73, 0)))
			{
				std::string sceneName = newSceneName;
				int sameNameCount = 0;
				auto sceneNames = m_SceneManager->GetSceneNames();

				while(FileSystem::FileExists("//Scenes/" + sceneName + ".Index") || std::find(sceneNames.begin(), sceneNames.end(), sceneName) != sceneNames.end())
				{
					sameNameCount++;
					sceneName = fmt::format(newSceneName + "{0}", sameNameCount);
				}
				auto scene = new Scene(sceneName);

				if(defaultSetup)
				{
					auto light = scene->GetEntityManager()->Create("Light");
					auto lightComp = light.AddComponent<Graphics::Light>();
					glm::mat4 lightView = glm::inverse(glm::lookAt(glm::vec3(30.0f, 9.0f, 50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
					light.GetTransform().SetLocalTransform(lightView);

					auto camera = scene->GetEntityManager()->Create("Camera");
					camera.AddComponent<Camera>();
					glm::mat4 viewMat = glm::inverse(glm::lookAt(glm::vec3(-2.3f, 1.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
					camera.GetTransform().SetLocalTransform(viewMat);
					camera.AddComponent<DefaultCameraController>(DefaultCameraController::ControllerType::EditorCamera);

					auto cube = scene->GetEntityManager()->Create("Cube");
					cube.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Cube);

					auto environment = scene->GetEntityManager()->Create("Environment");
					environment.AddComponent<Graphics::Environment>();
					environment.GetComponent<Graphics::Environment>().Load();

					scene->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes/");
				}
				Application::Get().GetSceneManager()->EnqueueScene(scene);
				Application::Get().GetSceneManager()->SwitchScene((int)(Application::Get().GetSceneManager()->GetScenes().size()) - 1);

				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if(ImGui::Button("Cancel", ImVec2(73, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::SetWindowSize("New Scene", ImVec2(450, 190));

		static int tab = 0;

		if (ExistProjectPopupOpen)
			ImGui::OpenPopup("Index - Open Project");

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		if (ImGui::BeginPopupModal("Index - Open Project", NULL))
		{

			if (ImGui::BeginTabBar(""))
			{
				if (ImGui::BeginTabItem("Open a Project"))
				{
					tab = 1;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Create New Project"))
				{
					tab = 2;
					ImGui::EndTabItem();
				}
			}

			switch (tab)
			{
			case 1: // Main tab
				ImGui::TextUnformatted("");

				ImGui::Text(" ");

				for (auto& recentProject : m_Settings.m_RecentProjects)
				{
					if (ImGui::MenuItem(recentProject.c_str()))
					{
						Application::Get().OpenProject(recentProject);

						for (int i = 0; i < int(m_Panels.size()); i++)
						{
							m_Panels[i]->OnNewProject();
						}
					}
				}
				ImGui::PopID();

				ImGui::Text(" ");

				if (ImGui::Button("Open Other"))
				{
					ImGui::CloseCurrentPopup();

					m_NewProjectPopupOpen = true;
					locationPopupOpened = true;

					// Set filePath to working directory
					const auto& path = OS::Instance()->GetExecutablePath();
					auto& browserPath = m_FileBrowserPanel.GetPath();
					browserPath = std::filesystem::path(path);
					m_FileBrowserPanel.SetFileTypeFilters({ ".IndexProject" });
					m_FileBrowserPanel.SetOpenDirectory(false);
					m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(ProjectOpenCallback));
					m_FileBrowserPanel.Open();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(73, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
			}

			switch (tab)
			{
			case 2: // Main tab

				ImGui::TextUnformatted("  ");
				ImGui::Text(" ");
				ImGui::SameLine();

				static std::string newProjectName = "My Project";
				ImGuiUtilities::InputText(newProjectName);
				ImGui::TextUnformatted("  Project Name*\n");
				ImGui::Text(" ");
				ImGui::Text("  ");
				ImGui::SameLine();
				if (ImGui::Button(ICON_MDI_FOLDER))
				{
					ImGui::CloseCurrentPopup();

					m_NewProjectPopupOpen = true;
					locationPopupOpened = true;

					// Set filePath to working directory
					const auto& path = OS::Instance()->GetExecutablePath();
					auto& browserPath = m_FileBrowserPanel.GetPath();
					browserPath = std::filesystem::path(path);
					m_FileBrowserPanel.ClearFileTypeFilters();
					m_FileBrowserPanel.SetOpenDirectory(true);
					m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(NewProjectLocationCallback));
					m_FileBrowserPanel.Open();
				}

				ImGui::SameLine();

				ImGui::TextUnformatted(projectLocation.c_str());
				ImGui::TextUnformatted("  Location*\n");

				ImGui::Text("  ");
				ImGui::Text("  ");
				ImGui::SameLine();
				ImGui::TextUnformatted("  Setup defaults for:\n");
				ImGui::Text("  ");
				ImGui::SameLine();

				static const char* SetupDim[]{ "3D", "2D" };
				static int selectedSetupDim = 0;

				ImGui::Combo(" ", &selectedSetupDim, SetupDim, IM_ARRAYSIZE(SetupDim));

				ImGui::Text("  ");

				ImGui::Text(" ");
				ImGui::SameLine();


				ImGui::Text(" ");
				ImGui::SameLine();

				if (ImGui::Button("Create", ImVec2(73, 0)))
				{
					Application::Get().OpenNewProject(projectLocation, newProjectName);
					m_FileBrowserPanel.SetOpenDirectory(false);

					for (int i = 0; i < int(m_Panels.size()); i++)
					{
						m_Panels[i]->OnNewProject();
					}

					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Exit", ImVec2(73, 0)))
				{
					ImGui::CloseCurrentPopup();
					SetAppState(AppState::Closing);
				}

				if (Application::Get().m_ProjectLoaded)
				{
					ImGui::SameLine();

					if (ImGui::Button("Cancel", ImVec2(73, 0)))
					{
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		ImGui::SetWindowSize("Index - Open Project", ImVec2(556, 328));

		if ((reopenNewProjectPopup || openProjectLoadPopup) && !m_NewProjectPopupOpen)
		{
			ImGui::OpenPopup("Index 1.0.0f1");
			reopenNewProjectPopup = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		if(ImGui::BeginPopupModal("Index 1.0.0f1", NULL))
		{

			if (ImGui::BeginTabBar(""))
			{
				if (ImGui::BeginTabItem("Open Project"))
				{
					tab = 1;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Create New Project"))
				{
					tab = 2;
					ImGui::EndTabItem();
				}
			}

			switch (tab)
			{
			case 1: // Main tab
				ImGui::TextUnformatted("");

				ImGui::Text(" ");

				for (auto& recentProject : m_Settings.m_RecentProjects)
				{
					if (ImGui::MenuItem(recentProject.c_str()))
					{
						Application::Get().OpenProject(recentProject);

						for (int i = 0; i < int(m_Panels.size()); i++)
						{
							m_Panels[i]->OnNewProject();
						}
					}
				}
				ImGui::PopID();

				ImGui::Text(" ");

				if (ImGui::Button("Open Other"))
				{
					ImGui::CloseCurrentPopup();

					m_NewProjectPopupOpen = true;
					locationPopupOpened = true;

					// Set filePath to working directory
					const auto& path = OS::Instance()->GetExecutablePath();
					auto& browserPath = m_FileBrowserPanel.GetPath();
					browserPath = std::filesystem::path(path);
					m_FileBrowserPanel.SetFileTypeFilters({ ".IndexProject" });
					m_FileBrowserPanel.SetOpenDirectory(false);
					m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(ProjectOpenCallback));
					m_FileBrowserPanel.Open();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(73, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
			}

			switch (tab)
			{
			case 2: // Main tab

				ImGui::TextUnformatted("  ");
				ImGui::Text(" ");
				ImGui::SameLine();

				static std::string newProjectName = "My Project";
				ImGuiUtilities::InputText(newProjectName);
				ImGui::TextUnformatted("  Project Name*\n");
				ImGui::Text("  ");
				if (ImGui::Button(ICON_MDI_FOLDER))
				{
					ImGui::CloseCurrentPopup();

					m_NewProjectPopupOpen = true;
					locationPopupOpened = true;

					// Set filePath to working directory
					const auto& path = OS::Instance()->GetExecutablePath();
					auto& browserPath = m_FileBrowserPanel.GetPath();
					browserPath = std::filesystem::path(path);
					m_FileBrowserPanel.ClearFileTypeFilters();
					m_FileBrowserPanel.SetOpenDirectory(true);
					m_FileBrowserPanel.SetCallback(BIND_FILEBROWSER_FN(NewProjectLocationCallback));
					m_FileBrowserPanel.Open();
				}

				ImGui::SameLine();

				ImGui::TextUnformatted(projectLocation.c_str());
				ImGui::TextUnformatted("  Location*\n");

				ImGui::TextUnformatted("  Setup defaults for:\n");
				ImGui::Text("  ");
				ImGui::SameLine();

				static const char* SetupDim[]{ "3D", "2D"};
				static int selectedSetupDim = 0;

				ImGui::Combo(" ", &selectedSetupDim, SetupDim, IM_ARRAYSIZE(SetupDim));

				ImGui::Text("  ");

				ImGui::Text(" ");
				ImGui::SameLine();


				ImGui::Text(" ");
				ImGui::SameLine();

				if (ImGui::Button("Create", ImVec2(73, 0)))
				{
					Application::Get().OpenNewProject(projectLocation, newProjectName);
					m_FileBrowserPanel.SetOpenDirectory(false);

					for (int i = 0; i < int(m_Panels.size()); i++)
					{
						m_Panels[i]->OnNewProject();
					}

					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Exit", ImVec2(73, 0)))
				{
					ImGui::CloseCurrentPopup();
					SetAppState(AppState::Closing);
				}

				if (Application::Get().m_ProjectLoaded)
				{
					ImGui::SameLine();

					if (ImGui::Button("Cancel", ImVec2(73, 0)))
					{
						ImGui::CloseCurrentPopup();
					}
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		ImGui::SetWindowSize("Index 1.0.0f1", ImVec2(566, 328));

		if(openReloadScenePopup)
			ImGui::OpenPopup("Reload Scene");

		if(ImGui::BeginPopupModal("Reload Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(" ");
			ImGui::Text(" ");
			ImGui::SameLine();

			ImGui::Image(LargeIndexIconLogo, ImVec2(my_image_width, my_image_height));
			ImGui::SameLine();

			ImGui::Text("Reload Scene?\n\n");
			ImGui::Separator();

			if(ImGui::Button("OK", ImVec2(120, 0)))
			{
				auto scene = new Scene("New Scene");
				Application::Get().GetSceneManager()->SwitchScene(Application::Get().GetSceneManager()->GetCurrentSceneIndex());

				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if(ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if(openAboutIndex)
			ImGui::OpenPopup("About Index");

		if(ImGui::BeginPopupModal("About Index", NULL, ImGuiWindowFlags_NoResize))
		{
			INDEXHeaderLogo = Graphics::Texture2D::CreateFromSource(INDEXHeaderWidth, INDEXHeaderHeight, (void*)INDEXHeader);

			int my_image_width = 175;
			int my_image_height = 58;

			ImGui::Text("  ");
			ImGui::Text("  ");
			ImGui::SameLine();
			ImGui::Image(INDEXHeaderLogo, ImVec2(my_image_width, my_image_height));
			ImGui::SameLine();
			ImGui::Text("                                                                               ");
			ImGui::SameLine();
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::Text("");
			ImGui::Text("     Version 1.0.0f1 Personal");
			ImGui::Text("");
			ImGui::Text("     Saul Alejandro, Khronos Group Inc, AMD,Omar Cornut, Cedric Guillemet, Michele Caini,");
			ImGui::Text("     Marcus Geelnard, Camilla Lowy ,Gabi Melman, Sean Barrett, Syoyo Fujita, Aurelien");
			ImGui::Text("     Chatelain, David Herberth, Erin Catto, Rapptz, ThePhD, Randolph Voorhies, Shane Grant");
			ImGui::Text("  ");
			ImGui::Text("     Lua Scripting powered by Sol2                         by Graphics Librarie powered by Vulkan");
			ImGui::Text("     (c) 2013-2023 Rapptz, ThePhD                         (c) 2000-2023 Khronos Group, Inc");
			ImGui::Text("                                                                          (c) 1969-2023 AMD, Inc");
			ImGui::Text("");
			ImGui::Text("     Special thanks to our beta users");
			ImGui::Text("     (c) 2022-2023 Indev Technologies. All rights reserved");
			ImGui::Text("     (c) 2021-2023 Indev Corporation. All rights reserved            License type: Index Personal");
			ImGui::Text(" ");
			ImGui::SameLine();

			ImGui::EndPopup();
		}
		ImGui::SetWindowSize("About Index", ImVec2(570, 365));

		if (openCheckforUpdates)
			ImGui::OpenPopup("Index Editor Update Check");

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
		if (ImGui::BeginPopupModal("Index Editor Update Check", NULL, ImGuiWindowFlags_NoResize))
		{


			ImGui::Text("   ");
			ImGui::SameLine();
			ImGui::Image(LargeIndexIconLogo, ImVec2(my_image_width, my_image_height));
			ImGui::SameLine();
			ImGui::Text("   The INDEX Editor is up-to-date. Currently installed version is 1.0.0f1");
			ImGui::Text(" ");
			ImGui::Text("");
			ImGui::SameLine();

			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		ImGui::SetWindowSize("Index Editor Update Check", ImVec2(570, 425));

		if (ImGui::BeginPopupModal("License Management", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("");
			ImGui::Text("");
			ImGui::Text("");

			ImGui::Text("     ");
			ImGui::SameLine();
			if (ImGui::Button("Check for updates"))
			{	
			}
			ImGui::SameLine();
			ImGui::Text("  Checks for updates to the currently installed license.");

			ImGui::Text("     ");
			ImGui::SameLine();
			ImGui::Button("Activate new license");
			ImGui::SameLine();
			ImGui::Text("  Activate Index with a different serial number.");

			ImGui::Text("     ");
			ImGui::SameLine();
			ImGui::Button("Return license");
			ImGui::SameLine();
			ImGui::Text("  Return this license and free an activation for the serial it is using.");

			ImGui::Text("     ");
			ImGui::SameLine();
			ImGui::Button("Manual activation");
			ImGui::SameLine();
			ImGui::Text("  Start the manual activation process by a file activation in the machine.");

			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::Text("     ");
			ImGui::SameLine();

			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
		ImGui::SetWindowSize("License Management", ImVec2(600, 355));

		if(openWelcomeINDEX)
			ImGui::OpenPopup("Welcome To INDEX");

		if(ImGui::BeginPopupModal("Welcome To INDEX", NULL, ImGuiWindowFlags_NoResize))
		{
			WelcomeIndexLogo = Graphics::Texture2D::CreateFromSource(WelcomeIndexWidth, WelcomeIndexHeight, (void*)WelcomeIndex);

			int WelcomeLogoWidth = 570;
			int WelcomeLogoheight = 72;

			VideoPlayerIcon = Graphics::Texture2D::CreateFromSource(VideoPlayerWidth, VideoPlayerHeight, (void*)VideoPlayer);

			int VideoPlayerWidth = 45;
			int VideoPlayerheight = 45;

			IndexBasicsIcon = Graphics::Texture2D::CreateFromSource(LearnDocsWidth, LearnDocsHeight, (void*)LearnDocs);

			int IndexBasicsIconWidth = 46;
			int IndexBasicsIconheight = 34;

			IndexAnswersIcon = Graphics::Texture2D::CreateFromSource(IndexAnswersWidth, IndexAnswersHeight, (void*)IndexAnswers);

			int IndexAnswersIconWidth = 47;
			int IndexAnswersIconheight = 35;

			IndexForumsIcon = Graphics::Texture2D::CreateFromSource(IndexForumsWidth, IndexForumsHeight, (void*)IndexForums);

			int IndexForumsIconWidth = 47;
			int IndexForumsIconheight = 35;

			IndexAssteStoreIcon = Graphics::Texture2D::CreateFromSource(IndexAssteStoreWidth, IndexAssteStoreHeight, (void*)IndexAssteStore);

			int IndexAssteStoreIconWidth = 38;
			int IndexAssteStoreIconheight = 46;

			ImGui::Image(WelcomeIndexLogo, ImVec2(WelcomeLogoWidth, WelcomeLogoheight));
			ImGui::Text("                                       as you dive into Index, may we suggest a few hints");
			ImGui::Text("                                       to get you off to a good start?");
			ImGui::Text("");
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::Image(VideoPlayerIcon, ImVec2(VideoPlayerWidth, VideoPlayerheight));
			ImGui::SameLine();
			ImGui::Text("  Video Tutorials");
			ImGui::Text("                                We have collected some videos to make you productive");
			ImGui::Text("                                immediately.");
			ImGui::Text("");
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::Image(IndexBasicsIcon, ImVec2(IndexBasicsIconWidth, IndexBasicsIconheight));
			ImGui::SameLine();
			ImGui::Text("  Index Basics");
			ImGui::Text("                                Take a look at our manual for a quick startup guide.");
			ImGui::Text("");
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::Image(IndexAnswersIcon, ImVec2(IndexAnswersIconWidth, IndexAnswersIconheight));
			ImGui::SameLine();
			ImGui::Text("  Index Answers");
			ImGui::Text("                                Have a question about how to use Index? check our answers site for");
			ImGui::Text("                                precise how-to knowledge.");
			ImGui::Text("");
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::Image(IndexForumsIcon, ImVec2(IndexForumsIconWidth, IndexForumsIconheight));
			ImGui::SameLine();
			ImGui::Text("  Index Forum");
			ImGui::Text("                                Meet the other Index users here - the friendliest people in the");
			ImGui::Text("                                industry.");
			ImGui::Text("");
			ImGui::Text("                ");
			ImGui::SameLine();
			ImGui::Image(IndexAssteStoreIcon, ImVec2(IndexAssteStoreIconWidth, IndexAssteStoreIconheight));
			ImGui::SameLine();
			ImGui::Text("  Index Asset Store");
			ImGui::Text("                                The Asset Store is the place to find art assets, game code and");
			ImGui::Text("                                extensions directly inside the Index editor. It's like having a complete");
			ImGui::Text("                                supermarket under your desk.");
			ImGui::Text("");
			ImGui::Text("                                                                                   ");
			ImGui::SameLine();

			if(ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();

		    bool ShowAtStartup = false;

			if(ImGui::Checkbox("Show at startup", &ShowAtStartup));

			ImGui::EndPopup();
		}
		ImGui::SetWindowSize("Welcome To INDEX", ImVec2(570, 465));
	}

	static const float identityMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

	void Editor::OnImGuizmo()
	{
		INDEX_PROFILE_FUNCTION();
		glm::mat4 view = glm::inverse(m_EditorCameraTransform.GetWorldMatrix());
		glm::mat4 proj = m_CurrentCamera->GetProjectionMatrix();

#ifdef USE_IMGUIZMO_GRID
		if(m_Settings.m_ShowGrid && !m_CurrentCamera->IsOrthographic())
			ImGuizmo::DrawGrid(glm::value_ptr(view),
				glm::value_ptr(proj),
				identityMatrix,
				120.f);
#endif

		if(!m_Settings.m_ShowGizmos || m_SelectedEntities.empty() || m_ImGuizmoOperation == 4)
			return;

		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();

		if(m_SelectedEntities.size() == 1)
		{
			entt::entity m_SelectedEntity = entt::null;

			m_SelectedEntity = m_SelectedEntities.front();
			if(registry.valid(m_SelectedEntity))
			{
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetOrthographic(m_CurrentCamera->IsOrthographic());

				auto transform = registry.try_get<Maths::Transform>(m_SelectedEntity);
				if(transform != nullptr)
				{
					glm::mat4 model = transform->GetWorldMatrix();

					float snapAmount[3] = {m_Settings.m_SnapAmount, m_Settings.m_SnapAmount, m_Settings.m_SnapAmount};
					float delta[16];

					ImGuizmo::Manipulate(glm::value_ptr(view),
						glm::value_ptr(proj),
						static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation),
						ImGuizmo::LOCAL,
						glm::value_ptr(model),
						delta,
						m_Settings.m_SnapQuizmo ? snapAmount : nullptr);

					if(ImGuizmo::IsUsing())
					{
						Entity parent = Entity(m_SelectedEntity, m_SceneManager->GetCurrentScene()).GetParent(); // m_CurrentScene->TryGetEntityWithUUID(entity.GetParentUUID());
						if(parent && parent.HasComponent<Maths::Transform>())
						{
							glm::mat4 parentTransform = parent.GetTransform().GetWorldMatrix();
							model = glm::inverse(parentTransform) * model;
						}

						if(ImGuizmo::IsScaleType()) // static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) & ImGuizmo::OPERATION::SCALE)
						{
							transform->SetLocalScale(Index::Maths::GetScale(model));
						}
						else
						{
							transform->SetLocalTransform(model);

							RigidBody2DComponent* rigidBody2DComponent = registry.try_get<Index::RigidBody2DComponent>(m_SelectedEntity);

							if(rigidBody2DComponent)
							{
								rigidBody2DComponent->GetRigidBody()->SetPosition(
									{model[3].x, model[3].y});
							}
							else
							{
								Index::RigidBody3DComponent* rigidBody3DComponent = registry.try_get<Index::RigidBody3DComponent>(m_SelectedEntity);
								if(rigidBody3DComponent)
								{
									rigidBody3DComponent->GetRigidBody()->SetPosition(model[3]);
									rigidBody3DComponent->GetRigidBody()->SetOrientation(Maths::GetRotation(model));
								}
							}
						}
					}
				}
			}
		}
		else
		{
			glm::vec3 medianPointLocation = glm::vec3(0.0f);
			glm::vec3 medianPointScale = glm::vec3(0.0f);
			int validcount = 0;
			for(auto entityID : m_SelectedEntities)
			{
				if(!registry.valid(entityID))
					continue;

				Entity entity = {entityID, m_SceneManager->GetCurrentScene()};

				if(!entity.HasComponent<Maths::Transform>())
					continue;

				medianPointLocation += entity.GetTransform().GetWorldPosition();
				medianPointScale += entity.GetTransform().GetLocalScale();
				validcount++;
			}
			medianPointLocation /= validcount; // m_SelectedEntities.size();
			medianPointScale /= validcount; // m_SelectedEntities.size();

			glm::mat4 medianPointMatrix = glm::translate(glm::mat4(1.0f), medianPointLocation) * glm::scale(glm::mat4(1.0f), medianPointScale);

			glm::mat4 projectionMatrix, viewMatrix;

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetOrthographic(m_CurrentCamera->IsOrthographic());

			float snapAmount[3] = {m_Settings.m_SnapAmount, m_Settings.m_SnapAmount, m_Settings.m_SnapAmount};
			glm::mat4 deltaMatrix = glm::mat4(1.0f);

			ImGuizmo::Manipulate(glm::value_ptr(view),
				glm::value_ptr(proj),
				static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation),
				ImGuizmo::LOCAL,
				glm::value_ptr(medianPointMatrix),
				glm::value_ptr(deltaMatrix),
				m_Settings.m_SnapQuizmo ? snapAmount : nullptr);

			if(ImGuizmo::IsUsing())
			{
				glm::vec3 deltaTranslation, deltaScale;
				glm::quat deltaRotation;
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::decompose(deltaMatrix, deltaScale, deltaRotation, deltaTranslation, skew, perspective);

				//                    if (parent && parent.HasComponent<Maths::Transform>())
				//                    {
				//                        glm::mat4 parentTransform = parent.GetTransform().GetWorldMatrix();
				//                        model = glm::inverse(parentTransform) * model;
				//                    }
				//

				static const bool MedianPointOrigin = false;

				if(MedianPointOrigin)
				{
					for(auto entityID : m_SelectedEntities)
					{
						if(!registry.valid(entityID))
							continue;
						auto transform = registry.try_get<Maths::Transform>(entityID);

						if(!transform)
							continue;
						if(ImGuizmo::IsScaleType()) // static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) == ImGuizmo::OPERATION::SCALE)
						{
							transform->SetLocalScale(transform->GetLocalScale() * deltaScale);
							// transform->SetLocalTransform(deltaMatrix * transform->GetLocalMatrix());
						}
						else
						{
							transform->SetLocalTransform(deltaMatrix * transform->GetLocalMatrix());

							// World matrix wont have updated yet so need to multiply by delta
							// TODO: refactor
							auto worldMatrix = deltaMatrix * transform->GetWorldMatrix();

							RigidBody2DComponent* rigidBody2DComponent = registry.try_get<Index::RigidBody2DComponent>(entityID);

							if(rigidBody2DComponent)
							{
								rigidBody2DComponent->GetRigidBody()->SetPosition({worldMatrix[3].x, worldMatrix[3].y});
							}
							else
							{
								Index::RigidBody3DComponent* rigidBody3DComponent = registry.try_get<Index::RigidBody3DComponent>(entityID);
								if(rigidBody3DComponent)
								{
									rigidBody3DComponent->GetRigidBody()->SetPosition(worldMatrix[3]);
									rigidBody3DComponent->GetRigidBody()->SetOrientation(Maths::GetRotation(worldMatrix));
								}
							}
						}
					}
				}
				else
				{
					for(auto entityID : m_SelectedEntities)
					{
						if(!registry.valid(entityID))
							continue;
						auto transform = registry.try_get<Maths::Transform>(entityID);

						if(!transform)
							continue;
						if(ImGuizmo::IsScaleType()) // static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) & ImGuizmo::OPERATION::SCALE)
						{
							transform->SetLocalScale(transform->GetLocalScale() * deltaScale);
							// transform->SetLocalTransform(deltaMatrix * transform->GetLocalMatrix());
						}
						else if(ImGuizmo::IsRotateType()) // static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) & ImGuizmo::OPERATION::ROTATE)
						{
							transform->SetLocalOrientation(glm::quat(glm::eulerAngles(transform->GetLocalOrientation()) + glm::eulerAngles(deltaRotation)));
						}
						else
						{
							transform->SetLocalTransform(deltaMatrix * transform->GetLocalMatrix());

							// World matrix wont have updated yet so need to multiply by delta
							// TODO: refactor
							auto worldMatrix = deltaMatrix * transform->GetWorldMatrix();

							RigidBody2DComponent* rigidBody2DComponent = registry.try_get<Index::RigidBody2DComponent>(entityID);

							if(rigidBody2DComponent)
							{
								rigidBody2DComponent->GetRigidBody()->SetPosition({worldMatrix[3].x, worldMatrix[3].y});
							}
							else
							{
								Index::RigidBody3DComponent* rigidBody3DComponent = registry.try_get<Index::RigidBody3DComponent>(entityID);
								if(rigidBody3DComponent)
								{
									rigidBody3DComponent->GetRigidBody()->SetPosition(worldMatrix[3]);
									rigidBody3DComponent->GetRigidBody()->SetOrientation(Maths::GetRotation(worldMatrix));
								}
							}
						}
					}
				}
			}
		}
	}


	void Editor::BeginDockSpace(bool gameFullScreen)
	{

		INDEX_PROFILE_FUNCTION();

		static bool p_open = true;
		static bool opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_NoCloseButton;
		bool opt_fullscreen = opt_fullscreen_persistant;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		if(opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();

			auto pos = viewport->Pos;
			auto size = viewport->Size;
			bool menuBar = true;
			if(menuBar)
			{
				const float infoBarSize = ImGui::GetFrameHeight();
				pos.y += infoBarSize;
				size.y -= infoBarSize;
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 2));

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
							| ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PopStyleVar();
		}

		// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the
		// pass-thru hole, so we ask Begin() to not render a background.
		if(opt_flags & ImGuiDockNodeFlags_DockSpace)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::Begin("MyDockspace", &p_open, window_flags);
		ImGui::Indent();
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.54f, 0.54f, 0.54f, 1.00f));

			bool selected;

			ImGui::SameLine((ImGui::GetWindowContentRegionMax().x * 0.46f) - (1.0f * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x)));

			if (Application::Get().GetEditorState() == EditorState::Next)
				Application::Get().SetEditorState(EditorState::Paused);

			// Play, Stop & Step

			{
				selected = Application::Get().GetEditorState() == EditorState::Play;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.60f, 0.98f, 1.00f));

				if (ImGui::Button(ICON_MDI_PLAY))
				{
					Application::Get().GetSystem<IndexPhysicsEngine>()->SetPaused(selected);
					Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(selected);

					Application::Get().GetSystem<AudioManager>()->UpdateListener(Application::Get().GetCurrentScene());
					Application::Get().GetSystem<AudioManager>()->SetPaused(selected);
					Application::Get().SetEditorState(selected ? EditorState::Preview : EditorState::Play);

					m_SelectedEntities.clear();
					/* m_SelectedEntity = entt::null;*/
					if (selected)
					{
						ImGui::SetWindowFocus("###scene");
						LoadCachedScene();
					}
					else
					{
						ImGui::SetWindowFocus("###game");
						CacheScene();
						Application::Get().GetCurrentScene()->OnInit();
					}
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Play");

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = Application::Get().GetEditorState() == EditorState::Paused;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.60f, 0.98f, 1.00f));

				if (ImGui::Button(ICON_MDI_PAUSE))
					Application::Get().SetEditorState(selected ? EditorState::Play : EditorState::Paused);

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Pause");

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();
			
			{
				selected = Application::Get().GetEditorState() == EditorState::Next;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.60f, 0.98f, 1.00f));

				if (ImGui::Button(ICON_MDI_STEP_FORWARD))
					Application::Get().SetEditorState(EditorState::Next);

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Step");

				if (selected)
					ImGui::PopStyleColor();
			}
			
			ImGui::SameLine((ImGui::GetWindowContentRegionMax().x * 0.93f) - (1.0f * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x)));

			// Layouts

			bool LayoutsPanels = false;

			if (ImGui::Button("Layouts " ICON_MDI_CHEVRON_DOWN))
			{
				LayoutsPanels = true;
			}

			if (LayoutsPanels)
				ImGui::OpenPopup("LayoutsPanelsOptions");

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));

			static const char* ini_to_load = NULL;
			if (ini_to_load)
			{
				ImGui::LoadIniSettingsFromDisk(ini_to_load);
				ini_to_load = NULL;
			}

			if (ImGui::BeginPopup("LayoutsPanelsOptions", ImGuiWindowFlags_Popup))
			{
				if (ImGui::MenuItem("2 by 3"))
				{
					ini_to_load = "Data/Resources/Layouts/2 by 3.ini";
				}

				if (ImGui::MenuItem("Default"))
				{
					ini_to_load = "Data/Resources/Layouts/Default.ini";
				}

				if (ImGui::MenuItem("Tall"))
				{
					ini_to_load = "Data/Resources/Layouts/Tall.ini";
				}

				if (ImGui::MenuItem("Wide"))
				{
					ini_to_load = "Data/Resources/Layouts/Wide.ini";
				}

				ImGui::EndPopup();
			}
			ImGui::PopStyleVar();

			ImGui::Unindent();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor();
		}

		ImGuiID DockspaceID = ImGui::GetID("MyDockspace");

		static std::vector<SharedPtr<EditorPanel>> hiddenPanels;
		if(m_Settings.m_FullScreenSceneView != gameFullScreen)
		{
			m_Settings.m_FullScreenSceneView = gameFullScreen;

			if(m_Settings.m_FullScreenSceneView)
			{
				for(auto panel : m_Panels)
				{
					if(panel->GetSimpleName() != "Game" && panel->Active())
					{
						panel->SetActive(false);
						hiddenPanels.push_back(panel);
					}
				}
			}
			else
			{
				for(auto panel : hiddenPanels)
				{
					panel->SetActive(true);
				}

				hiddenPanels.clear();
			}
		}

		if(!ImGui::DockBuilderGetNode(DockspaceID))
		{
			ImGui::DockBuilderRemoveNode(DockspaceID); // Clear out existing layout
			ImGui::DockBuilderAddNode(DockspaceID); // Add empty node
			ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetIO().DisplaySize * ImGui::GetIO().DisplayFramebufferScale);

			ImGuiID dock_main_id = DockspaceID;
			ImGuiID DockBottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);
			ImGuiID DockLeft = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
			ImGuiID DockRight = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);

			ImGuiID DockLeftChild = ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Down, 0.875f, nullptr, &DockLeft);
			ImGuiID DockRightChild = ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Down, 0.875f, nullptr, &DockRight);
			ImGuiID DockingLeftDownChild = ImGui::DockBuilderSplitNode(DockLeftChild, ImGuiDir_Down, 0.06f, nullptr, &DockLeftChild);
			ImGuiID DockingRightDownChild = ImGui::DockBuilderSplitNode(DockRightChild, ImGuiDir_Down, 0.06f, nullptr, &DockRightChild);

			ImGuiID DockBottomChild = ImGui::DockBuilderSplitNode(DockBottom, ImGuiDir_Down, 0.2f, nullptr, &DockBottom);
			ImGuiID DockingBottomLeftChild = ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Down, 0.4f, nullptr, &DockLeft);
			ImGuiID DockingBottomRightChild = ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Down, 0.4f, nullptr, &DockRight);

			ImGuiID DockMiddle = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.8f, nullptr, &dock_main_id);
			ImGuiID DockBottomMiddle = ImGui::DockBuilderSplitNode(DockMiddle, ImGuiDir_Down, 0.3f, nullptr, &DockMiddle);
			ImGuiID DockMiddleLeft = ImGui::DockBuilderSplitNode(DockMiddle, ImGuiDir_Left, 0.5f, nullptr, &DockMiddle);
			ImGuiID DockMiddleRight = ImGui::DockBuilderSplitNode(DockMiddle, ImGuiDir_Right, 0.5f, nullptr, &DockMiddle);

			ImGui::DockBuilderDockWindow("###game", DockMiddleRight);
			ImGui::DockBuilderDockWindow("###scene", DockMiddleLeft);
			ImGui::DockBuilderDockWindow("###inspector", DockRight);
			ImGui::DockBuilderDockWindow("###console", DockBottomMiddle);;
			ImGui::DockBuilderDockWindow("###profiler", DockingBottomLeftChild);
			ImGui::DockBuilderDockWindow("###resources", DockingBottomLeftChild);
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", DockLeft);
			ImGui::DockBuilderDockWindow("GraphicsInfo", DockLeft);
			ImGui::DockBuilderDockWindow("ApplicationInfo", DockLeft);
			ImGui::DockBuilderDockWindow("###hierarchy", DockLeft);
			ImGui::DockBuilderDockWindow("###textEdit", DockMiddleLeft);
			ImGui::DockBuilderDockWindow("###scenesettings", DockLeft);
			ImGui::DockBuilderDockWindow("###editorsettings", DockLeft);
			ImGui::DockBuilderDockWindow("###projectsettings", DockLeft);

			ImGui::DockBuilderFinish(DockspaceID);
		}


		// Dockspace
		ImGuiIO& io = ImGui::GetIO();
		if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGui::DockSpace(DockspaceID, ImVec2(0.0f, 0.0f), opt_flags);
		}
	}

	void Editor::EndDockSpace()
	{
		ImGui::End();
	}

	void Editor::OnNewScene(Scene* scene)
	{
		INDEX_PROFILE_FUNCTION();
		Application::OnNewScene(scene);
		// m_SelectedEntity = entt::null;
		m_SelectedEntities.clear();
		glm::mat4 viewMat = glm::inverse(glm::lookAt(glm::vec3(-31.0f, 12.0f, 51.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		m_EditorCameraTransform.SetLocalTransform(viewMat);

		for(auto panel : m_Panels)
		{
			panel->OnNewScene(scene);
		}

		//std::string Configuration;
		std::string Platform;
		std::string RenderAPI;
		std::string dash = " - ";

#ifdef INDEX_PLATFORM_WINDOWS
		Platform = "Windows Standalone";
#elif INDEX_PLATFORM_LINUX
		Platform = "Linux Standalone";
#elif INDEX_PLATFORM_MACOS
		Platform = "MacOS Standalone";
#elif INDEX_PLATFORM_IOS
		Platform = "iOS Standalone";
#endif

		switch(Graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef INDEX_RENDER_API_OPENGL
		case Graphics::RenderAPI::OPENGL:
			RenderAPI = "<OpenGL>";
			break;
#endif

#ifdef INDEX_RENDER_API_VULKAN
#	if defined(INDEX_PLATFORM_MACOS) || defined(INDEX_PLATFORM_IOS)
		case Graphics::RenderAPI::VULKAN:
			RenderAPI = "<Vulkan ( MoltenVK )>";
			break;
#	else
		case Graphics::RenderAPI::VULKAN:
			RenderAPI = "<Vulkan>";
			break;
#	endif
#endif

#ifdef INDEX_RENDER_API_DIRECT3D
		case DIRECT3D:
			RenderAPI = "<DX3D>";
			break;
#endif
		default:
			break;
		}

		std::stringstream Title;
		Title << "Index 1.0.0f1 Personal (64bit)" << dash << scene->GetSceneName() << dash << Application::Get().GetWindow()->GetTitle() << dash << Platform << " " << RenderAPI;

		Application::Get().GetWindow()->SetWindowTitle(Title.str());
	}

	void Editor::Draw3DGrid()
	{
		INDEX_PROFILE_FUNCTION();
#if 1
		if(!m_GridRenderer || !Application::Get().GetSceneManager()->GetCurrentScene())
		{
			return;
		}

		DebugRenderer::DrawHairLine(glm::vec3(-5000.0f, 0.0f, 0.0f), glm::vec3(5000.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		DebugRenderer::DrawHairLine(glm::vec3(0.0f, -5000.0f, 0.0f), glm::vec3(0.0f, 5000.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		DebugRenderer::DrawHairLine(glm::vec3(0.0f, 0.0f, -5000.0f), glm::vec3(0.0f, 0.0f, 5000.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

		m_GridRenderer->OnImGui();

		m_GridRenderer->BeginScene(Application::Get().GetSceneManager()->GetCurrentScene(), m_EditorCamera.get(), &m_EditorCameraTransform);
		m_GridRenderer->RenderScene();
#endif
	}

	void Editor::Draw2DGrid(ImDrawList* drawList,
		const ImVec2& cameraPos,
		const ImVec2& windowPos,
		const ImVec2& canvasSize,
		const float factor,
		const float thickness)
	{
		INDEX_PROFILE_FUNCTION();
		static const auto graduation = 10;
		float GRID_SZ = canvasSize.y * 0.5f / factor;
		const ImVec2& offset = {
			canvasSize.x * 0.5f - cameraPos.x * GRID_SZ, canvasSize.y * 0.5f + cameraPos.y * GRID_SZ};

		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		float gridThickness = 1.0f;

		const auto& gridColor = GRID_COLOR;
		auto smallGraduation = GRID_SZ / graduation;
		const auto& smallGridColor = IM_COL32(100, 100, 100, smallGraduation);

		for(float x = -GRID_SZ; x < canvasSize.x + GRID_SZ; x += GRID_SZ)
		{
			auto localX = floorf(x + fmodf(offset.x, GRID_SZ));
			drawList->AddLine(
				ImVec2{localX, 0.0f} + windowPos, ImVec2{localX, canvasSize.y} + windowPos, gridColor, gridThickness);

			if(smallGraduation > 5.0f)
			{
				for(int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localX + smallGraduation * i);
					drawList->AddLine(ImVec2{graduation, 0.0f} + windowPos,
						ImVec2{graduation, canvasSize.y} + windowPos,
						smallGridColor,
						1.0f);
				}
			}
		}

		for(float y = -GRID_SZ; y < canvasSize.y + GRID_SZ; y += GRID_SZ)
		{
			auto localY = floorf(y + fmodf(offset.y, GRID_SZ));
			drawList->AddLine(
				ImVec2{0.0f, localY} + windowPos, ImVec2{canvasSize.x, localY} + windowPos, gridColor, gridThickness);

			if(smallGraduation > 5.0f)
			{
				for(int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localY + smallGraduation * i);
					drawList->AddLine(ImVec2{0.0f, graduation} + windowPos,
						ImVec2{canvasSize.x, graduation} + windowPos,
						smallGridColor,
						1.0f);
				}
			}
		}
	}

	bool Editor::OnFileDrop(WindowFileEvent& e)
	{
		FileOpenCallback(e.GetFilePath());
		return true;
	}

	void Editor::OnEvent(Event& e)
	{
		INDEX_PROFILE_FUNCTION();
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowFileEvent>(BIND_EVENT_FN(Editor::OnFileDrop));
		// Block events here

		Application::OnEvent(e);
	}

	Maths::Ray Editor::GetScreenRay(int x, int y, Camera* camera, int width, int height)
	{
		INDEX_PROFILE_FUNCTION();
		if(!camera)
			return Maths::Ray();

		float screenX = (float)x / (float)width;
		float screenY = (float)y / (float)height;

		bool flipY = true;

#ifdef INDEX_RENDER_API_OPENGL
		if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::OPENGL)
			flipY = true;
#endif
		return camera->GetScreenRay(screenX, screenY, glm::inverse(m_EditorCameraTransform.GetWorldMatrix()), flipY);
	}

	void Editor::OnUpdate(const TimeStep& ts)
	{
		INDEX_PROFILE_FUNCTION();

		static float autoSaveTimer = 0.0f;
		if(m_AutoSaveSettingsTime > 0)
		{
			if(autoSaveTimer > m_AutoSaveSettingsTime)
			{
				SaveEditorSettings();
				autoSaveTimer = 0;
			}

			autoSaveTimer += ts.GetMillis();
		}

		if(m_EditorState == EditorState::Play)
			autoSaveTimer = 0.0f;

		if(m_SceneViewActive)
		{
			auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();

			// if(Application::Get().GetSceneActive())
			{
				const glm::vec2 mousePos = Input::Get().GetMousePosition();
				m_EditorCameraController.SetCamera(m_EditorCamera);
				m_EditorCameraController.HandleMouse(m_EditorCameraTransform, (float)ts.GetSeconds(), mousePos.x, mousePos.y);
				m_EditorCameraController.HandleKeyboard(m_EditorCameraTransform, (float)ts.GetSeconds());
				m_EditorCameraTransform.SetWorldMatrix(glm::mat4(1.0f));

				if(Input::Get().GetKeyPressed(InputCode::Key::F))
				{
					if(registry.valid(m_SelectedEntities.front()))
					{
						auto transform = registry.try_get<Maths::Transform>(m_SelectedEntities.front());
						if(transform)
							FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
					}
				}
			}

			if(Input::Get().GetKeyHeld(InputCode::Key::O))
			{
				FocusCamera(glm::vec3(0.0f, 0.0f, 0.0f), 2.0f, 2.0f);
			}

			if(m_TransitioningCamera)
			{
				if(m_CameraTransitionStartTime < 0.0f)
					m_CameraTransitionStartTime = (float)ts.GetElapsedSeconds();

				float focusProgress = Maths::Min(((float)ts.GetElapsedSeconds() - m_CameraTransitionStartTime) / m_CameraTransitionSpeed, 1.f);
				glm::vec3 newCameraPosition = glm::mix(m_CameraStartPosition, m_CameraDestination, focusProgress);
				m_EditorCameraTransform.SetLocalPosition(newCameraPosition);

				if(m_EditorCameraTransform.GetLocalPosition() == m_CameraDestination)
					m_TransitioningCamera = false;
			}

			if(!Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight) && !ImGuizmo::IsUsing())
			{
				if(Input::Get().GetKeyPressed(InputCode::Key::W))
				{
					SetImGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::E))
				{
					SetImGuizmoOperation(ImGuizmo::OPERATION::ROTATE);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::R))
				{
					SetImGuizmoOperation(ImGuizmo::OPERATION::SCALE);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::T))
				{
					SetImGuizmoOperation(ImGuizmo::OPERATION::BOUNDS);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::Y))
				{
					SetImGuizmoOperation(ImGuizmo::OPERATION::UNIVERSAL);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::Q))
				{
					ToggleSnap();
				}
			}

			if((Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || (Input::Get().GetKeyHeld(InputCode::Key::LeftControl))))
			{
				if(Input::Get().GetKeyPressed(InputCode::Key::S) && Application::Get().GetSceneActive())
				{
					Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(m_ProjectSettings.m_ProjectRoot + "Assets/scenes/", false);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::O))
					Application::Get().GetSceneManager()->GetCurrentScene()->Deserialise(m_ProjectSettings.m_ProjectRoot + "Assets/scenes/", false);

				if(Input::Get().GetKeyPressed(InputCode::Key::X))
				{
					for(auto entity : m_SelectedEntities)
						SetCopiedEntity(entity, true);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::C))
				{
					for(auto entity : m_SelectedEntities)
						SetCopiedEntity(entity, false);
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::V) && !m_CopiedEntities.empty())
				{
					for(auto entity : m_CopiedEntities)
					{
						Application::Get().GetCurrentScene()->DuplicateEntity({entity, Application::Get().GetCurrentScene()});
						if(entity != entt::null)
						{
							// if(m_CopiedEntity == m_SelectedEntity)
							//   m_SelectedEntity = entt::null;
							Entity(entity, Application::Get().GetCurrentScene()).Destroy();
						}
					}
				}

				if(Input::Get().GetKeyPressed(InputCode::Key::D) && !m_SelectedEntities.empty())
				{
					for(auto entity : m_CopiedEntities)
						Application::Get().GetCurrentScene()->DuplicateEntity({entity, Application::Get().GetCurrentScene()});
				}
			}
		}
		else
			m_EditorCameraController.StopMovement();

		Application::OnUpdate(ts);
	}

	void Editor::FocusCamera(const glm::vec3& point, float distance, float speed)
	{
		INDEX_PROFILE_FUNCTION();
		if(m_CurrentCamera->IsOrthographic())
		{
			m_EditorCameraTransform.SetLocalPosition(point);
			// m_CurrentCamera->SetScale(distance * 0.5f);
		}
		else
		{
			m_TransitioningCamera = true;

			m_CameraDestination = point + m_EditorCameraTransform.GetForwardDirection() * distance;
			m_CameraTransitionStartTime = -1.0f;
			m_CameraTransitionSpeed = 1.0f / speed;
			m_CameraStartPosition = m_EditorCameraTransform.GetLocalPosition();
		}
	}

	bool Editor::OnWindowResize(WindowResizeEvent& e)
	{
		return false;
	}

	void Editor::RecompileShaders()
	{
		INDEX_PROFILE_FUNCTION();
		INDEX_LOG_INFO("Recompiling shaders Disabled");

#ifdef INDEX_RENDER_API_VULKAN
#	ifdef INDEX_PLATFORM_WINDOWS
		// std::string filePath = ROOT_DIR"/Index/Assets/EngineShaders/CompileShadersWindows.bat";
		// system(filePath.c_str());
#	elif INDEX_PLATFORM_MACOS
		// std::string filePath = ROOT_DIR "/Index/Assets/EngineShaders/CompileShadersMac.sh";
		// system(filePath.c_str());
#	endif
#endif
	}

	void Editor::DebugDraw()
	{
		INDEX_PROFILE_FUNCTION();
		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
		glm::vec4 selectedColour = glm::vec4(0.9f);
		if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::MeshBoundingBoxes)
		{
			auto group = registry.group<Graphics::ModelComponent>(entt::get<Maths::Transform>);

			for(auto entity : group)
			{
				const auto& [model, trans] = group.get<Graphics::ModelComponent, Maths::Transform>(entity);
				auto& meshes = model.ModelRef->GetMeshes();
				for(auto mesh : meshes)
				{
					if(mesh->GetActive())
					{
						auto& worldTransform = trans.GetWorldMatrix();
						auto bbCopy = mesh->GetBoundingBox()->Transformed(worldTransform);
						DebugRenderer::DebugDraw(bbCopy, selectedColour, true);
					}
				}
			}
		}

		if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::SpriteBoxes)
		{
			auto group = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

			for(auto entity : group)
			{
				const auto& [sprite, trans] = group.get<Graphics::Sprite, Maths::Transform>(entity);

				{
					auto& worldTransform = trans.GetWorldMatrix();

					auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
					bb.Transform(trans.GetWorldMatrix());
					DebugRenderer::DebugDraw(bb, selectedColour, true);
				}
			}

			auto animGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);

			for(auto entity : animGroup)
			{
				const auto& [sprite, trans] = animGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

				{
					auto& worldTransform = trans.GetWorldMatrix();

					auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
					bb.Transform(trans.GetWorldMatrix());
					DebugRenderer::DebugDraw(bb, selectedColour, true);
				}
			}
		}

		if(m_Settings.m_DebugDrawFlags & EditorDebugFlags::CameraFrustum)
		{
			auto cameraGroup = registry.group<Camera>(entt::get<Maths::Transform>);

			for(auto entity : cameraGroup)
			{
				const auto& [camera, trans] = cameraGroup.get<Camera, Maths::Transform>(entity);

				{
					DebugRenderer::DebugDraw(camera.GetFrustum(glm::inverse(trans.GetWorldMatrix())), glm::vec4(0.9f));
				}
			}
		}

		for(auto m_SelectedEntity : m_SelectedEntities)
			if(registry.valid(m_SelectedEntity)) // && Application::Get().GetEditorState() == EditorState::Preview)
			{
				auto transform = registry.try_get<Maths::Transform>(m_SelectedEntity);

				auto model = registry.try_get<Graphics::ModelComponent>(m_SelectedEntity);
				if(transform && model && model->ModelRef)
				{
					auto& meshes = model->ModelRef->GetMeshes();
					for(auto mesh : meshes)
					{
						if(mesh->GetActive())
						{
							auto& worldTransform = transform->GetWorldMatrix();
							auto bbCopy = mesh->GetBoundingBox()->Transformed(worldTransform);
							DebugRenderer::DebugDraw(bbCopy, selectedColour, true);
						}
					}
					if(model->ModelRef->GetSkeleton())
					{
						auto& skeleton = *model->ModelRef->GetSkeleton().get();
						const int num_joints = skeleton.num_joints();

						// Iterate through each joint in the skeleton
						for(int i = 0; i < num_joints; ++i)
						{
							// Get the current joint's world space transform

							const ozz::math::Transform joint_transform; //= skeleton.joint_rest_poses()[i];

							// Convert ozz::math::Transform to glm::mat4
							glm::mat4 joint_world_transform = glm::translate(glm::mat4(1.0f), glm::vec3(joint_transform.translation.x, joint_transform.translation.y, joint_transform.translation.z));
							joint_world_transform *= glm::mat4_cast(glm::quat(joint_transform.rotation.x, joint_transform.rotation.y, joint_transform.rotation.z, joint_transform.rotation.w));
							joint_world_transform = glm::scale(joint_world_transform, glm::vec3(joint_transform.scale.x, joint_transform.scale.y, joint_transform.scale.z));

							// Multiply the joint's world transform by the entity transform
							glm::mat4 final_transform = transform->GetWorldMatrix() * joint_world_transform;

							// Get the parent joint's world space transform
							const int parent_index = skeleton.joint_parents()[i];
							glm::mat4 parent_world_transform(1.0f);
							if(parent_index != ozz::animation::Skeleton::Constants::kNoParent)
							{
								const ozz::math::Transform parent_transform; // = skeleton.joint_rest_poses()[parent_index];
								parent_world_transform = glm::translate(glm::mat4(1.0f), glm::vec3(parent_transform.translation.x, parent_transform.translation.y, parent_transform.translation.z));
								parent_world_transform *= glm::mat4_cast(glm::quat(parent_transform.rotation.x, parent_transform.rotation.y, parent_transform.rotation.z, parent_transform.rotation.w));
								parent_world_transform = glm::scale(parent_world_transform, glm::vec3(parent_transform.scale.x, parent_transform.scale.y, parent_transform.scale.z));
								parent_world_transform = transform->GetWorldMatrix() * parent_world_transform;
							}

							// Draw a line between the current joint and its parent joint
							// (assuming you have a function to draw a line between two points)
							DebugRenderer::DrawHairLine(glm::vec3(final_transform[3]), glm::vec3(parent_world_transform[3]), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Example color: red
						}
					}
				}

				auto sprite = registry.try_get<Graphics::Sprite>(m_SelectedEntity);
				if(transform && sprite)
				{
					{
						auto& worldTransform = transform->GetWorldMatrix();

						auto bb = Maths::BoundingBox(Maths::Rect(sprite->GetPosition(), sprite->GetPosition() + sprite->GetScale()));
						bb.Transform(worldTransform);
						DebugRenderer::DebugDraw(bb, selectedColour, true);
					}
				}

				auto animSprite = registry.try_get<Graphics::AnimatedSprite>(m_SelectedEntity);
				if(transform && animSprite)
				{
					{
						auto& worldTransform = transform->GetWorldMatrix();

						auto bb = Maths::BoundingBox(Maths::Rect(animSprite->GetPosition(), animSprite->GetPosition() + animSprite->GetScale()));
						bb.Transform(worldTransform);
						DebugRenderer::DebugDraw(bb, selectedColour, true);
					}
				}

				auto camera = registry.try_get<Camera>(m_SelectedEntity);
				if(camera && transform)
				{
					DebugRenderer::DebugDraw(camera->GetFrustum(glm::inverse(transform->GetWorldMatrix())), glm::vec4(0.9f));
				}

				auto light = registry.try_get<Graphics::Light>(m_SelectedEntity);
				if(light && transform)
				{
					DebugRenderer::DebugDraw(light, transform->GetWorldOrientation(), glm::vec4(glm::vec3(light->Colour), 0.2f));
				}

				auto sound = registry.try_get<SoundComponent>(m_SelectedEntity);
				if(sound)
				{
					DebugRenderer::DebugDraw(sound->GetSoundNode(), glm::vec4(0.8f, 0.8f, 0.8f, 0.2f));
				}

				auto phys3D = registry.try_get<RigidBody3DComponent>(m_SelectedEntity);
				if(phys3D)
				{
					auto cs = phys3D->GetRigidBody()->GetCollisionShape();
					if(cs)
						cs->DebugDraw(phys3D->GetRigidBody().get());
				}
			}
	}

	void Editor::SelectObject(const Maths::Ray& ray)
	{
		INDEX_PROFILE_FUNCTION();
		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
		float closestEntityDist = Maths::M_INFINITY;
		entt::entity currentClosestEntity = entt::null;

		auto group = registry.group<Graphics::ModelComponent>(entt::get<Maths::Transform>);

		static Timer timer;
		static float timeSinceLastSelect = 0.0f;

		for(auto entity : group)
		{
			const auto& [model, trans] = group.get<Graphics::ModelComponent, Maths::Transform>(entity);

			auto& meshes = model.ModelRef->GetMeshes();

			for(auto mesh : meshes)
			{
				if(mesh->GetActive())
				{
					auto& worldTransform = trans.GetWorldMatrix();

					auto bbCopy = mesh->GetBoundingBox()->Transformed(worldTransform);
					float distance;
					ray.Intersects(bbCopy, distance);

					if(distance < Maths::M_INFINITY)
					{
						if(distance < closestEntityDist)
						{
							closestEntityDist = distance;
							currentClosestEntity = entity;
						}
					}
				}
			}
		}

		if(!m_SelectedEntities.empty())
		{
			if(registry.valid(currentClosestEntity) && IsSelected(currentClosestEntity))
			{
				if(timer.GetElapsedS() - timeSinceLastSelect < 1.0f)
				{
					auto& trans = registry.get<Maths::Transform>(currentClosestEntity);
					auto& model = registry.get<Graphics::ModelComponent>(currentClosestEntity);
					auto bb = model.ModelRef->GetMeshes().front()->GetBoundingBox()->Transformed(trans.GetWorldMatrix());

					FocusCamera(trans.GetWorldPosition(), glm::distance(bb.Max(), bb.Min()));
				}
				else
				{
					UnSelect(currentClosestEntity);
				}
			}

			timeSinceLastSelect = timer.GetElapsedS();

			auto& io = ImGui::GetIO();
			auto ctrl = Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || (Input::Get().GetKeyHeld(InputCode::Key::LeftControl));

			if(!ctrl)
				m_SelectedEntities.clear();

			SetSelected(currentClosestEntity);
			return;
		}

		auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

		for(auto entity : spriteGroup)
		{
			const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

			auto& worldTransform = trans.GetWorldMatrix();
			auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
			bb.Transform(trans.GetWorldMatrix());

			float distance;
			ray.Intersects(bb, distance);
			if(distance < Maths::M_INFINITY)
			{
				if(distance < closestEntityDist)
				{
					closestEntityDist = distance;
					currentClosestEntity = entity;
				}
			}
		}

		auto animSpriteGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);

		for(auto entity : animSpriteGroup)
		{
			const auto& [sprite, trans] = animSpriteGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

			auto& worldTransform = trans.GetWorldMatrix();
			auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
			bb.Transform(trans.GetWorldMatrix());
			float distance;
			ray.Intersects(bb, distance);
			if(distance < Maths::M_INFINITY)
			{
				if(distance < closestEntityDist)
				{
					closestEntityDist = distance;
					currentClosestEntity = entity;
				}
			}
		}

		{
			if(IsSelected(currentClosestEntity))
			{
				auto& trans = registry.get<Maths::Transform>(currentClosestEntity);
				auto& sprite = registry.get<Graphics::Sprite>(currentClosestEntity);
				auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));

				FocusCamera(trans.GetWorldPosition(), glm::distance(bb.Max(), bb.Min()));
			}
		}

		SetSelected(currentClosestEntity);
	}

	void Editor::OpenTextFile(const std::string& filePath, const std::function<void()>& callback)
	{
		INDEX_PROFILE_FUNCTION();
		std::string physicalPath;
		if(!VFS::Get().ResolvePhysicalPath(filePath, physicalPath))
		{
			INDEX_LOG_ERROR("Failed to Load Lua script {0}", filePath);
			return;
		}

		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			EditorPanel* w = m_Panels[i].get();
			if(w->GetSimpleName() == "TextEdit")
			{
				m_Panels.erase(m_Panels.begin() + i);
				break;
			}
		}

		m_Panels.emplace_back(CreateSharedPtr<TextEditPanel>(physicalPath));
		m_Panels.back().As<TextEditPanel>()->SetOnSaveCallback(callback);
		m_Panels.back()->SetEditor(this);
	}

	EditorPanel* Editor::GetTextEditPanel()
	{
		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			EditorPanel* w = m_Panels[i].get();
			if(w->GetSimpleName() == "TextEdit")
			{
				return w;
			}
		}

		return nullptr;
	}

	void Editor::RemovePanel(EditorPanel* panel)
	{
		INDEX_PROFILE_FUNCTION();
		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			EditorPanel* w = m_Panels[i].get();
			if(w == panel)
			{
				m_Panels.erase(m_Panels.begin() + i);
				return;
			}
		}
	}

	void Editor::ShowPreview()
	{
		INDEX_PROFILE_FUNCTION();
		ImGui::Begin("Preview");
		if(m_PreviewTexture)
			ImGuiUtilities::Image(m_PreviewTexture.get(), {200, 200});
		ImGui::End();
	}

	void Editor::OnDebugDraw()
	{
		Application::OnDebugDraw();
		DebugDraw();

		// Application::Get().GetEditorState() == EditorState::Preview &&
	}

	void Editor::OnRender()
	{
		INDEX_PROFILE_FUNCTION();
		// DrawPreview();

		bool isProfiling = false;
		static bool firstFrame = true;
#if INDEX_PROFILE
		isProfiling = tracy::GetProfiler().IsConnected();
#endif
		if(!isProfiling && m_Settings.m_SleepOutofFocus && !Application::Get().GetWindow()->GetWindowFocus() && m_EditorState != EditorState::Play && !firstFrame)
			OS::Instance()->Delay(1000000);

		Application::OnRender();

		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			m_Panels[i]->OnRender();
		}

		if(m_Settings.m_ShowGrid && !m_EditorCamera->IsOrthographic())
			Draw3DGrid();

		firstFrame = false;
	}

	void Editor::DrawPreview()
	{
		INDEX_PROFILE_FUNCTION();
		if(!m_PreviewTexture)
		{
			Graphics::TextureDesc desc;
			desc.format = Graphics::RHIFormat::R8G8B8A8_Unorm;
			desc.flags = Graphics::TextureFlags::Texture_RenderTarget;

			m_PreviewTexture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create(desc, 200, 200));

			// m_PreviewRenderer = CreateSharedPtr<Graphics::ForwardRenderer>(200, 200, false);
			m_PreviewSphere = SharedPtr<Graphics::Mesh>(Graphics::CreateSphere());

			// m_PreviewRenderer->SetRenderTarget(m_PreviewTexture.get(), true);
		}

		glm::mat4 proj = glm::perspective(0.1f, 10.0f, 200.0f / 200.0f, 60.0f);
		glm::mat4 view = glm::inverse(Maths::Mat4FromTRS(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)));

		//        m_PreviewRenderer->Begin();
		//        //m_PreviewRenderer->BeginScene(proj, view);
		//        m_PreviewRenderer->SubmitMesh(m_PreviewSphere.get(), nullptr, glm::mat4(1.0f), glm::mat4(1.0f));
		//        m_PreviewRenderer->Present();
		//        m_PreviewRenderer->End();
	}

	void Editor::FileOpenCallback(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();

		auto path = filePath;
		path = StringUtilities::BackSlashesToSlashes(path);
		if(IsTextFile(path))
			OpenTextFile(path, NULL);
		else if(IsModelFile(path))
		{
			Entity modelEntity = Application::Get().GetSceneManager()->GetCurrentScene()->GetEntityManager()->Create();
			modelEntity.AddComponent<Graphics::ModelComponent>(path);

			SetSelected(modelEntity.GetHandle());
			// m_SelectedEntity = modelEntity.GetHandle();
		}
		else if(IsAudioFile(path))
		{
			std::string physicalPath;
			Index::VFS::Get().ResolvePhysicalPath(path, physicalPath);
			auto sound = Sound::Create(physicalPath, StringUtilities::GetFilePathExtension(path));

			auto soundNode = SharedPtr<SoundNode>(SoundNode::Create());
			soundNode->SetSound(sound);
			soundNode->SetVolume(1.0f);
			soundNode->SetPosition(glm::vec3(0.1f, 10.0f, 10.0f));
			soundNode->SetLooping(true);
			soundNode->SetIsGlobal(false);
			soundNode->SetPaused(false);
			soundNode->SetReferenceDistance(1.0f);
			soundNode->SetRadius(30.0f);

			Entity entity = Application::Get().GetSceneManager()->GetCurrentScene()->GetEntityManager()->Create();
			entity.AddComponent<SoundComponent>(soundNode);
			entity.GetOrAddComponent<Maths::Transform>();
			SetSelected(entity.GetHandle());
		}
		else if(IsSceneFile(path))
		{
			int index = Application::Get().GetSceneManager()->EnqueueSceneFromFile(path);
			Application::Get().GetSceneManager()->SwitchScene(index);
		}
		else if(IsTextureFile(path))
		{
			auto entity = Application::Get().GetSceneManager()->GetCurrentScene()->CreateEntity("Sprite");
			auto& sprite = entity.AddComponent<Graphics::Sprite>();
			entity.GetOrAddComponent<Maths::Transform>();

			SharedPtr<Graphics::Texture2D> texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(path, path));
			sprite.SetTexture(texture);
		}
	}

	void Editor::FileEmbedCallback(const std::string& filePath)
	{
		if(IsTextureFile(filePath))
		{
			std::string fileName = StringUtilities::RemoveFilePathExtension(StringUtilities::GetFileName(filePath));
			std::string outPath = StringUtilities::GetFileLocation(filePath) + fileName + ".inl";

			INDEX_LOG_INFO("Embed texture from {0} to {1}", filePath, outPath);
			EmbedTexture(filePath, outPath, fileName);
		}
		else if(IsShaderFile(filePath))
		{
			EmbedShader(filePath);
		}
	}

	void Editor::ProjectOpenCallback(const std::string& filePath)
	{
		m_NewProjectPopupOpen = false;
		reopenNewProjectPopup = false;
		locationPopupOpened = false;
		m_FileBrowserPanel.ClearFileTypeFilters();

		if(FileSystem::FileExists(filePath))
		{
			auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), filePath);
			if(it == m_Settings.m_RecentProjects.end())
				m_Settings.m_RecentProjects.push_back(filePath);
		}

		Application::Get().OpenProject(filePath);

		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			m_Panels[i]->OnNewProject();
		}
	}

	void Editor::NewProjectOpenCallback(const std::string& filePath)
	{
		Application::Get().OpenNewProject(filePath);
		m_FileBrowserPanel.SetOpenDirectory(false);

		for(int i = 0; i < int(m_Panels.size()); i++)
		{
			m_Panels[i]->OnNewProject();
		}
	}

	void Editor::SaveEditorSettings()
	{
		INDEX_PROFILE_FUNCTION();
		m_IniFile.RemoveAll();
		m_IniFile.SetOrAdd("ShowGrid", m_Settings.m_ShowGrid);
		m_IniFile.SetOrAdd("ShowGizmos", m_Settings.m_ShowGizmos);
		m_IniFile.SetOrAdd("ShowViewSelected", m_Settings.m_ShowViewSelected);
		m_IniFile.SetOrAdd("ShowImGuiDemo", m_Settings.m_ShowImGuiDemo);
		m_IniFile.SetOrAdd("SnapAmount", m_Settings.m_SnapAmount);
		m_IniFile.SetOrAdd("SnapQuizmo", m_Settings.m_SnapQuizmo);
		m_IniFile.SetOrAdd("DebugDrawFlags", m_Settings.m_DebugDrawFlags);
		m_IniFile.SetOrAdd("PhysicsDebugDrawFlags", Application::Get().GetSystem<IndexPhysicsEngine>()->GetDebugDrawFlags());
		m_IniFile.SetOrAdd("PhysicsDebugDrawFlags2D", Application::Get().GetSystem<B2PhysicsEngine>()->GetDebugDrawFlags());
		m_IniFile.SetOrAdd("Theme", (int)m_Settings.m_Theme);
		m_IniFile.SetOrAdd("ProjectRoot", m_ProjectSettings.m_ProjectRoot);
		m_IniFile.SetOrAdd("ProjectName", m_ProjectSettings.m_ProjectName);
		m_IniFile.SetOrAdd("SleepOutofFocus", m_Settings.m_SleepOutofFocus);
		m_IniFile.SetOrAdd("CameraSpeed", m_Settings.m_CameraSpeed);
		m_IniFile.SetOrAdd("CameraNear", m_Settings.m_CameraNear);
		m_IniFile.SetOrAdd("CameraFar", m_Settings.m_CameraFar);

		std::sort(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end());
		m_Settings.m_RecentProjects.erase(std::unique(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end()), m_Settings.m_RecentProjects.end());
		m_IniFile.SetOrAdd("RecentProjectCount", int(m_Settings.m_RecentProjects.size()));

		for(int i = 0; i < int(m_Settings.m_RecentProjects.size()); i++)
		{
			m_IniFile.SetOrAdd("RecentProject" + std::to_string(i), m_Settings.m_RecentProjects[i]);
		}

		m_IniFile.Rewrite();
	}

	void Editor::AddDefaultEditorSettings()
	{
		INDEX_PROFILE_FUNCTION();
		m_ProjectSettings.m_ProjectRoot = "../../ExampleProject/";
		m_ProjectSettings.m_ProjectName = "Example";

		m_IniFile.Add("ShowGrid", m_Settings.m_ShowGrid);
		m_IniFile.Add("ShowGizmos", m_Settings.m_ShowGizmos);
		m_IniFile.Add("ShowViewSelected", m_Settings.m_ShowViewSelected);
		m_IniFile.Add("TransitioningCamera", m_TransitioningCamera);
		m_IniFile.Add("ShowImGuiDemo", m_Settings.m_ShowImGuiDemo);
		m_IniFile.Add("SnapAmount", m_Settings.m_SnapAmount);
		m_IniFile.Add("SnapQuizmo", m_Settings.m_SnapQuizmo);
		m_IniFile.Add("DebugDrawFlags", m_Settings.m_DebugDrawFlags);
		m_IniFile.Add("PhysicsDebugDrawFlags", 0);
		m_IniFile.Add("PhysicsDebugDrawFlags2D", 0);
		m_IniFile.Add("Theme", (int)m_Settings.m_Theme);
		m_IniFile.Add("ProjectRoot", m_ProjectSettings.m_ProjectRoot);
		m_IniFile.Add("ProjectName", m_ProjectSettings.m_ProjectName);
		m_IniFile.Add("SleepOutofFocus", m_Settings.m_SleepOutofFocus);
		m_IniFile.Add("RecentProjectCount", 0);
		m_IniFile.Add("CameraSpeed", m_Settings.m_CameraSpeed);
		m_IniFile.Add("CameraNear", m_Settings.m_CameraNear);
		m_IniFile.Add("CameraFar", m_Settings.m_CameraFar);
		m_IniFile.Rewrite();
	}

	void Editor::LoadEditorSettings()
	{
		INDEX_PROFILE_FUNCTION();
		m_Settings.m_ShowGrid = m_IniFile.GetOrDefault("ShowGrid", m_Settings.m_ShowGrid);
		m_Settings.m_ShowGizmos = m_IniFile.GetOrDefault("ShowGizmos", m_Settings.m_ShowGizmos);
		m_Settings.m_ShowViewSelected = m_IniFile.GetOrDefault("ShowViewSelected", m_Settings.m_ShowViewSelected);
		m_TransitioningCamera = m_IniFile.GetOrDefault("TransitioningCamera", m_TransitioningCamera);
		m_Settings.m_ShowImGuiDemo = m_IniFile.GetOrDefault("ShowImGuiDemo", m_Settings.m_ShowImGuiDemo);
		m_Settings.m_SnapAmount = m_IniFile.GetOrDefault("SnapAmount", m_Settings.m_SnapAmount);
		m_Settings.m_SnapQuizmo = m_IniFile.GetOrDefault("SnapQuizmo", m_Settings.m_SnapQuizmo);
		m_Settings.m_DebugDrawFlags = m_IniFile.GetOrDefault("DebugDrawFlags", m_Settings.m_DebugDrawFlags);
		m_Settings.m_Theme = ImGuiUtilities::Theme(m_IniFile.GetOrDefault("Theme", (int)m_Settings.m_Theme));

		m_ProjectSettings.m_ProjectRoot = m_IniFile.GetOrDefault("ProjectRoot", std::string("../../ExampleProject/"));
		m_ProjectSettings.m_ProjectName = m_IniFile.GetOrDefault("ProjectName", std::string("Example"));
		m_Settings.m_Physics2DDebugFlags = m_IniFile.GetOrDefault("PhysicsDebugDrawFlags2D", 0);
		m_Settings.m_Physics3DDebugFlags = m_IniFile.GetOrDefault("PhysicsDebugDrawFlags", 0);
		m_Settings.m_SleepOutofFocus = m_IniFile.GetOrDefault("SleepOutofFocus", true);
		m_Settings.m_CameraSpeed = m_IniFile.GetOrDefault("CameraSpeed", 1000.0f);
		m_Settings.m_CameraNear = m_IniFile.GetOrDefault("CameraNear", 0.01f);
		m_Settings.m_CameraFar = m_IniFile.GetOrDefault("CameraFar", 2000.0f);

		m_EditorCameraController.SetSpeed(m_Settings.m_CameraSpeed);

		int recentProjectCount = 0;
		std::string projectPath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".IndexProject");

		if(FileSystem::FileExists(projectPath))
		{
			auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), projectPath);
			if(it == m_Settings.m_RecentProjects.end())
				m_Settings.m_RecentProjects.push_back(projectPath);
		}

		recentProjectCount = m_IniFile.GetOrDefault("RecentProjectCount", 0);
		for(int i = 0; i < recentProjectCount; i++)
		{
			projectPath = m_IniFile.GetOrDefault("RecentProject" + std::to_string(i), std::string());

			if(FileSystem::FileExists(projectPath))
			{
				auto it = std::find(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end(), projectPath);
				if(it == m_Settings.m_RecentProjects.end())
					m_Settings.m_RecentProjects.push_back(projectPath);
			}
		}

		std::sort(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end());
		m_Settings.m_RecentProjects.erase(std::unique(m_Settings.m_RecentProjects.begin(), m_Settings.m_RecentProjects.end()), m_Settings.m_RecentProjects.end());
	}

	const char* Editor::GetIconFontIcon(const std::string& filePath)
	{
		INDEX_PROFILE_FUNCTION();
		if(IsTextFile(filePath))
		{
			return ICON_MDI_FILE_XML;
		}
		else if(IsModelFile(filePath))
		{
			return ICON_MDI_SHAPE;
		}
		else if(IsAudioFile(filePath))
		{
			return ICON_MDI_FILE_MUSIC;
		}
		else if(IsTextureFile(filePath))
		{
			return ICON_MDI_FILE_IMAGE;
		}

		return ICON_MDI_FILE;
	}

	void Editor::CreateGridRenderer()
	{
		INDEX_PROFILE_FUNCTION();
		if(!m_GridRenderer)
			m_GridRenderer = CreateSharedPtr<Graphics::GridRenderer>(uint32_t(Application::Get().m_SceneViewWidth), uint32_t(Application::Get().m_SceneViewHeight));
	}

	const SharedPtr<Graphics::GridRenderer>& Editor::GetGridRenderer()
	{
		INDEX_PROFILE_FUNCTION();
		if(!m_GridRenderer)
			m_GridRenderer = CreateSharedPtr<Graphics::GridRenderer>(uint32_t(Application::Get().m_SceneViewWidth), uint32_t(Application::Get().m_SceneViewHeight));
		return m_GridRenderer;
	}

	void Editor::CacheScene()
	{
		INDEX_PROFILE_FUNCTION();
		Application::Get().GetCurrentScene()->Serialise(m_TempSceneSaveFilePath, false);
	}

	void Editor::LoadCachedScene()
	{
		INDEX_PROFILE_FUNCTION();

		if(FileSystem::FileExists(m_TempSceneSaveFilePath + Application::Get().GetCurrentScene()->GetSceneName() + ".index"))
		{
			Application::Get().GetCurrentScene()->Deserialise(m_TempSceneSaveFilePath, false);
		}
		else
		{
			std::string physicalPath;
			if(Index::VFS::Get().ResolvePhysicalPath("//Scenes/" + Application::Get().GetCurrentScene()->GetSceneName() + ".index", physicalPath))
			{
				auto newPath = StringUtilities::RemoveName(physicalPath);
				Application::Get().GetCurrentScene()->Deserialise(newPath, false);
			}
		}
	}
}
