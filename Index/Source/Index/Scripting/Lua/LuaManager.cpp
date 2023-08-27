#include "Precompiled.h"
#include "LuaManager.h"
#include "Maths/Transform.h"
#include "Core/OS/Window.h"
#include "Core/VFS.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/OS/Input.h"
#include "Scene/SceneManager.h"
#include "LuaScriptComponent.h"
#include "Scene/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"

#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/Model.h"
#include "Maths/Random.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/EntityFactory.h"
#include "Physics/IndexPhysicsEngine/IndexPhysicsEngine.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"

#include <imgui/imgui.h>
#include <Tracy/TracyLua.hpp>
#include <sol/sol.hpp>

#include <filesystem>

#ifdef CUSTOM_SMART_PTR
namespace sol
{
    template <typename T>
    struct unique_usertype_traits<Index::SharedPtr<T>>
    {
        typedef T type;
        typedef Index::SharedPtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Index::UniquePtr<T>>
    {
        typedef T type;
        typedef Index::UniquePtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Index::WeakPtr<T>>
    {
        typedef T type;
        typedef Index::WeakPtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };
}

#endif

namespace Index
{

    std::vector<std::string> LuaManager::s_Identifiers = {
        "Log",
        "Trace",
        "Info",
        "Warn",
        "Error",
        "Critical",
        "Input",
        "GetKeyPressed",
        "GetKeyHeld",
        "GetMouseClicked",
        "GetMouseHeld",
        "GetMousePosition",
        "GetScrollOffset",
        "enttRegistry",
        "Entity",
        "EntityManager",
        "Create"
        "GetRegistry",
        "Valid",
        "Destroy",
        "SetParent",
        "GetParent",
        "IsParent",
        "GetChildren",
        "SetActive",
        "Active",
        "GetEntityByName",
        "AddPyramidEntity",
        "AddSphereEntity",
        "AddLightCubeEntity",
        "NameComponent",
        "GetNameComponent",
        "GetCurrentEntity",
        "SetThisComponent",
        "LuaScriptComponent",
        "GetLuaScriptComponent",
        "Transform",
        "GetTransform"
    };

    LuaManager::LuaManager()
        : m_State(nullptr)
    {
    }

    void LuaManager::OnInit()
    {
        INDEX_PROFILE_FUNCTION();

        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);
        tracy::LuaRegister(m_State->lua_state());

        BindAppLua(*m_State);
        BindInputLua(*m_State);
        BindMathsLua(*m_State);
        BindImGuiLua(*m_State);
        BindECSLua(*m_State);
        BindLogLua(*m_State);
        BindSceneLua(*m_State);
        BindPhysicsLua(*m_State);
    }

    LuaManager::~LuaManager()
    {
        delete m_State;
    }

    void LuaManager::OnInit(Scene* scene)
    {
        INDEX_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        // auto& state = *m_State;
        // std::string ScriptsPath;
        // VFS::Get().ResolvePhysicalPath("//Scripts", ScriptsPath);
        //
        //// Setup the lua path to see luarocks packages
        // auto package_path = std::filesystem::path(ScriptsPath) / "lua" / "?.lua;";
        // package_path += std::filesystem::path(ScriptsPath) / "?" / "?.lua;";
        // package_path += std::filesystem::path(ScriptsPath) / "?" / "?" / "?.lua;";

        // std::string test = state["package"]["path"];
        // state["package"]["path"] = std::string(package_path.string()) + test;

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.SetThisComponent();
            luaScript.OnInit();
        }
    }

