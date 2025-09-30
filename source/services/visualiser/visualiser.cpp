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

   Visualiser::ImGuiBackend::ImGuiBackend(SDL_Window& window, SDL_Renderer& renderer)
   {
      bool succeeded{ ImGui_ImplSDL3_InitForSDLRenderer(&window, &renderer) };
      runtime_assert(succeeded, "failed to initialize ImGui's SDL implementation");

      succeeded = ImGui_ImplSDLRenderer3_Init(&renderer);
      runtime_assert(succeeded, "failed to initialize ImGui for SDL renderer");
   }

   Visualiser::ImGuiBackend::~ImGuiBackend()
   {
      ImGui_ImplSDLRenderer3_Shutdown();
      ImGui_ImplSDL3_Shutdown();
   }

   bool Visualiser::tick(std::array<std::uint8_t, 0x10000>& memory)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         ImGui_ImplSDL3_ProcessEvent(&event);

         switch (event.type)
         {
            case SDL_EVENT_QUIT:
               return false;

            default:
               break;
         }
      }

      ImGui_ImplSDLRenderer3_NewFrame();
      ImGui_ImplSDL3_NewFrame();
      ImGui::NewFrame();

      MemoryEditor::Sizes sizes;
      memory_editor_.CalcSizes(sizes, memory.size(), 0);
      ImGui::SetNextWindowSize({ sizes.WindowWidth, sizes.WindowWidth * 0.60f }, ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSizeConstraints({}, { sizes.WindowWidth, std::numeric_limits<float>::max() });
      if (ImGui::Begin("Memory", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse))
      {
         memory_editor_.DrawContents(memory.data(), memory.size(), 0);
         if (memory_editor_.ContentsWidthChanged)
         {
            memory_editor_.CalcSizes(sizes, memory.size(), 0);
            ImGui::SetWindowSize(ImVec2(sizes.WindowWidth, ImGui::GetWindowSize().y));
         }
      }
      ImGui::End();

      ImGui::Render();
      SDL_RenderClear(renderer_.get());
      ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_.get());
      SDL_RenderPresent(renderer_.get());

      return true;
   }
}