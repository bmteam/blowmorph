// Copyright (c) 2015 Blowmorph Team

#ifndef SERVER_ENTITY_H_
#define SERVER_ENTITY_H_

#include <string>

#include <Box2D/Box2D.h>

#include "base/macros.h"
#include "base/protocol.h"
#include "base/pstdint.h"

#include "engine/body.h"

#include "server/id_manager.h"

namespace bm {

class Controller;

class Activator;
class Critter;
class Kit;
class Player;
class Projectile;
class Wall;

class Entity {
 public:
  static const uint32_t BAD_ID = IdManager::BAD_ID;

  enum Type {
    TYPE_ACTIVATOR,
    TYPE_CRITTER,
    TYPE_KIT,
    TYPE_PLAYER,
    TYPE_PROJECTILE,
    TYPE_WALL,
  };

  // Collision filters.
  enum FilterType {
    FILTER_ACTIVATOR  = 0x0001,
    FILTER_CRITTER    = 0x0002,
    FILTER_KIT        = 0x0004,
    FILTER_PLAYER     = 0x0008,
    FILTER_PROJECTILE = 0x0010,
    FILTER_WALL       = 0x0020,
    FILTER_ALL        = 0xffff,
    FILTER_NONE       = 0x0000
  };

 public:
  Entity(
    Controller* controller,
    uint32_t id,
    const std::string& entity_name,
    Type type,
    b2Vec2 position,
    uint16_t collision_category,
    uint16_t collision_mask);
  virtual ~Entity();

  Controller* GetController();

  // FIXME(xairy): remove dynamic config from cfg files.

  uint32_t GetId() const;
  Type GetType() const;
  bool IsStatic() const;

  b2Vec2 GetPosition() const;
  void SetPosition(const b2Vec2& position);

  float GetRotation() const;
  void SetRotation(float angle);

  b2Vec2 GetVelocity() const;
  void SetVelocity(const b2Vec2& velocity);

  float GetMass() const;
  void ApplyImpulse(const b2Vec2& impulse);
  void SetImpulse(const b2Vec2& impulse);

  // FIXME(xairy): get rid of it.
  void SetUpdatedFlag(bool value);
  bool IsUpdated() const;

  void Destroy();
  bool IsDestroyed() const;

  virtual void GetSnapshot(int64_t time, EntitySnapshot* output) = 0;
  virtual void Damage(int damage, uint32_t source_id) = 0;

  // Double dispatch. Collision handling.

  virtual void Collide(Entity* entity) = 0;

  virtual void Collide(Player* other) = 0;
  virtual void Collide(Critter* other) = 0;
  virtual void Collide(Projectile* other) = 0;
  virtual void Collide(Wall* other) = 0;
  virtual void Collide(Kit* other) = 0;
  virtual void Collide(Activator* other) = 0;

  static void Collide(Activator* first, Activator* second);
  static void Collide(Activator* first, Kit* second);
  static void Collide(Activator* first, Wall* second);
  static void Collide(Activator* first, Player* second);
  static void Collide(Activator* first, Critter* second);
  static void Collide(Activator* first, Projectile* second);

  static void Collide(Kit* first, Kit* second);
  static void Collide(Kit* first, Wall* second);
  static void Collide(Kit* first, Player* second);
  static void Collide(Kit* first, Critter* second);
  static void Collide(Kit* first, Projectile* second);

  static void Collide(Wall* first, Wall* second);
  static void Collide(Wall* first, Player* second);
  static void Collide(Wall* first, Critter* second);
  static void Collide(Wall* first, Projectile* second);

  static void Collide(Player* first, Player* second);
  static void Collide(Player* first, Critter* second);
  static void Collide(Player* first, Projectile* second);

  static void Collide(Critter* first, Critter* second);
  static void Collide(Critter* first, Projectile* second);

  static void Collide(Projectile* first, Projectile* second);

 protected:
  Controller* controller_;

  uint32_t _id;
  Type type_;

  bool _is_destroyed;
  bool _is_updated;

  Body* body_;
};

}  // namespace bm

#endif  // SERVER_ENTITY_H_
