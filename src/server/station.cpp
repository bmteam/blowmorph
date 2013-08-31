#include "station.h"

#include <memory>
#include <string>

#include <base/error.h>
#include <base/macros.h>
#include <base/protocol.h>
#include <base/pstdint.h>

#include "vector.h"
#include "shape.h"
#include "world_manager.h"

namespace bm {

Station* Station::Create(
  WorldManager* world_manager,
  uint32_t id,
  const Vector2f& position,
  int health_regeneration,
  int blow_regeneration,
  int morph_regeneration,
  Type type
) {
  std::auto_ptr<Station> station(new Station(world_manager, id));
  CHECK(station.get() != NULL);

  std::auto_ptr<Shape> shape(world_manager->LoadShape("station.shape"));
  if(shape.get() == NULL) {
    return NULL;
  }
  shape->SetPosition(position);
  
  station->_shape = shape.release();
  station->_health_regeneration = health_regeneration;
  station->_blow_regeneration = blow_regeneration;
  station->_morph_regeneration = morph_regeneration;
  station->_type = type;

  return station.release();
}

Station::~Station() { }

std::string Station::GetType() {
  return "Station";
}
bool Station::IsStatic() {
  return false;
}

void Station::Update(TimeType time) { }

void Station::GetSnapshot(TimeType time, EntitySnapshot* output) {
  output->type = EntitySnapshot::ENTITY_TYPE_STATION;
  output->time = time;
  output->id = _id;
  output->x = _shape->GetPosition().x;
  output->y = _shape->GetPosition().y;
  if(_type == TYPE_HEALTH) {
    output->data[0] = EntitySnapshot::STATION_TYPE_HEALTH;
  } else if(_type == TYPE_BLOW) {
    output->data[0] = EntitySnapshot::STATION_TYPE_BLOW;
  } else if(_type == TYPE_MORPH) {
    output->data[0] = EntitySnapshot::STATION_TYPE_MORPH;
  } else if(_type == TYPE_COMPOSITE) {
    output->data[0] = EntitySnapshot::STATION_TYPE_COMPOSITE;
  } else {
    CHECK(false);
  }
}

void Station::OnEntityAppearance(Entity* entity) {

}
void Station::OnEntityDisappearance(Entity* entity) {

}

void Station::Damage(int damage) { }

// Double dispatch. Collision detection.

bool Station::Collide(Entity* entity) {
  return entity->Collide(this);
}

bool Station::Collide(Player* other) {
  return Entity::Collide(this, other);
}
bool Station::Collide(Dummy* other) {
  return Entity::Collide(this, other);
}
bool Station::Collide(Bullet* other) {
  return Entity::Collide(this, other);
}
bool Station::Collide(Wall* other) {
  return Entity::Collide(this, other);
}
bool Station::Collide(Station* other) {
  return Entity::Collide(other, this);
}

Station::Station(WorldManager* world_manager, uint32_t id) : Entity(world_manager, id) { }


} // namespace bm
