#include "ProjectSerializer.h"
#include <fstream>
#include <iomanip>
#include <memory>

// ECS Core
#include "core/AssetManager.h"
#include "core/Log.h"
#include "core/Project.h"
#include "ecs/Entity.h"

// All Component Headers
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/BoxColliderComponent.h"
#include "ecs/components/CameraComponent.h"
#include "ecs/components/CircleColliderComponent.h"
#include "ecs/components/InputControllerComponent.h"
#include "ecs/components/PolygonColliderComponent.h"
#include "ecs/components/RigidBodyComponent.h"
#include "ecs/components/ScriptComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/TextComponent.h"
#include "ecs/components/VelocityComponent.h"

using json = nlohmann::json;

namespace Engine
{
  void ProjectSerializer::saveProjectFile(Project *project, const fs::path &filePath)
  {
    if (!project)
      return;

    if (!project->isLoaded)
    {
      Log::warn("[ProjectSerializer] Refusing to save project before it is loaded: " + filePath.string());
      return;
    }

    json j;

    // ---- Persistent config ----
    j["Project"] = {
        {"Name", project->config.name},
        {"Version", project->config.engineVersion},
        {"StartScene", project->config.startScenePath},
        {"AssetDirectory", project->config.assetDirectory},
        {"Width", project->config.width},
        {"Height", project->config.height}};

    // ---- Runtime-only state ----
    j["Runtime"] = {
        {"LastSceneOpened", project->runtime.lastActiveScene},
        {"LastModified", SDL_GetTicks()}};

    std::ofstream out(filePath);
    if (!out.is_open())
    {
      Log::error("[ProjectSerializer] Failed to open " + filePath.string());
      return;
    }

    out << j.dump(4);
    Log::info("[ProjectSerializer] Saved project file: " + filePath.string());
  }

  bool ProjectSerializer::loadProjectFile(Project *project, const fs::path &filepath)
  {
    std::ifstream stream(filepath);
    if (!stream.is_open())
      return false;

    json data;
    try
    {
      stream >> data;
    }
    catch (const json::parse_error &e)
    {
      Log::error("Failed to parse project file: " + std::string(e.what()));
      return false;
    }

    if (!data.contains("Project"))
    {
      Log::error("Project file missing 'Project' section.");
      return false;
    }

    // ---- Load config ----
    const auto &p = data["Project"];
    auto &config = project->config;

    config.name = p.value("Name", "Untitled Project");
    config.engineVersion = p.value("Version", "1.0.0");
    config.startScenePath = p.value("StartScene", "");
    config.assetDirectory = p.value("AssetDirectory", "assets");
    config.width = p.value("Width", 1280);
    config.height = p.value("Height", 720);

    // ---- Load runtime (optional!) ----
    if (data.contains("Runtime"))
    {
      const auto &r = data["Runtime"];
      project->runtime.lastActiveScene =
          r.value("LastSceneOpened", config.startScenePath);
    }
    else
    {
      project->runtime.lastActiveScene = config.startScenePath;
    }

    project->isLoaded = true;

    Log::info("Loaded project: " + config.name +
              " (Scene: " + project->runtime.lastActiveScene + ")");
    return true;
  }
  void ProjectSerializer::saveScene(Scene *scene, const fs::path &filePath)
  {
    json sceneJson;
    sceneJson["SceneName"] = scene->getName();

    auto &bg = scene->getBackground();
    sceneJson["Background"] = {
        {"Type", (int)bg.type},
        {"Color1", {bg.color1.r, bg.color1.g, bg.color1.b, bg.color1.a}},
        {"Color2", {bg.color2.r, bg.color2.g, bg.color2.b, bg.color2.a}},
        {"AssetId", bg.assetId},
        {"Stretch", bg.stretch}};

    // Serialize Scene Camera
    if (auto *camera = scene->getSceneCamera())
    {
      sceneJson["SceneCamera"] = {
          {"Position", {camera->getPosition().x, camera->getPosition().y}},
          {"Rotation", camera->getRotation()},
          {"OrthographicSize", camera->getOrthographicSize()},
          {"AspectRatio", camera->getAspectRatio()},
          {"NearClip", camera->getNearClip()},
          {"FarClip", camera->getFarClip()}};
    }

    sceneJson["Entities"] = json::array();
    for (auto *entity : scene->getEntityRawPointers())
      sceneJson["Entities"].push_back(serializeEntity(entity));

    std::ofstream out(filePath);
    if (out.is_open())
      out << std::setw(4) << sceneJson << std::endl;
  }

