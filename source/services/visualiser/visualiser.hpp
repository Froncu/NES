#ifndef VISUALISER_HPP
#define VISUALISER_HPP

#include "hardware/CPU/CPU.hpp"
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

         [[nodiscard]] bool update(Memory const& memory, CPU const& processor);

         [[nodiscard]] bool run() const;
         [[nodiscard]] bool tick() const;

      private:
         SDL_Context const context_ = {};

         UniquePointer<SDL_Window> const window_ =
         {
            []
            {
               SDL_Window* const window = SDL_CreateWindow("Emulator", 1280, 720, SDL_WINDOW_RESIZABLE);
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

         ImGuiBackend const imgui_backend_{ *window_, *renderer_ };

         ProgramCounter jump_address_ = 0;
         int bytes_per_row_ = 16;
         int visible_rows_ = 16;

         bool jump_requested_ = false;
         bool run_ = false;
         bool tick_ = false;
   };
}

#endif