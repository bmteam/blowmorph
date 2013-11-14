// Copyright (c) 2013 Blowmorph Team

#include "client/object.h"

#include <string>

#include <SFML/Graphics.hpp>

#include "interpolator/interpolator.h"

#include "base/macros.h"
#include "base/pstdint.h"

namespace {

float Length(const sf::Vector2f& vector) {
  return sqrt(vector.x * vector.x + vector.y * vector.y);
}

sf::Vector2f Round(const sf::Vector2f& vector) {
  return sf::Vector2f(round(vector.x), round(vector.y));
}

}  // anonymous namespace

namespace interpolator {

template<>
sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, double ratio) {
  sf::Vector2f result;
  result.x = lerp(a.x, b.x, ratio);
  result.y = lerp(a.y, b.y, ratio);
  return result;
}

template<>
bm::ObjectState lerp(const bm::ObjectState& a,
    const bm::ObjectState& b, double ratio) {
  bm::ObjectState result;
  result.position = lerp(a.position, b.position, ratio);
  result.blowCharge = lerp(a.blowCharge, b.blowCharge, ratio);
  result.morphCharge = lerp(a.morphCharge, b.morphCharge, ratio);
  result.health = lerp(a.health, b.health, ratio);
  return result;
}

}  // namespace interpolator

namespace bm {

// TODO(alex): fix method names.
// FIXME(alex): hardcoded initial interpolation time step.
// FIXME(xiary): make Object factory.
Object::Object(const sf::Vector2f& position, int64_t time,
    uint32_t id, uint32_t type, const std::string& path)
      : id(id),
        type(type),
        visible(false),
        name_visible(false),
        interpolation_enabled(false),
        interpolator(ObjectInterpolator(75, 1)) {
  bool rv = sprite.Initialize(path);
  CHECK(rv == true);

  name_offset = sf::Vector2f(-15.0f, -30.0f);

  ObjectState state;
  state.blowCharge = 0;
  state.health = 0;
  state.morphCharge = 0;
  state.position = position;
  interpolator.Push(state, time);
}

void Object::EnableInterpolation() {
  interpolation_enabled = true;
  interpolator.SetFrameCount(2);
}

void Object::DisableInterpolation() {
  interpolation_enabled = false;
  interpolator.SetFrameCount(1);
}

void Object::EnforceState(const ObjectState& state, int64_t time) {
  interpolator.Clear();
  interpolator.Push(state, time);
}

void Object::UpdateState(const ObjectState& state, int64_t time) {
  interpolator.Push(state, time);
}

sf::Vector2f Object::GetPosition(int64_t time) {
  return interpolator.Interpolate(time).position;
}

sf::Vector2f Object::GetPosition() {
  CHECK(!interpolation_enabled);
  return GetPosition(0);
}

void Object::SetPosition(const sf::Vector2f& value) {
  CHECK(!interpolation_enabled);
  ObjectState state = interpolator.Interpolate(0);
  state.position = value;
  EnforceState(state, 0);
}

void Object::Move(const sf::Vector2f& value) {
  CHECK(!interpolation_enabled);
  ObjectState state = interpolator.Interpolate(0);
  state.position = state.position + value;
  EnforceState(state, 0);
}

void RenderObject(Object* object, int64_t time,
    sf::Font* font, sf::RenderWindow& render_window) {
  CHECK(object != NULL);
  CHECK(font != NULL);

  ObjectState state = object->interpolator.Interpolate(time);

  if (object->visible) {
    sf::Vector2f object_pos(round(state.position.x), round(state.position.y));
    object->sprite.SetPosition(object_pos);
    object->sprite.Render(&render_window);
  }

  if (object->name_visible) {
    sf::Vector2f name_pos = Round(state.position + object->name_offset);
    sf::Text text("Player", *font, 12);
    text.setPosition(name_pos.x, name_pos.y);
    render_window.draw(text);
  }
}

}  // namespace bm