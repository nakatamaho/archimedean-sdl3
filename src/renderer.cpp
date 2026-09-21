#include "archview/renderer.hpp"

#include <SDL3/SDL.h>

namespace archview {

bool Renderer::skeleton_available() const noexcept
{
    return true;
}

}  // namespace archview
