﻿#ifndef BLOWMORPH_CLIENT_TEXTURE_MANAGER_HPP_
#define BLOWMORPH_CLIENT_TEXTURE_MANAGER_HPP_

#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <base/pstdint.hpp>

namespace bm {

class Texture;

// TODO[16.8.2012 alex]: replace it with a generic Rect/rect.
struct TileRect {
  size_t x;
  size_t y;
  size_t width;
  size_t height;

  TileRect(size_t x, size_t y, size_t width, size_t height) : x(x), y(y), width(width), height(height) { }
};
typedef std::vector<TileRect> Tileset;

class TextureAtlas {
public:
  ~TextureAtlas();
  
  Texture* GetTexture() const;
  
  glm::vec2 GetSize() const;
  size_t GetTileCount() const;
  glm::vec2 GetTilePosition(size_t i) const;
  glm::vec2 GetTileSize(size_t i) const;

private:
  TextureAtlas();
  
  Texture* texture;
  Tileset tileset;
  
  friend TextureAtlas* LoadOldTexture(const std::string& path,
                                 bm::uint32_t transparentColor);
  friend TextureAtlas* LoadTileset(const std::string& path,
                                 bm::uint32_t transparentColor,
                                 size_t startX, size_t startY, 
                                 size_t horizontalStep, size_t verticalStep,
                                 size_t tileWidth, size_t tileHeight);
};

// Loads an image or a tileset from a given path.
// Returns NULL on failure.
TextureAtlas* LoadOldTexture(const std::string& path,
                        bm::uint32_t transparentColor = 0xFFFFFFFF);

// Loads a tileset from a given path.
// Returns NULL on failure.
TextureAtlas* LoadTileset(const std::string& path,
                     bm::uint32_t transparentColor = 0xFFFFFFFF,
                     size_t startX = 0, size_t startY = 0, 
                     size_t horizontalStep = 0, size_t verticalStep = 0,
                     size_t tileWidth = 0, size_t tileHeight = 0);

Tileset MakeSimpleTileset(size_t startX = 0, size_t startY = 0, 
                          size_t horizontalStep = 0, size_t verticalStep = 0,
                          size_t tileWidth = 0, size_t tileHeight = 0,
                          size_t imageWidth = 0, size_t imageHeight = 0);

} // namespace bm

#endif // BLOWMORPH_CLIENT_TEXTURE_MANAGER_HPP_