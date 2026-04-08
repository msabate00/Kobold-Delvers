#pragma once

#include <string>
#include <vector>
#include "app/defs.h"

namespace sf_screenshot {

enum class Kind {
    Opaque,
    TransparentWorld
};

std::string BuildPath(Kind kind);
bool WritePngRGBA(const char* path, std::vector<uint8>& pixels, int w, int h);

} // namespace sf_screenshot
