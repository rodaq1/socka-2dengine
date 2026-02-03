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

using json = nlohmann::json;

namespace Engine
{
  void ProjectSerializer::saveProjectFile(Project *project, const fs::path &filePath)
  {
    if (!project)
      return;

    nlohmann::json j;
    const auto &config = project->getConfig();
    j["Project"] = {
        {"Name", config.name},
        {"Version", config.engineVersion},
        {"StartScene", config.startScenePath},
        {"AssetDirectory", config.assetDirectory},
        {"LastModified", SDL_GetTicks()},
        {"LastSceneOpened", config.lastActiveScene},
        {"Width", config.width},
        {"Height", config.height}};

    std::ofstream out(filePath);
    if (!out.is_open())
    {
      Log::error("[ProjectSerializer] Failed to open " + filePath.string());
      return;
    }
    out << j.dump(4);
    Log::info("[ProjectSerializer] Saved project file: " + filePath.string());
  }

  bool ProjectSerializer::loadProjectFile(Project *project,
                                          const fs::path &filepath)
  {
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
      return false;
    }

    nlohmann::json data;
    try
    {
      stream >> data;
    }
    catch (nlohmann::json::parse_error &e)
    {
      Log::error("Failed to parse project file: " + std::string(e.what()));
      return false;
    }

    // Ensure we are reading from the "Project" sub-object used in saveProjectFile
    if (!data.contains("Project"))
    {
      Log::error("Project file is missing 'Project' root object.");
      return false;
    }

    auto &pData = data["Project"];
    auto &config = project->getConfig();

    // Use .value() to provide safe defaults if fields are missing in older files
    config.name = pData.value("Name", "Untitled Project");
    config.engineVersion = pData.value("Version", "1.0.0");
    config.startScenePath = pData.value("StartScene", "scenes/main.json");
    config.assetDirectory = pData.value("AssetDirectory", "assets");
    config.lastActiveScene = pData.value("LastSceneOpened", "main.json");

    // LOAD WIDTH AND HEIGHT HERE
    config.width = pData.value("Width", 1280);
    config.height = pData.value("Height", 720);

    Log::info("Loaded project: " + config.name + " (" +
              std::to_string(config.width) + "x" + std::to_string(config.height) +
              ")");
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
                                    const fs::path& filepath,
                                    AssetManager *assetManager)
  {
    std::ifstream in(filepath);
    if (!in.is_open())
      return false;

    try
    {
      json sceneJson = json::parse(in);
      scenePtr =
          std::make_unique<Scene>(sceneJson.value("SceneName", "Untitled"));
      scenePtr->setRenderer(renderer);

      if (sceneJson.contains("Background")) {
            auto& bgJson = sceneJson["Background"];
            BackgroundSettings bg;
            
            bg.type = (BackgroundType)bgJson.value("Type", 0);
            
            if (bgJson.contains("Color1")) {
                bg.color1 = { bgJson["Color1"][0], bgJson["Color1"][1], 
                             bgJson["Color1"][2], bgJson["Color1"][3] };
            }
            if (bgJson.contains("Color2")) {
                bg.color2 = { bgJson["Color2"][0], bgJson["Color2"][1], 
                             bgJson["Color2"][2], bgJson["Color2"][3] };
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
          deserializeEntity(scenePtr.get(), entityData, assetManager);
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

 std::string Engine::ProjectSerializer::findAssetPath(const fs::path &root,
                                                     const std::string &fileName,
                                                     int depth,
                                                     int maxDepth) {
    if (depth > maxDepth) return "";
    fs::path candidate = root / fileName;
    if (fs::exists(candidate)) return candidate.string();

    for (auto& entry : fs::directory_iterator(root)) {
        if (entry.is_directory()) {
            std::string found = findAssetPath(entry.path(), fileName, depth + 1, maxDepth);
            if (!found.empty()) return found;
        }
    }
    return "";
}

  void ProjectSerializer::deserializeEntity(Scene *scene, const json &j,
                                            AssetManager *assetManager)
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
        fs::path assetsBase = "assets";

        std::string actualPath = findAssetPath(assetsBase, assetId, 0, 5);

        if (actualPath.empty())
        {
          std::cerr << "[ERROR] AssetManager: PHYSICAL FILE MISSING after "
                       "recursive search: "
                    << assetId << std::endl;
          return;
        }

        // Asset Loading using the discovered path
        SDL_Texture *tex = nullptr;
        if (assetManager)
        {
          assetManager->loadTextureIfMissing(assetId, actualPath);
          tex = assetManager->getTexture(assetId);
        }

        auto *cPtr = entity->addComponent<SpriteComponent>(assetId, tex);

        if (cPtr)
        {
          auto &c = *cPtr;

          // Basic property assignment
          c.zIndex = val.value("ZIndex", 0);
          c.visible = val.value("Visible", true);
          c.isFixed = val.value("IsFixed", false);
          c.flipH = val.value("FlipH", false);
          c.flipV = val.value("FlipV", false);

          // Array-based property assignment: SourceRect [x, y, w, h]
          if (val.contains("SourceRect") && val["SourceRect"].is_array())
          {
            c.sourceRect = {
                val["SourceRect"][0].get<int>(), val["SourceRect"][1].get<int>(),
                val["SourceRect"][2].get<int>(), val["SourceRect"][3].get<int>()};
          }

          // Array-based property assignment: Color [r, g, b, a]
          if (val.contains("Color") && val["Color"].is_array())
          {
            c.color = {(uint8_t)val["Color"][0].get<int>(),
                       (uint8_t)val["Color"][1].get<int>(),
                       (uint8_t)val["Color"][2].get<int>(),
                       (uint8_t)val["Color"][3].get<int>()};
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
      auto *cPtr =
          entity->addComponent<RigidBodyComponent>((BodyType)val["BodyType"]);
      if (cPtr)
      {
        cPtr->mass = val["Mass"];
        cPtr->gravityScale = val["GravityScale"];
        cPtr->linearDrag = val["LinearDrag"];
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