#ifndef VISUALISER_HPP
#define VISUALISER_HPP

#include "pch.hpp"
#include "utility/runtime_assert.hpp"

namespace nes
{
   class Visualiser final
   {
      class SDL_Context final
      {
         public:
            SDL_Context();
            SDL_Context(SDL_Context const&) = delete;
            SDL_Context(SDL_Context&&) = delete;

            ~SDL_Context();

            SDL_Context& operator=(SDL_Context const&) = delete;
            SDL_Context& operator=(SDL_Context&&) = delete;

         private:
            SDL_InitFlags const initialisation_flags_{ SDL_INIT_VIDEO };
      };

      public:
         Visualiser() = default;
         Visualiser(Visualiser const&) = delete;
         Visualiser(Visualiser&&) = delete;

         ~Visualiser() = default;

         Visualiser& operator=(Visualiser const&) = delete;
         Visualiser& operator=(Visualiser&&) = delete;

      private:
         SDL_Context context_{};

         UniquePointer<SDL_Window> window_
         {
            []
            {
               SDL_Window* const window = SDL_CreateWindow("Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0);
               runtime_assert(window, "failed to create window ({})", SDL_GetError());

               return window;
            }(),
            SDL_DestroyWindow
         };

         UniquePointer<SDL_Renderer> renderer_
         {
            [this]
            {
               SDL_Renderer* const renderer = SDL_CreateRenderer(window_.get(), nullptr);
               runtime_assert(renderer, "failed to create renderer ({})", SDL_GetError());

               return renderer;
            }(),
            SDL_DestroyRenderer
         };
   };
}

#endif