#ifndef VISUALISER_HPP
#define VISUALISER_HPP

#include "hardware/processor/processor.hpp"
#include "pch.hpp"
#include "utility/runtime_assert.hpp"

namespace nes
{
   class Visualiser final
   {
      class SDL_Context final
      {
         public:
            SDL_Context() noexcept;
            SDL_Context(SDL_Context const&) = delete;
            SDL_Context(SDL_Context&&) = delete;

            ~SDL_Context() noexcept;

            SDL_Context& operator=(SDL_Context const&) = delete;
            SDL_Context& operator=(SDL_Context&&) = delete;

         private:
            SDL_InitFlags const initialisation_flags_{ SDL_INIT_VIDEO };
      };

      class ImGuiBackend final
      {
         public:
            ImGuiBackend(SDL_Window& window, SDL_Renderer& renderer) noexcept;
            ImGuiBackend(ImGuiBackend const&) = delete;
            ImGuiBackend(ImGuiBackend&&) = delete;

            ~ImGuiBackend() noexcept;

            ImGuiBackend& operator=(ImGuiBackend const&) = delete;
            ImGuiBackend& operator=(ImGuiBackend&&) = delete;

            UniquePointer<ImGuiContext> const context{ ImGui::CreateContext(), ImGui::DestroyContext };
      };

      public:
         Visualiser() noexcept;
         Visualiser(Visualiser const&) = delete;
         Visualiser(Visualiser&&) = delete;

         ~Visualiser() noexcept;

         Visualiser& operator=(Visualiser const&) = delete;
         Visualiser& operator=(Visualiser&&) = delete;

         [[nodiscard]] bool update(Memory const& memory, Processor& processor) noexcept;

         [[nodiscard]] bool tick_repeatedly() const noexcept;
         [[nodiscard]] bool tick_once() const noexcept;
         [[nodiscard]] bool step() const noexcept;
         [[nodiscard]] bool reset() const noexcept;

         [[nodiscard]] std::filesystem::path const& program_path() const noexcept;
         [[nodiscard]] Word program_load_address() const noexcept;
         [[nodiscard]] bool load_program_requested() const noexcept;

      private:
         SDL_Context const context_ = {};

         UniquePointer<SDL_Window> const window_ =
         {
            []
            {
               SDL_Window* const window{ SDL_CreateWindow("Emulator", 1280, 720, SDL_WINDOW_RESIZABLE) };
               runtime_assert(window, std::format("failed to create window ({})", SDL_GetError()));

               return window;
            }(),
            SDL_DestroyWindow
         };

         UniquePointer<SDL_Renderer> const renderer_ =
         {
            [this]
            {
               SDL_Renderer* const renderer{ SDL_CreateRenderer(window_.get(), nullptr) };
               runtime_assert(renderer, std::format("failed to create renderer ({})", SDL_GetError()));

               return renderer;
            }(),
            SDL_DestroyRenderer
         };

         ImGuiBackend const imgui_backend_{ *window_, *renderer_ };

         Word jump_address_{};
         int bytes_per_row_{ 16 };
         int visible_rows_{ 16 };
         bool jump_requested_{};
         std::filesystem::path program_path_{};
         Word program_load_address_{};
         bool load_program_requested_{};

         bool tick_repeatedly_{};
         bool tick_once_{};
         bool step_{};
         bool reset_{};
   };
}

#endif