  json ProjectSerializer::serializeEntity(Entity *entity)
  {
    json j;
    j["Name"] = entity->getName();
    j["Components"] = json::object();

    // 1. Transform
    if (entity->hasComponent<TransformComponent>())
    {
      auto *c = entity->getComponent<TransformComponent>();
      j["Components"]["Transform"] = {
          {"Position", {c->position.x, c->position.y}},
          {"Rotation", c->rotation},
          {"Scale", {c->scale.x, c->scale.y}}};
    }

    // 2. Sprite
    if (entity->hasComponent<SpriteComponent>())
    {
      auto *c = entity->getComponent<SpriteComponent>();
      j["Components"]["Sprite"] = {
          {"AssetId", c->assetId},
          {"ZIndex", c->zIndex},
          {"Visible", c->visible},
          {"IsFixed", c->isFixed},
          {"FlipH", c->flipH},
          {"FlipV", c->flipV},
          {"SourceRect",
           {c->sourceRect.x, c->sourceRect.y, c->sourceRect.w, c->sourceRect.h}},
          {"Color", {c->color.r, c->color.g, c->color.b, c->color.a}}};
    }

    if (entity->hasComponent<VelocityComponent>())
    {
      auto *c = entity->getComponent<VelocityComponent>();
      j["Components"]["Velocity"] = {
          {"Velocity", {c->velocity.x, c->velocity.y}}};
    }

    if (entity->hasComponent<Engine::TextComponent>())
    {
      auto *c = entity->getComponent<Engine::TextComponent>();
      j["Components"]["TextComponent"] = {
          {"Text", c->text},
          {"FontPath", c->fontPath},
          {"FontSize", c->fontSize},
          {"Color", {c->color.r, c->color.g, c->color.b, c->color.a}},
          {"IsFixed", c->isFixed},
          {"ZIndex", c->zIndex}};
    }

    // 3. Animation
    if (entity->hasComponent<AnimationComponent>())
    {
      auto *c = entity->getComponent<AnimationComponent>();
      json anims = json::object();
      for (auto const &[name, anim] : c->animations)
      {
        anims[name] = {{"Row", anim.row},
                       {"Frames", anim.frameCount},
                       {"Speed", anim.speed}};
      }
      j["Components"]["Animation"] = {{"Animations", anims},
                                      {"Current", c->currentAnimationName},
                                      {"IsLooping", c->isLooping},
                                      {"IsPlaying", c->isPlaying}};
    }

    // 4. Box Collider
    if (entity->hasComponent<BoxColliderComponent>())
    {
      auto *c = entity->getComponent<BoxColliderComponent>();
      j["Components"]["BoxCollider"] = {{"Size", {c->size.x, c->size.y}},
                                        {"Offset", {c->offset.x, c->offset.y}},
                                        {"Rotation", c->rotation},
                                        {"Layer", c->layer},
                                        {"Mask", c->mask},
                                        {"IsTrigger", c->isTrigger},
                                        {"IsStatic", c->isStatic}};
    }

    // 5. Circle Collider
    if (entity->hasComponent<CircleColliderComponent>())
    {
      auto *c = entity->getComponent<CircleColliderComponent>();
      j["Components"]["CircleCollider"] = {
          {"Radius", c->radius}, {"Offset", {c->offset.x, c->offset.y}}, {"Layer", c->layer}, {"Mask", c->mask}, {"IsTrigger", c->isTrigger}, {"IsStatic", c->isStatic}};
    }

    // 6. Polygon Collider
    if (entity->hasComponent<PolygonColliderComponent>())
    {
      auto *c = entity->getComponent<PolygonColliderComponent>();
      json verts = json::array();
      for (const auto &v : c->vertices)
        verts.push_back({v.x, v.y});
      j["Components"]["PolygonCollider"] = {
          {"Vertices", verts},
          {"Offset", {c->offset.x, c->offset.y}},
          {"Layer", c->layer},
          {"Mask", c->mask},
          {"IsTrigger", c->isTrigger}};
    }

    // 7. RigidBody
    if (entity->hasComponent<RigidBodyComponent>())
    {
      auto *c = entity->getComponent<RigidBodyComponent>();
      j["Components"]["RigidBody"] = {{"BodyType", (int)c->bodyType},
                                      {"Mass", c->mass},
                                      {"GravityScale", c->gravityScale},
                                      {"LinearDrag", c->linearDrag}};
    }

    // 8. Input Controller
    if (entity->hasComponent<InputControllerComponent>())
    {
      auto *c = entity->getComponent<InputControllerComponent>();
      json maps = json::array();
      for (const auto &m : c->mappings)
      {
        maps.push_back({{"Action", m.actionName}, {"Key", (int)m.scancode}});
      }
      j["Components"]["InputController"] = {{"Mappings", maps},
                                            {"Parameters", c->parameters}};
    }

    // 9. Camera
    if (entity->hasComponent<CameraComponent>())
    {
      auto *c = entity->getComponent<CameraComponent>();
      j["Components"]["CameraComponent"] = {{"isPrimary", c->isPrimary}};
    }

    // 10. Script
    if (entity->hasComponent<ScriptComponent>())
    {
      auto *c = entity->getComponent<ScriptComponent>();
      j["Components"]["Script"] = {{"Path", c->scriptPath}};
    }

    return j;
  }

