// Copyright (c) 2013 Blowmorph Team

#include "server/world_manager.h"

#include <cmath>
#include <cstdlib>

#include <map>
#include <string>
#include <vector>

#include <pugixml.hpp>

#include <Box2D/Box2D.h>

#include "base/error.h"
#include "base/macros.h"
#include "base/pstdint.h"
#include "base/settings_manager.h"

#include "server/entity.h"
#include "server/id_manager.h"

#include "server/bullet.h"
#include "server/dummy.h"
#include "server/player.h"
#include "server/wall.h"
#include "server/station.h"

namespace {

// XXX(xairy): shouldn't it come from math.h?
double round(double value) {
  return ::floor(value + 0.5);
}
float round(float value) {
  return ::floorf(value + 0.5f);
}

// Returns random number in the range [0, max).
// XXX(xairy): not thread safe because of rand().
size_t Random(size_t max) {
  CHECK(max > 0);
  double zero_to_one = static_cast<double>(rand()) /  // NOLINT
    (static_cast<double>(RAND_MAX) + 1.0f);
  return static_cast<size_t>(zero_to_one * max);
}

}  // anonymous namespace

namespace bm {

WorldManager::WorldManager(IdManager* id_manager)
    : world_(b2Vec2(0.0f, 0.0f)), _map_type(MAP_NONE), _id_manager(id_manager) {
  world_.SetContactListener(&contact_listener_);
  bool rv = _settings.Open("data/entities.cfg");
  CHECK(rv == true); // FIXME.
}

WorldManager::~WorldManager() {
  std::map<uint32_t, Entity*>::iterator i, end;
  end = _static_entities.end();
  for (i = _static_entities.begin(); i != end; ++i) {
    delete i->second;
  }
  end = _dynamic_entities.end();
  for (i = _dynamic_entities.begin(); i != end; ++i) {
    delete i->second;
  }
}

b2World* WorldManager::GetWorld() {
  return &world_;
}

SettingsManager* WorldManager::GetSettings() {
  return &_settings;
}

void WorldManager::AddEntity(uint32_t id, Entity* entity) {
  CHECK(_static_entities.count(id) == 0 && _dynamic_entities.count(id) == 0);
  if (entity->IsStatic()) {
    _static_entities[id] = entity;
  } else {
    _dynamic_entities[id] = entity;
  }

  std::map<uint32_t, Entity*>::iterator itr, end;
  end = _static_entities.end();
  for (itr = _static_entities.begin(); itr != end; ++itr) {
    itr->second->OnEntityAppearance(entity);
    entity->OnEntityAppearance(itr->second);
  }
  end = _dynamic_entities.end();
  for (itr = _dynamic_entities.begin(); itr != end; ++itr) {
    itr->second->OnEntityAppearance(entity);
    entity->OnEntityAppearance(itr->second);
  }
}

void WorldManager::DeleteEntity(uint32_t id, bool deallocate) {
  CHECK(_static_entities.count(id) + _dynamic_entities.count(id) == 1);
  Entity* entity = NULL;
  if (_static_entities.count(id) == 1) {
    entity = _static_entities[id];
    _static_entities.erase(id);
  } else if (_dynamic_entities.count(id) == 1) {
    entity = _dynamic_entities[id];
    _dynamic_entities.erase(id);
  }
  CHECK(entity != NULL);

  std::map<uint32_t, Entity*>::iterator itr, end;
  end = _static_entities.end();
  for (itr = _static_entities.begin(); itr != end; ++itr) {
    itr->second->OnEntityDisappearance(entity);
  }
  end = _dynamic_entities.end();
  for (itr = _dynamic_entities.begin(); itr != end; ++itr) {
    itr->second->OnEntityDisappearance(entity);
  }

  if (deallocate) {
    delete entity;
  }
}

void WorldManager::DeleteEntities(const std::vector<uint32_t>& input,
    bool deallocate) {
  size_t size = input.size();
  for (size_t i = 0; i < size; i++) {
    DeleteEntity(input[i], deallocate);
  }
}

Entity* WorldManager::GetEntity(uint32_t id) {
  CHECK(_static_entities.count(id) + _dynamic_entities.count(id) == 1);
  if (_static_entities.count(id) == 1) {
    return _static_entities[id];
  } else if (_dynamic_entities.count(id) == 1) {
    return _dynamic_entities[id];
  }
  return NULL;
}

std::map<uint32_t, Entity*>* WorldManager::GetStaticEntities() {
  return &_static_entities;
}

std::map<uint32_t, Entity*>* WorldManager::GetDynamicEntities() {
  return &_dynamic_entities;
}

void WorldManager::GetDestroyedEntities(std::vector<uint32_t>* output) {
  output->clear();
  output->reserve(128);

  std::map<uint32_t, Entity*>::iterator itr, end;
  end = _static_entities.end();
  for (itr = _static_entities.begin(); itr != end; ++itr) {
    if (itr->second->IsDestroyed()) {
      output->push_back(itr->first);
    }
  }
  end = _dynamic_entities.end();
  for (itr = _dynamic_entities.begin(); itr != end; ++itr) {
    if (itr->second->IsDestroyed()) {
      output->push_back(itr->first);
    }
  }
}

void WorldManager::UpdateEntities(int64_t time) {
  std::map<uint32_t, Entity*>::iterator i, end;
  end = _static_entities.end();
  for (i = _static_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    entity->Update(time);
  }
  end = _dynamic_entities.end();
  for (i = _dynamic_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    entity->Update(time);
  }
}

void WorldManager::StepPhysics(int64_t time_delta) {
  int32_t velocity_iterations = 6;
  int32_t position_iterations = 2;
  world_.Step(static_cast<float>(time_delta),
      velocity_iterations, position_iterations);
}

void WorldManager::DestroyOutlyingEntities() {
  std::map<uint32_t, Entity*>::iterator i, end;
  end = _static_entities.end();
  for (i = _static_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    b2Vec2 position = entity->GetPosition();
    if (abs(position.x) > _bound || abs(position.y) > _bound) {
      entity->Destroy();
    }
  }
  end = _dynamic_entities.end();
  for (i = _dynamic_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    b2Vec2 position = entity->GetPosition();
    if (abs(position.x) > _bound || abs(position.y) > _bound) {
      if (entity->GetType() != "Player") {
        entity->Destroy();
      }
    }
  }
}

bool WorldManager::CreateBullet(
  uint32_t owner_id,
  const b2Vec2& start,
  const b2Vec2& end,
  int64_t time
) {
  CHECK(_static_entities.count(owner_id) +
    _dynamic_entities.count(owner_id) == 1);
  uint32_t id = _id_manager->NewId();
  Bullet* bullet = Bullet::Create(this, id, owner_id, start, end, time);
  if (bullet == NULL) {
    return false;
  }
  AddEntity(id, bullet);
  return true;
}

bool WorldManager::CreateDummy(
  const b2Vec2& position,
  int64_t time
) {
  uint32_t id = _id_manager->NewId();
  Dummy* dummy = Dummy::Create(this, id, position, time);
  if (dummy == NULL) {
    return false;
  }
  AddEntity(id, dummy);
  return true;
}

bool WorldManager::CreateWall(
  const b2Vec2& position,
  Wall::Type type
) {
  uint32_t id = _id_manager->NewId();
  Wall* wall = Wall::Create(this, id, position, type);
  if (wall == NULL) {
    return false;
  }
  AddEntity(id, wall);
  return true;
}

bool WorldManager::CreateStation(
  const b2Vec2& position,
  int health_regeneration,
  int blow_regeneration,
  int morph_regeneration,
  Station::Type type
) {
  uint32_t id = _id_manager->NewId();
  Station* station = Station::Create(this, id, position, health_regeneration,
    blow_regeneration, morph_regeneration, type);
  if (station == NULL) {
    return false;
  }
  AddEntity(id, station);
  return true;
}

bool WorldManager::CreateAlignedWall(float x, float y, Wall::Type type) {
  CHECK(_map_type == MAP_GRID);

  int xa = static_cast<int>(round(x / _block_size));
  int ya = static_cast<int>(round(y / _block_size));

  return _CreateAlignedWall(xa, ya, type);
}

bool WorldManager::_CreateAlignedWall(int x, int y, Wall::Type type) {
  CHECK(_map_type == MAP_GRID);

  return CreateWall(b2Vec2(x * _block_size, y * _block_size), type);
}

bool WorldManager::LoadMap(const std::string& file) {
  pugi::xml_document document;
  pugi::xml_parse_result parse_result = document.load_file(file.c_str());
  if (!parse_result) {
    Error::Throw(__FILE__, __LINE__, "Unable to parse %s!\n", file.c_str());
    return false;
  }
  pugi::xml_node map_node = document.child("map");
  if (!map_node) {
    Error::Throw(__FILE__, __LINE__,
      "Tag 'map' not found in %s!\n", file.c_str());
    return false;
  }

  pugi::xml_attribute block_size = map_node.attribute("block_size");
  if (!block_size) {
    Error::Throw(__FILE__, __LINE__,
      "Tag 'map' does not have attribute 'block_size' in %s!\n", file.c_str());
    return false;
  }
  _block_size = block_size.as_float();
  _map_type = MAP_GRID;

  pugi::xml_attribute bound = map_node.attribute("bound");
  if (!bound) {
    Error::Throw(__FILE__, __LINE__,
      "Tag 'map' does not have attribute 'bound' in %s!\n", file.c_str());
    return false;
  }
  _bound = bound.as_float();

  pugi::xml_node node;
  for (node = map_node.first_child(); node; node = node.next_sibling()) {
    if (std::string(node.name()) == "wall") {
      if (!_LoadWall(node)) {
        return false;
      }
    } else if (std::string(node.name()) == "chunk") {
      if (!_LoadChunk(node)) {
        return false;
      }
    } else if (std::string(node.name()) == "spawn") {
      if (!_LoadSpawn(node)) {
        return false;
      }
    } else if (std::string(node.name()) == "station") {
      if (!_LoadStation(node)) {
        return false;
      }
    }
  }

  return true;
}

// TODO(xairy): refactor.
bool WorldManager::_LoadWall(const pugi::xml_node& node) {
  CHECK(_map_type == MAP_GRID);
  CHECK(std::string(node.name()) == "wall");

  pugi::xml_attribute x = node.attribute("x");
  pugi::xml_attribute y = node.attribute("y");
  pugi::xml_attribute type = node.attribute("type");
  if (!x || !y || !type) {
    THROW_ERROR("Incorrect format of 'wall' in map file!\n");
    return false;
  } else {
    Wall::Type type_value;
    bool rv = _LoadWallType(type, &type_value);
    if (rv == false) {
      return false;
    }
    rv = _CreateAlignedWall(x.as_int(), y.as_int(), type_value);
    if (rv == false) {
      return false;
    }
  }

  return true;
}

// TODO(xairy): refactor.
bool WorldManager::_LoadChunk(const pugi::xml_node& node) {
  CHECK(_map_type == MAP_GRID);
  CHECK(std::string(node.name()) == "chunk");

  pugi::xml_attribute x = node.attribute("x");
  pugi::xml_attribute y = node.attribute("y");
  pugi::xml_attribute width = node.attribute("width");
  pugi::xml_attribute height = node.attribute("height");
  pugi::xml_attribute type = node.attribute("type");
  if (!x || !y || !width || !height || !type) {
    THROW_ERROR("Incorrect format of 'chunk' in map file!\n");
    return false;
  } else {
    int xv = x.as_int();
    int yv = y.as_int();
    int wv = width.as_int();
    int hv = height.as_int();
    Wall::Type type_value;
    bool rv = _LoadWallType(type, &type_value);
    if (rv == false) {
      return false;
    }
    for (int i = 0; i < wv; i++) {
      for (int j = 0; j < hv; j++) {
        bool rv = _CreateAlignedWall(xv + i, yv + j, type_value);
        if (rv == false) {
          return false;
        }
      }
    }
  }

  return true;
}

bool WorldManager::_LoadSpawn(const pugi::xml_node& node) {
  CHECK(_map_type == MAP_GRID);
  CHECK(std::string(node.name()) == "spawn");

  pugi::xml_attribute x_attr = node.attribute("x");
  pugi::xml_attribute y_attr = node.attribute("y");
  if (!x_attr || !y_attr) {
    THROW_ERROR("Incorrect format of 'spawn' in map file!\n");
    return false;
  } else {
    float x = x_attr.as_float();
    float y = y_attr.as_float();
    _spawn_positions.push_back(b2Vec2(x, y));
  }

  return true;
}

bool WorldManager::_LoadStation(const pugi::xml_node& node) {
  CHECK(_map_type == MAP_GRID);
  CHECK(std::string(node.name()) == "station");

  pugi::xml_attribute x_attr = node.attribute("x");
  pugi::xml_attribute y_attr = node.attribute("y");
  pugi::xml_attribute hr_attr = node.attribute("health_regeneration");
  pugi::xml_attribute br_attr = node.attribute("blow_regeneration");
  pugi::xml_attribute mr_attr = node.attribute("morph_regeneration");
  pugi::xml_attribute type_attr = node.attribute("type");
  if (!x_attr || !y_attr || !hr_attr || !br_attr || !mr_attr || !type_attr) {
    THROW_ERROR("Incorrect format of 'station' in map file!\n");
    return false;
  } else {
    float x = x_attr.as_float();
    float y = y_attr.as_float();
    int hr = hr_attr.as_int();
    int br = br_attr.as_int();
    int mr = mr_attr.as_int();
    Station::Type type;
    bool rv = _LoadStationType(type_attr, &type);
    if (rv == false) {
      return false;
    }
    rv = CreateStation(b2Vec2(x, y), hr, br, mr, type);
    if (rv == false) {
      return false;
    }
  }

  return true;
}

bool WorldManager::_LoadWallType(const pugi::xml_attribute& attribute,
    Wall::Type* output) {
  CHECK(std::string(attribute.name()) == "type");
  if (std::string(attribute.value()) == "ordinary") {
    *output = Wall::TYPE_ORDINARY;
  } else if (std::string(attribute.value()) == "unbreakable") {
    *output = Wall::TYPE_UNBREAKABLE;
  } else if (std::string(attribute.value()) == "morphed") {
    *output = Wall::TYPE_MORPHED;
  } else {
    THROW_ERROR("Incorrect wall type in map file!\n");
    return false;
  }
  return true;
}

bool WorldManager::_LoadStationType(const pugi::xml_attribute& attribute,
    Station::Type* output) {
  CHECK(std::string(attribute.name()) == "type");
  if (std::string(attribute.value()) == "health") {
    *output = Station::TYPE_HEALTH;
  } else if (std::string(attribute.value()) == "blow") {
    *output = Station::TYPE_BLOW;
  } else if (std::string(attribute.value()) == "morph") {
    *output = Station::TYPE_MORPH;
  } else if (std::string(attribute.value()) == "composite") {
    *output = Station::TYPE_COMPOSITE;
  } else {
    THROW_ERROR("Incorrect station type in map file!\n");
    return false;
  }
  return true;
}

void WorldManager::Blow(const b2Vec2& location) {
/*
  float radius = _settings.GetFloat("player.blow.radius");
  int damage = _settings.GetInt32("player.blow.damage");

  Circle explosion(location, radius);

  std::map<uint32_t, Entity*>::iterator i, end;
  end = _static_entities.end();
  for (i = _static_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    if (explosion.Collide(entity->GetShape())) {
      entity->Damage(damage);
    }
  }
  end = _dynamic_entities.end();
  for (i = _dynamic_entities.begin(); i != end; ++i) {
    Entity* entity = i->second;
    if (explosion.Collide(entity->GetShape())) {
      entity->Damage(damage);
    }
  }
*/
  // !FIXME: explosion.
}

void WorldManager::Morph(const b2Vec2& location) {
  int radius = _settings.GetInt32("player.morph.radius");
  int lx = static_cast<int>(round(location.x / _block_size));
  int ly = static_cast<int>(round(location.y / _block_size));
  for (int x = -radius; x <= radius; x++) {
    for (int y = -radius; y <= radius; y++) {
      if (x * x + y * y <= radius * radius) {
        bool rv = _CreateAlignedWall(lx + x, ly + y, Wall::TYPE_MORPHED);
        CHECK(rv == true); // !FIXME.
      }
    }
  }
}

b2Vec2 WorldManager::GetRandomSpawn() const {
  CHECK(_spawn_positions.size() > 0);

  size_t spawn_count = _spawn_positions.size();
  size_t spawn = Random(spawn_count);
  return _spawn_positions[spawn];
}

}  // namespace bm
