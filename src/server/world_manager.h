// Copyright (c) 2013 Blowmorph Team

#ifndef SERVER_WORLD_MANAGER_H_
#define SERVER_WORLD_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include <pugixml.hpp>

#include "base/pstdint.h"
#include "base/settings_manager.h"

#include "server/entity.h"
#include "server/vector.h"

#include "server/bullet.h"
#include "server/dummy.h"
#include "server/player.h"
#include "server/wall.h"
#include "server/station.h"

namespace bm {

class Entity;
class IdManager;

class WorldManager {
 public:
  explicit WorldManager(IdManager* id_manager);
  ~WorldManager();

  SettingsManager* GetSettings();

  bool LoadMap(const std::string& file);

  void AddEntity(uint32_t id, Entity* entity);
  void DeleteEntity(uint32_t id, bool deallocate);
  void DeleteEntities(const std::vector<uint32_t>& input, bool deallocate);

  Entity* GetEntity(uint32_t id);
  std::map<uint32_t, Entity*>* GetStaticEntities();
  std::map<uint32_t, Entity*>* GetDynamicEntities();
  void GetDestroyedEntities(std::vector<uint32_t>* output);

  void UpdateEntities(int64_t time);
  void CollideEntities();
  void DestroyOutlyingEntities();

  bool CreateBullet(
    uint32_t owner_id,
    const Vector2f& start,
    const Vector2f& end,
    int64_t time);

  bool CreateDummy(
    const Vector2f& position,
    int64_t time);

  bool CreateWall(
    const Vector2f& position,
    Wall::Type type);

  bool CreateStation(
    const Vector2f& position,
    int health_regeneration,
    int blow_regeneration,
    int morph_regeneration,
    Station::Type type);

  // Works only with grid map.
  bool CreateAlignedWall(float x, float y, Wall::Type type);

  // Works only with grid map.
  bool Blow(const Vector2f& location, uint32_t source_id);
  bool Morph(const Vector2f& location);

  // Returns one of the spawn positions stored in '_spawn_positions'.
  Vector2f GetRandomSpawn() const;

  // XXX(xairy): in WorldManager?
  Shape* LoadShape(const std::string& settings_prefix);

 private:
  bool _LoadWall(const pugi::xml_node& node);
  bool _LoadChunk(const pugi::xml_node& node);
  bool _LoadSpawn(const pugi::xml_node& node);
  bool _LoadStation(const pugi::xml_node& node);

  bool _LoadWallType(const pugi::xml_attribute& attribute, Wall::Type* output);
  bool _LoadStationType(const pugi::xml_attribute& attr, Station::Type* output);

  // Works only with grid map.
  bool _CreateAlignedWall(int x, int y, Wall::Type type);

  std::map<uint32_t, Entity*> _static_entities;
  std::map<uint32_t, Entity*> _dynamic_entities;

  std::vector<Vector2f> _spawn_positions;

  enum {
    MAP_NONE,
    MAP_GRID
  } _map_type;

  float _block_size;  // Works only with grid map.
  float _bound;  // Entities with greater coordinates are destroyed.

  IdManager* _id_manager;
  SettingsManager _settings;
};

}  // namespace bm

#endif  // SERVER_WORLD_MANAGER_H_