  bool ProjectSerializer::loadScene(std::unique_ptr<Scene> &scenePtr,
                                    SDL_Renderer *renderer,
                                    const fs::path &filepath,
                                    AssetManager *assetManager, Project *project)
  {
    std::ifstream in(filepath);
    if (!in.is_open())
      return false;

    try
    {
      Log::info("starting the load of a scene");
      json sceneJson = json::parse(in);
      scenePtr =
          std::make_unique<Scene>(sceneJson.value("SceneName", "Untitled"));
      scenePtr->setRenderer(renderer);

      if (sceneJson.contains("Background"))
      {
        auto &bgJson = sceneJson["Background"];
        BackgroundSettings bg;

        bg.type = (BackgroundType)bgJson.value("Type", 0);

        if (bgJson.contains("Color1"))
        {
          bg.color1 = {bgJson["Color1"][0], bgJson["Color1"][1],
                       bgJson["Color1"][2], bgJson["Color1"][3]};
        }
        if (bgJson.contains("Color2"))
        {
          bg.color2 = {bgJson["Color2"][0], bgJson["Color2"][1],
                       bgJson["Color2"][2], bgJson["Color2"][3]};
        }

        bg.assetId = bgJson.value("AssetId", "");
        bg.stretch = bgJson.value("Stretch", true);

        scenePtr->setBackground(bg);
      }

      // Deserialize Scene Camera
      if (sceneJson.contains("SceneCamera"))
      {
        auto &camVal = sceneJson["SceneCamera"];
        if (auto *camera = scenePtr->getSceneCamera())
        {
          // Handle Position [x, y]
          if (camVal.contains("Position") && camVal["Position"].is_array())
          {
            float x = camVal["Position"][0].get<float>();
            float y = camVal["Position"][1].get<float>();
            camera->setPosition({x, y});
          }
          else
          {
            camera->setPosition({0.0f, 0.0f});
          }

          camera->setRotation(camVal.value("Rotation", 0.0f));

          camera->setOrthographicSize(camVal.value("OrthographicSize", 10.0f));
          camera->setAspectRatio(camVal.value("AspectRatio", 1.778f));
          camera->setNearClip(camVal.value("NearClip", -1.0f));
          camera->setFarClip(camVal.value("FarClip", 1.0f));
        }
      }

      scenePtr->init();

      // Deserialize Entities
      if (sceneJson.contains("Entities"))
      {
        for (const auto &entityData : sceneJson["Entities"])
        {
          deserializeEntity(scenePtr.get(), entityData, assetManager, project);
        }
      }
    }
    catch (const std::exception &e)
    {
      Log::error("Serialization Error: " + std::string(e.what()));
      return false;
    }
    return true;
  }

  namespace fs = std::filesystem;

