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
            SDL_InitFlags const initialisation_flags_ = SDL_INIT_VIDEO;
      };

      class ImGuiBackend final
      {
         public:
            ImGuiBackend(SDL_Window& window, SDL_Renderer& renderer);
            ImGuiBackend(ImGuiBackend const&) = delete;
            ImGuiBackend(ImGuiBackend&&) = delete;

            ~ImGuiBackend();

            ImGuiBackend& operator=(ImGuiBackend const&) = delete;
            ImGuiBackend& operator=(ImGuiBackend&&) = delete;

            UniquePointer<ImGuiContext> const context = { ImGui::CreateContext(), ImGui::DestroyContext };
      };

      public:
         Visualiser() = default;
         Visualiser(Visualiser const&) = delete;
         Visualiser(Visualiser&&) = delete;

         ~Visualiser() = default;

         Visualiser& operator=(Visualiser const&) = delete;
         Visualiser& operator=(Visualiser&&) = delete;

         [[nodiscard]] bool tick(std::array<std::uint8_t, 0x10000>& memory);

      private:
         SDL_Context const context_ = {};

         UniquePointer<SDL_Window> const window_ =
         {
            []
            {
               SDL_Window* const window = SDL_CreateWindow("Emulator", 1280, 720, 0);
               runtime_assert(window, "failed to create window ({})", SDL_GetError());

               return window;
            }(),
            SDL_DestroyWindow
         };

         UniquePointer<SDL_Renderer> const renderer_ =
         {
            [this]
            {
               SDL_Renderer* const renderer = SDL_CreateRenderer(window_.get(), nullptr);
               runtime_assert(renderer, "failed to create renderer ({})", SDL_GetError());

               return renderer;
            }(),
            SDL_DestroyRenderer
         };

         ImGuiBackend const imgui_backend_ = { *window_, *renderer_ };
         MemoryEditor memory_editor_ = {};
   };
}

#endif