    void LuaManager::OnUpdate(Scene* scene)
    {
        INDEX_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        float dt = (float)Engine::Get().GetTimeStep().GetSeconds();

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.OnUpdate(dt);
        }
    }

    void LuaManager::OnNewProject(const std::string& projectPath)
    {
        auto& state = *m_State;
        std::string ScriptsPath;
        VFS::Get().ResolvePhysicalPath("//Scripts", ScriptsPath);

        // Setup the lua path to see luarocks packages
        auto package_path = std::filesystem::path(ScriptsPath) / "lua" / "?.lua;";
        package_path += std::filesystem::path(ScriptsPath) / "?" / "?.lua;";
        package_path += std::filesystem::path(ScriptsPath) / "?" / "?" / "?.lua;";

        std::string currentPaths = state["package"]["path"];
        state["package"]["path"] = std::string(package_path.string()) + currentPaths;
    }

    entt::entity GetEntityByName(entt::registry& registry, const std::string& name)
    {
        INDEX_PROFILE_FUNCTION();
        entt::entity e = entt::null;
        registry.view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component)
                                            {
                if(name == component.name)
                {
                    e = entity;
                } });

        return e;
    }

    void LuaManager::BindLogLua(sol::state& state)
    {
        INDEX_PROFILE_FUNCTION();
        auto log = state.create_table("Log");

        log.set_function("Trace", [&](sol::this_state s, std::string_view message)
                         { INDEX_LOG_TRACE(message); });

        log.set_function("Info", [&](sol::this_state s, std::string_view message)
                         { INDEX_LOG_TRACE(message); });

        log.set_function("Warn", [&](sol::this_state s, std::string_view message)
                         { INDEX_LOG_WARN(message); });

        log.set_function("Error", [&](sol::this_state s, std::string_view message)
                         { INDEX_LOG_ERROR(message); });

        log.set_function("Critical", [&](sol::this_state s, std::string_view message)
                         { INDEX_LOG_CRITICAL(message); });
    }

    void LuaManager::BindInputLua(sol::state& state)
    {
        INDEX_PROFILE_FUNCTION();
        auto input = state["Input"].get_or_create<sol::table>();

        input.set_function("GetKeyPressed", [](Index::InputCode::Key key) -> bool
                           { return Input::Get().GetKeyPressed(key); });

        input.set_function("GetKeyHeld", [](Index::InputCode::Key key) -> bool
                           { return Input::Get().GetKeyHeld(key); });

        input.set_function("GetMouseClicked", [](Index::InputCode::MouseKey key) -> bool
                           { return Input::Get().GetMouseClicked(key); });

        input.set_function("GetMouseHeld", [](Index::InputCode::MouseKey key) -> bool
                           { return Input::Get().GetMouseHeld(key); });

        input.set_function("GetMousePosition", []() -> glm::vec2
                           { return Input::Get().Get().GetMousePosition(); });

        input.set_function("GetScrollOffset", []() -> float
                           { return Input::Get().Get().GetScrollOffset(); });

        input.set_function("GetControllerAxis", [](int id, int axis) -> float
                           { return Input::Get().GetControllerAxis(id, axis); });

        input.set_function("GetControllerName", [](int id) -> std::string
                           { return Input::Get().GetControllerName(id); });

        input.set_function("GetControllerHat", [](int id, int hat) -> int
                           { return Input::Get().GetControllerHat(id, hat); });

        input.set_function("IsControllerButtonPressed", [](int id, int button) -> bool
                           { return Input::Get().IsControllerButtonPressed(id, button); });

        std::initializer_list<std::pair<sol::string_view, Index::InputCode::Key>> keyItems = {
            { "A", Index::InputCode::Key::A },
            { "B", Index::InputCode::Key::B },
            { "C", Index::InputCode::Key::C },
            { "D", Index::InputCode::Key::D },
            { "E", Index::InputCode::Key::E },
            { "F", Index::InputCode::Key::F },
            { "H", Index::InputCode::Key::G },
            { "G", Index::InputCode::Key::H },
            { "I", Index::InputCode::Key::I },
            { "J", Index::InputCode::Key::J },
            { "K", Index::InputCode::Key::K },
            { "L", Index::InputCode::Key::L },
            { "M", Index::InputCode::Key::M },
            { "N", Index::InputCode::Key::N },
            { "O", Index::InputCode::Key::O },
            { "P", Index::InputCode::Key::P },
            { "Q", Index::InputCode::Key::Q },
            { "R", Index::InputCode::Key::R },
            { "S", Index::InputCode::Key::S },
            { "T", Index::InputCode::Key::T },
            { "U", Index::InputCode::Key::U },
            { "V", Index::InputCode::Key::V },
            { "W", Index::InputCode::Key::W },
            { "X", Index::InputCode::Key::X },
            { "Y", Index::InputCode::Key::Y },
            { "Z", Index::InputCode::Key::Z },
            //{ "UNKOWN", Index::InputCode::Key::Unknown },
            { "Space", Index::InputCode::Key::Space },
            { "Escape", Index::InputCode::Key::Escape },
            { "APOSTROPHE", Index::InputCode::Key::Apostrophe },
            { "Comma", Index::InputCode::Key::Comma },
            { "MINUS", Index::InputCode::Key::Minus },
            { "PERIOD", Index::InputCode::Key::Period },
            { "SLASH", Index::InputCode::Key::Slash },
            { "SEMICOLON", Index::InputCode::Key::Semicolon },
            { "EQUAL", Index::InputCode::Key::Equal },
            { "LEFT_BRACKET", Index::InputCode::Key::LeftBracket },
            { "BACKSLASH", Index::InputCode::Key::Backslash },
            { "RIGHT_BRACKET", Index::InputCode::Key::RightBracket },
            //{ "BACK_TICK", Index::InputCode::Key::BackTick },
            { "Enter", Index::InputCode::Key::Enter },
            { "Tab", Index::InputCode::Key::Tab },
            { "Backspace", Index::InputCode::Key::Backspace },
            { "Insert", Index::InputCode::Key::Insert },
            { "Delete", Index::InputCode::Key::Delete },
            { "Right", Index::InputCode::Key::Right },
            { "Left", Index::InputCode::Key::Left },
            { "Down", Index::InputCode::Key::Down },
            { "Up", Index::InputCode::Key::Up },
            { "PageUp", Index::InputCode::Key::PageUp },
            { "PageDown", Index::InputCode::Key::PageDown },
            { "Home", Index::InputCode::Key::Home },
            { "End", Index::InputCode::Key::End },
            { "CAPS_LOCK", Index::InputCode::Key::CapsLock },
            { "SCROLL_LOCK", Index::InputCode::Key::ScrollLock },
            { "NumLock", Index::InputCode::Key::NumLock },
            { "PrintScreen", Index::InputCode::Key::PrintScreen },
            { "Pasue", Index::InputCode::Key::Pause },
            { "LeftShift", Index::InputCode::Key::LeftShift },
            { "LeftControl", Index::InputCode::Key::LeftControl },
            { "LEFT_ALT", Index::InputCode::Key::LeftAlt },
            { "LEFT_SUPER", Index::InputCode::Key::LeftSuper },
            { "RightShift", Index::InputCode::Key::RightShift },
            { "RightControl", Index::InputCode::Key::RightControl },
            { "RIGHT_ALT", Index::InputCode::Key::RightAlt },
            { "RIGHT_SUPER", Index::InputCode::Key::RightSuper },
            { "Menu", Index::InputCode::Key::Menu },
            { "F1", Index::InputCode::Key::F1 },
            { "F2", Index::InputCode::Key::F2 },
            { "F3", Index::InputCode::Key::F3 },
            { "F4", Index::InputCode::Key::F4 },
            { "F5", Index::InputCode::Key::F5 },
            { "F6", Index::InputCode::Key::F6 },
            { "F7", Index::InputCode::Key::F7 },
            { "F8", Index::InputCode::Key::F8 },
            { "F9", Index::InputCode::Key::F9 },
            { "F10", Index::InputCode::Key::F10 },
            { "F11", Index::InputCode::Key::F11 },
            { "F12", Index::InputCode::Key::F12 },
            { "Keypad0", Index::InputCode::Key::D0 },
            { "Keypad1", Index::InputCode::Key::D1 },
            { "Keypad2", Index::InputCode::Key::D2 },
            { "Keypad3", Index::InputCode::Key::D3 },
            { "Keypad4", Index::InputCode::Key::D4 },
            { "Keypad5", Index::InputCode::Key::D5 },
            { "Keypad6", Index::InputCode::Key::D6 },
            { "Keypad7", Index::InputCode::Key::D7 },
            { "Keypad8", Index::InputCode::Key::D8 },
            { "Keypad9", Index::InputCode::Key::D9 },
            { "Decimal", Index::InputCode::Key::Period },
            { "Divide", Index::InputCode::Key::Slash },
            { "Multiply", Index::InputCode::Key::KPMultiply },
            { "Subtract", Index::InputCode::Key::Minus },
            { "Add", Index::InputCode::Key::KPAdd },
            { "KP_EQUAL", Index::InputCode::Key::KPEqual }
        };
        state.new_enum<Index::InputCode::Key, false>("Key", keyItems); // false makes it read/write in Lua, but its faster

        std::initializer_list<std::pair<sol::string_view, Index::InputCode::MouseKey>> mouseItems = {
            { "Left", Index::InputCode::MouseKey::ButtonLeft },
            { "Right", Index::InputCode::MouseKey::ButtonRight },
            { "Middle", Index::InputCode::MouseKey::ButtonMiddle },
        };
        state.new_enum<Index::InputCode::MouseKey, false>("MouseButton", mouseItems);
    }

    SharedPtr<Graphics::Texture2D> LoadTexture(const std::string& name, const std::string& path)
    {
        INDEX_PROFILE_FUNCTION();
        return SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
    }

    SharedPtr<Graphics::Texture2D> LoadTextureWithParams(const std::string& name, const std::string& path, Index::Graphics::TextureFilter filter, Index::Graphics::TextureWrap wrapMode)
    {
        INDEX_PROFILE_FUNCTION();
        return SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path, Graphics::TextureDesc(filter, filter, wrapMode)));
    }

    void LuaManager::BindECSLua(sol::state& state)
    {
        INDEX_PROFILE_FUNCTION();

        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("enttRegistry");

        sol::usertype<Entity> entityType               = state.new_usertype<Entity>("Entity", sol::constructors<sol::types<entt::entity, Scene*>>());
        sol::usertype<EntityManager> entityManagerType = state.new_usertype<EntityManager>("EntityManager");
        entityManagerType.set_function("Create", static_cast<Entity (EntityManager::*)()>(&EntityManager::Create));
        entityManagerType.set_function("GetRegistry", &EntityManager::GetRegistry);

        entityType.set_function("Valid", &Entity::Valid);
        entityType.set_function("Destroy", &Entity::Destroy);
        entityType.set_function("SetParent", &Entity::SetParent);
        entityType.set_function("GetParent", &Entity::GetParent);
        entityType.set_function("IsParent", &Entity::IsParent);
        entityType.set_function("GetChildren", &Entity::GetChildren);
        entityType.set_function("SetActive", &Entity::SetActive);
        entityType.set_function("Active", &Entity::Active);

        state.set_function("GetEntityByName", &GetEntityByName);

        state.set_function("AddPyramidEntity", &EntityFactory::AddPyramid);
        state.set_function("AddSphereEntity", &EntityFactory::AddSphere);
        state.set_function("AddLightCubeEntity", &EntityFactory::AddLightCube);

        sol::usertype<NameComponent> nameComponent_type = state.new_usertype<NameComponent>("NameComponent");
        nameComponent_type["name"]                      = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS(state, NameComponent, static_cast<NameComponent& (Entity::*)()>(&Entity::AddComponent<NameComponent>));

        sol::usertype<LuaScriptComponent> script_type = state.new_usertype<LuaScriptComponent>("LuaScriptComponent", sol::constructors<sol::types<std::string, Scene*>>());
        REGISTER_COMPONENT_WITH_ECS(state, LuaScriptComponent, static_cast<LuaScriptComponent& (Entity::*)(std::string&&, Scene*&&)>(&Entity::AddComponent<LuaScriptComponent, std::string, Scene*>));
        script_type.set_function("GetCurrentEntity", &LuaScriptComponent::GetCurrentEntity);
        script_type.set_function("SetThisComponent", &LuaScriptComponent::SetThisComponent);

        using namespace Maths;
        REGISTER_COMPONENT_WITH_ECS(state, Transform, static_cast<Transform& (Entity::*)()>(&Entity::AddComponent<Transform>));

        using namespace Graphics;
        sol::usertype<TextComponent> textComponent_type = state.new_usertype<TextComponent>("TextComponent");
        textComponent_type["TextString"]                = &TextComponent::TextString;
        textComponent_type["Colour"]                    = &TextComponent::Colour;
        textComponent_type["MaxWidth"]                  = &TextComponent::MaxWidth;

        REGISTER_COMPONENT_WITH_ECS(state, TextComponent, static_cast<TextComponent& (Entity::*)()>(&Entity::AddComponent<TextComponent>));

        sol::usertype<Sprite> sprite_type = state.new_usertype<Sprite>("Sprite", sol::constructors<sol::types<glm::vec2, glm::vec2, glm::vec4>, Sprite(const SharedPtr<Graphics::Texture2D>&, const glm::vec2&, const glm::vec2&, const glm::vec4&)>());
        sprite_type.set_function("SetTexture", &Sprite::SetTexture);
        sprite_type.set_function("SetSpriteSheet", &Sprite::SetSpriteSheet);

        REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (Entity::*)(const glm::vec2&, const glm::vec2&, const glm::vec4&)>(&Entity::AddComponent<Sprite, const glm::vec2&, const glm::vec2&, const glm::vec4&>));

        state.new_usertype<Light>(
            "Light",
            "Intensity", &Light::Intensity,
            "Radius", &Light::Radius,
            "Colour", &Light::Colour,
            "Direction", &Light::Direction,
            "Position", &Light::Position,
            "Type", &Light::Type,
            "Angle", &Light::Angle);

        REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (Entity::*)()>(&Entity::AddComponent<Light>));

        std::initializer_list<std::pair<sol::string_view, Index::Graphics::PrimitiveType>> primitives = {
            { "Cube", Index::Graphics::PrimitiveType::Cube },
            { "Plane", Index::Graphics::PrimitiveType::Plane },
            { "Quad", Index::Graphics::PrimitiveType::Quad },
            { "Pyramid", Index::Graphics::PrimitiveType::Pyramid },
            { "Sphere", Index::Graphics::PrimitiveType::Sphere },
            { "Capsule", Index::Graphics::PrimitiveType::Capsule },
            { "Cylinder", Index::Graphics::PrimitiveType::Cylinder },
            { "Terrain", Index::Graphics::PrimitiveType::Terrain },
        };

        state.new_enum<Index::Graphics::PrimitiveType, false>("PrimitiveType", primitives);

        state.new_usertype<Model>("Model",
                                  // Constructors
                                  sol::constructors<
                                      Index::Graphics::Model(),
                                      Index::Graphics::Model(const std::string&),
                                      Index::Graphics::Model(const Index::SharedPtr<Index::Graphics::Mesh>&, Index::Graphics::PrimitiveType),
                                      Index::Graphics::Model(Index::Graphics::PrimitiveType)>(),
                                  // Properties
                                  "meshes", &Index::Graphics::Model::GetMeshes,
                                  "skeleton", &Index::Graphics::Model::GetSkeleton,
                                  "animations", &Index::Graphics::Model::GetAnimations,
                                  "file_path", &Index::Graphics::Model::GetFilePath,
                                  "primitive_type", sol::property(&Index::Graphics::Model::GetPrimitiveType, &Index::Graphics::Model::SetPrimitiveType),
                                  // Methods
                                  "add_mesh", &Index::Graphics::Model::AddMesh,
                                  "load_model", &Index::Graphics::Model::LoadModel);

        REGISTER_COMPONENT_WITH_ECS(state, Model, static_cast<Model& (Entity::*)(const std::string&)>(&Entity::AddComponent<Model, const std::string&>));

        // Member functions
        sol::usertype<Material> material_type = state.new_usertype<Material>("Material",

                                                                             sol::constructors<
                                                                                 Index::Graphics::Material()>(),
                                                                             // Setters
                                                                             "set_albedo_texture", &Material::SetAlbedoTexture,
                                                                             "set_normal_texture", &Material::SetNormalTexture,
                                                                             "set_roughness_texture", &Material::SetRoughnessTexture,
                                                                             "set_metallic_texture", &Material::SetMetallicTexture,
                                                                             "set_ao_texture", &Material::SetAOTexture,
                                                                             "set_emissive_texture", &Material::SetEmissiveTexture,

                                                                             // Getters
                                                                             "get_name", &Material::GetName,
                                                                             "get_properties", &Material::GetProperties,
                                                                             //"get_textures", &Material::GetTextures,
                                                                             "get_shader", &Material::GetShader,

                                                                             // Other member functions
                                                                             "load_pbr_material", &Material::LoadPBRMaterial,
                                                                             "load_material", &Material::LoadMaterial,
                                                                             "set_textures", &Material::SetTextures,
                                                                             "set_material_properties", &Material::SetMaterialProperites,
                                                                             "update_material_properties_data", &Material::UpdateMaterialPropertiesData,
                                                                             "set_name", &Material::SetName,
                                                                             "bind", &Material::Bind);

        // Enum for RenderFlags
        std::initializer_list<std::pair<sol::string_view, Material::RenderFlags>> render_flags = {
            { "NONE", Material::RenderFlags::NONE },
            { "DEPTHTEST", Material::RenderFlags::DEPTHTEST },
            { "WIREFRAME", Material::RenderFlags::WIREFRAME },
            { "FORWARDRENDER", Material::RenderFlags::FORWARDRENDER },
            { "DEFERREDRENDER", Material::RenderFlags::DEFERREDRENDER },
            { "NOSHADOW", Material::RenderFlags::NOSHADOW },
            { "TWOSIDED", Material::RenderFlags::TWOSIDED },
            { "ALPHABLEND", Material::RenderFlags::ALPHABLEND }

        };

        state.new_enum<Material::RenderFlags, false>("RenderFlags", render_flags);

        sol::usertype<Camera> camera_type = state.new_usertype<Camera>("Camera", sol::constructors<Camera(float, float, float, float), Camera(float, float)>());
        camera_type["fov"]                = &Camera::GetFOV;
        camera_type["aspectRatio"]        = &Camera::GetAspectRatio;
        camera_type["nearPlane"]          = &Camera::GetNear;
        camera_type["farPlane"]           = &Camera::GetFar;
        camera_type["SetIsOrthographic"]  = &Camera::SetIsOrthographic;
        camera_type["SetNearPlane"]       = &Camera::SetNear;
        camera_type["SetFarPlane"]        = &Camera::SetFar;

        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (Entity::*)(const float&, const float&)>(&Entity::AddComponent<Camera, const float&, const float&>));

        sol::usertype<RigidBody3DComponent> RigidBody3DComponent_type = state.new_usertype<RigidBody3DComponent>("RigidBody3DComponent", sol::constructors<sol::types<const SharedPtr<RigidBody3D>&>>());
        RigidBody3DComponent_type.set_function("GetRigidBody", &RigidBody3DComponent::GetRigidBody);
        REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)(SharedPtr<RigidBody3D>&)>(&Entity::AddComponent<RigidBody3DComponent, SharedPtr<RigidBody3D>&>));

        sol::usertype<RigidBody2DComponent> RigidBody2DComponent_type = state.new_usertype<RigidBody2DComponent>("RigidBody2DComponent", sol::constructors<sol::types<const RigidBodyParameters&>>());
        RigidBody2DComponent_type.set_function("GetRigidBody", &RigidBody2DComponent::GetRigidBody);

        REGISTER_COMPONENT_WITH_ECS(state, RigidBody2DComponent, static_cast<RigidBody2DComponent& (Entity::*)(const RigidBodyParameters&)>(&Entity::AddComponent<RigidBody2DComponent, const RigidBodyParameters&>));

        REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (Entity::*)()>(&Entity::AddComponent<SoundComponent>));

        auto mesh_type = state.new_usertype<Index::Graphics::Mesh>("Mesh",
                                                                   sol::constructors<Index::Graphics::Mesh(), Index::Graphics::Mesh(const Index::Graphics::Mesh&),
                                                                                     Index::Graphics::Mesh(const std::vector<uint32_t>&, const std::vector<Vertex>&, float)>());

        // Bind the member functions and variables
        mesh_type["GetMaterial"]    = &Index::Graphics::Mesh::GetMaterial;
        mesh_type["SetMaterial"]    = &Index::Graphics::Mesh::SetMaterial;
        mesh_type["GetBoundingBox"] = &Index::Graphics::Mesh::GetBoundingBox;
        mesh_type["GetActive"]      = &Index::Graphics::Mesh::GetActive;
        mesh_type["SetName"]        = &Index::Graphics::Mesh::SetName;

        std::initializer_list<std::pair<sol::string_view, Index::Graphics::TextureFilter>> textureFilter = {
            { "None", Index::Graphics::TextureFilter::NONE },
            { "Linear", Index::Graphics::TextureFilter::LINEAR },
            { "Nearest", Index::Graphics::TextureFilter::NEAREST }
        };

        std::initializer_list<std::pair<sol::string_view, Index::Graphics::TextureWrap>> textureWrap = {
            { "None", Index::Graphics::TextureWrap::NONE },
            { "Repeat", Index::Graphics::TextureWrap::REPEAT },
            { "Clamp", Index::Graphics::TextureWrap::CLAMP },
            { "MirroredRepeat", Index::Graphics::TextureWrap::MIRRORED_REPEAT },
            { "ClampToEdge", Index::Graphics::TextureWrap::CLAMP_TO_EDGE },
            { "ClampToBorder", Index::Graphics::TextureWrap::CLAMP_TO_BORDER }
        };

        state.set_function("LoadMesh", &CreatePrimative);

        state.new_enum<Index::Graphics::TextureWrap, false>("TextureWrap", textureWrap);
        state.new_enum<Index::Graphics::TextureFilter, false>("TextureFilter", textureFilter);

        state.set_function("LoadTexture", &LoadTexture);
        state.set_function("LoadTextureWithParams", &LoadTextureWithParams);
    }

    static float LuaRand(float a, float b)
    {
        return Random32::Rand(a, b);
    }

    void LuaManager::BindSceneLua(sol::state& state)
    {
        sol::usertype<Scene> scene_type = state.new_usertype<Scene>("Scene");
        scene_type.set_function("GetRegistry", &Scene::GetRegistry);
        scene_type.set_function("GetEntityManager", &Scene::GetEntityManager);

        sol::usertype<Graphics::Texture2D> texture2D_type = state.new_usertype<Graphics::Texture2D>("Texture2D");
        texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);

        state.set_function("Rand", &LuaRand);
    }

    static void SwitchSceneByIndex(int index)
    {
        Application::Get().GetSceneManager()->SwitchScene(index);
    }

    static void SwitchScene()
    {
        Application::Get().GetSceneManager()->SwitchScene();
    }

    static void SwitchSceneByName(const std::string& name)
    {
        Application::Get().GetSceneManager()->SwitchScene(name);
    }

    static void SetPhysicsDebugFlags(int flags)
    {
        Application::Get().GetSystem<IndexPhysicsEngine>()->SetDebugDrawFlags(flags);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
        state.set_function("SwitchSceneByName", &SwitchSceneByName);
        state.set_function("SwitchScene", &SwitchScene);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);

        std::initializer_list<std::pair<sol::string_view, Index::PhysicsDebugFlags>> physicsDebugFlags = {
            { "CONSTRAINT", Index::PhysicsDebugFlags::CONSTRAINT },
            { "MANIFOLD", Index::PhysicsDebugFlags::MANIFOLD },
            { "COLLISIONVOLUMES", Index::PhysicsDebugFlags::COLLISIONVOLUMES },
            { "COLLISIONNORMALS", Index::PhysicsDebugFlags::COLLISIONNORMALS },
            { "AABB", Index::PhysicsDebugFlags::AABB },
            { "LINEARVELOCITY", Index::PhysicsDebugFlags::LINEARVELOCITY },
            { "LINEARFORCE", Index::PhysicsDebugFlags::LINEARFORCE },
            { "BROADPHASE", Index::PhysicsDebugFlags::BROADPHASE },
            { "BROADPHASE_PAIRS", Index::PhysicsDebugFlags::BROADPHASE_PAIRS },
            { "BOUNDING_RADIUS", Index::PhysicsDebugFlags::BOUNDING_RADIUS },
        };

        state.new_enum<PhysicsDebugFlags, false>("PhysicsDebugFlags", physicsDebugFlags);

        app_type.set_function("GetWindowSize", &Application::GetWindowSize);
        state.set_function("GetAppInstance", &Application::Get);
    }
}