  std::string ProjectSerializer::findAssetPath(const fs::path &root, const std::string &fileName, int depth, int maxDepth)
  {
    // 1. Initial State & Guard Logging
    if (depth == 0)
    {
      Log::info("[AssetSearch] Starting search for: " + fileName + " in root: " + root.string());
    }

    if (depth > maxDepth)
    {
      Log::warn("[AssetSearch] Max depth reached (" + std::to_string(maxDepth) + ") at: " + root.string());
      return "";
    }

    if (!fs::exists(root))
    {
      Log::error("[AssetSearch] Root path does not exist: " + root.string());
      return "";
    }

    if (!fs::is_directory(root))
    {
      Log::warn("[AssetSearch] Path is not a directory: " + root.string());
      return "";
    }

    // 2. Check current directory
    fs::path candidate = root / fileName;
    if (fs::exists(candidate))
    {
      std::string foundPath = candidate.string();
      Log::info("[AssetSearch] Success! Found " + fileName + " at: " + foundPath);
      return foundPath;
    }

    // 3. Recursive step
    try
    {
      for (const auto &entry : fs::directory_iterator(root))
      {
        if (entry.is_directory())
        {
          // Log the branch we are about to enter
          Log::info("[AssetSearch] Stepping into sub-directory: " + entry.path().filename().string() + " (Depth: " + std::to_string(depth + 1) + ")");

          std::string found = findAssetPath(entry.path(), fileName, depth + 1, maxDepth);
          if (!found.empty())
          {
            return found;
          }
        }
      }
    }
    catch (const fs::filesystem_error &e)
    {
      Log::error("[AssetSearch] Filesystem iterator error at " + root.string() + ": " + e.what());
    }

    // 4. Failure logging (only for the initial call to avoid log spam)
    if (depth == 0)
    {
      Log::error("[AssetSearch] Failed to find " + fileName + " after recursive search.");
    }

    return "";
  }

