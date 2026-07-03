#pragma once

#include "Renderer.hpp"

namespace pse {

Renderer::Color16 color16_from_24(Renderer::Color24 c);

usize to_flat(u16 x, u16 y);

void blit_incr(Renderer::Blit* a);

bool blit_valid(const Renderer::Blit* a);

}
