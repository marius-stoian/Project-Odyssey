#pragma once

#include "boundary.h"

#include "core/geometry.h"
#include "game/catalogs.h"
#include "luna/engine/image.h"

#include <map>
#include <string>
#include <vector>

namespace odysseus::game {

// Programmer art for the world objects of objects.json (US-155), drawn by code like the rest until real art is chosen: one 32 x 32 picture
// per object, side by side in one image. `frame` of an object names its picture ("fire-pit", "shelter"...); an unknown name gets a grey
// block, so a new object in the data always shows up. `rects` says where each object's picture is, by object name.
luna::engine::Image makeObjectPage(const std::vector<const PlantDef*>& objects, std::map<std::string, odysseus::core::Rect>& rects);

inline constexpr int kObjectPictureSize = 32;

} // namespace odysseus::game