  void ProjectSerializer::deserializeEntity(Scene *scene, const json &j,
                                            AssetManager *assetManager, Project *project)
  {
    Entity *entity = scene->createEntity(j.value("Name", "Entity"));
    const auto &comps = j["Components"];

    // 1. Transform (Note: getComponent returns a pointer, so we dereference to
    // use reference logic)
    if (comps.contains("Transform"))
    {
      auto *cPtr = entity->getComponent<TransformComponent>();
      if (cPtr)
      {
        auto &c = *cPtr;
        auto val = comps["Transform"];
        c.position = {val["Position"][0], val["Position"][1]};
        c.rotation = val["Rotation"];
        c.scale = {val["Scale"][0], val["Scale"][1]};
      }
    }

    // 2. Sprite
    if (comps.contains("Sprite"))
    {
      const auto &val = comps["Sprite"];
      std::string assetId = val.value("AssetId", "");

      if (!assetId.empty())
      {
        std::vector<fs::path> searchRoots = {project->getAssetPath()};
        std::string actualPath = "";

        for (const auto &root : searchRoots)
        {
          actualPath = findAssetPath(root, assetId, 0, 5);
          if (!actualPath.empty())
            break;
        }

        SDL_Texture *tex = nullptr;

        if (!actualPath.empty() && assetManager)
        {
          assetManager->loadTextureIfMissing(assetId, actualPath);
          tex = assetManager->getTexture(assetId);
        }
        else
        {
          Log::error("Sprite asset not found: " + assetId);
        }

        auto *cPtr = entity->addComponent<SpriteComponent>(assetId, tex);
        if (cPtr)
        {
          cPtr->zIndex = val.value("ZIndex", 0);
          cPtr->visible = val.value("Visible", true);
          cPtr->isFixed = val.value("IsFixed", false);
          cPtr->flipH = val.value("FlipH", false);
          cPtr->flipV = val.value("FlipV", false);

          if (val.contains("SourceRect") && val["SourceRect"].is_array() && val["SourceRect"].size() == 4)
          {
            cPtr->sourceRect.x = val["SourceRect"][0];
            cPtr->sourceRect.y = val["SourceRect"][1];
            cPtr->sourceRect.w = val["SourceRect"][2];
            cPtr->sourceRect.h = val["SourceRect"][3];
          }

          if (val.contains("Color") && val["Color"].is_array() && val["Color"].size() == 4)
          {
            cPtr->color.r = val["Color"][0];
            cPtr->color.g = val["Color"][1];
            cPtr->color.b = val["Color"][2];
            cPtr->color.a = val["Color"][3];
          }
        }
      }
    }
    // 3. Animation
    if (comps.contains("Animation"))
    {
      auto *cPtr = entity->addComponent<AnimationComponent>();
      if (cPtr)
      {
        auto &c = *cPtr;
        auto val = comps["Animation"];
        for (auto it = val["Animations"].begin(); it != val["Animations"].end();
             ++it)
        {
          c.addAnimation(it.key(), it.value()["Row"], it.value()["Frames"],
                         it.value()["Speed"]);
        }
        c.currentAnimationName = val["Current"];
        c.isLooping = val["IsLooping"];
        if (val["IsPlaying"])
          c.play(c.currentAnimationName);
      }
    }

    if (comps.contains("Velocity"))
    {
      auto val = comps["Velocity"];
      glm::vec2 v = {val["Velocity"][0], val["Velocity"][1]};

      auto *cPtr = entity->addComponent<VelocityComponent>(v);
      (void)cPtr;
    }

    // 4. Box Collider
    if (comps.contains("BoxCollider"))
    {
      auto val = comps["BoxCollider"];
      auto *cPtr = entity->addComponent<BoxColliderComponent>(
          glm::vec2(val["Size"][0], val["Size"][1]),
          glm::vec2(val["Offset"][0], val["Offset"][1]), (float)val["Rotation"],
          (bool)val["IsTrigger"], (bool)val["IsStatic"]);
      if (cPtr)
      {
        cPtr->layer = val["Layer"];
        cPtr->mask = val["Mask"];
      }
    }

    // 5. Circle Collider
    if (comps.contains("CircleCollider"))
    {
      auto val = comps["CircleCollider"];
      auto *cPtr = entity->addComponent<CircleColliderComponent>(
          (float)val["Radius"], glm::vec2(val["Offset"][0], val["Offset"][1]),
          (bool)val["IsTrigger"], (bool)val["IsStatic"]);
      if (cPtr)
      {
        cPtr->layer = val["Layer"];
        cPtr->mask = val["Mask"];
      }
    }

    // 6. Polygon Collider
    if (comps.contains("PolygonCollider"))
    {
      auto val = comps["PolygonCollider"];
      std::vector<glm::vec2> verts;
      for (const auto &v : val["Vertices"])
        verts.push_back({v[0], v[1]});
      auto *cPtr = entity->addComponent<PolygonColliderComponent>(
          verts, glm::vec2(val["Offset"][0], val["Offset"][1]),
          (bool)val["IsTrigger"]);
      if (cPtr)
      {
        cPtr->layer = val["Layer"];
        cPtr->mask = val["Mask"];
      }
    }

    // 7. RigidBody
    if (comps.contains("RigidBody"))
    {
      auto val = comps["RigidBody"];
      BodyType type = static_cast<BodyType>(val.value("BodyType", 0));

      auto *cPtr = entity->addComponent<RigidBodyComponent>(type);
      if (cPtr)
      {
        cPtr->mass = val.value("Mass", 1.0f);
        cPtr->gravityScale = val.value("GravityScale", 1.0f);
        cPtr->linearDrag = val.value("LinearDrag", 0.0f);
      }
    }

    // 8. Input Controller
    if (comps.contains("InputController"))
    {
      auto *cPtr = entity->addComponent<InputControllerComponent>();
      if (cPtr)
      {
        auto val = comps["InputController"];
        cPtr->mappings.clear();
        for (const auto &m : val["Mappings"])
        {
          cPtr->addMapping(m["Action"], (SDL_Scancode)m["Key"]);
        }
        cPtr->parameters = val["Parameters"].get<std::map<std::string, float>>();
      }
    }

    // 9. Camera
    if (comps.contains("CameraComponent"))
    {
      const auto &val = comps["CameraComponent"];
      auto *c = entity->addComponent<CameraComponent>();
      c->isPrimary = val.value("isPrimary", true);
    }

    // 10. Script
    if (comps.contains("Script"))
    {
      entity->addComponent<ScriptComponent>(comps["Script"]["Path"]);
    }
  }
} // namespace Engine