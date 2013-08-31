#ifndef BLOWMORPH_SERVER_DUMMY_H_
#define BLOWMORPH_SERVER_DUMMY_H_

#include <string>

#include <base/macros.h>
#include <base/protocol.h>
#include <base/pstdint.h>

#include "entity.h"
#include "vector.h"

namespace bm {

class Dummy : public Entity {
  friend class Entity;

public:
  static Dummy* Create(
    WorldManager* world_manager,
    uint32_t id,
    const Vector2f& position,
    TimeType time
  );
  virtual ~Dummy();

  virtual std::string GetType();
  virtual bool IsStatic();

  virtual void Update(TimeType time);
  virtual void GetSnapshot(TimeType time, EntitySnapshot* output);

  virtual void OnEntityAppearance(Entity* entity);
  virtual void OnEntityDisappearance(Entity* entity);

  virtual void Damage(int damage);

  virtual void SetPosition(const Vector2f& position);

  // Double dispatch. Collision detection.

  virtual bool Collide(Entity* entity);

  virtual bool Collide(Player* other);
  virtual bool Collide(Dummy* other);
  virtual bool Collide(Bullet* other);
  virtual bool Collide(Wall* other);
  virtual bool Collide(Station* other);

protected:
  DISALLOW_COPY_AND_ASSIGN(Dummy);
  Dummy(WorldManager* world_manager, uint32_t id);

  float _speed;
  Entity* _meat;
  TimeType _last_update;
  Vector2f _prev_position;
};

} // namespace bm

#endif // BLOWMORPH_SERVER_DUMMY_H_