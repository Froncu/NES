#include "visualiser.hpp"

namespace nes
{
   Visualiser::SDL_Context::SDL_Context()
   {
      bool const succeeded = SDL_Init(initialisation_flags_);
      runtime_assert(succeeded, "failed to initialise SDL ({})", SDL_GetError());
   }

   Visualiser::SDL_Context::~SDL_Context()
   {
      SDL_QuitSubSystem(initialisation_flags_);
   }
}