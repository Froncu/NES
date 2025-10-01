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

      ImGuiIO& input_output = ImGui::GetIO();
      input_output.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
      input_output.ConfigDockingWithShift = true;
      input_output.ConfigDockingTransparentPayload = true;
   }

   Visualiser::ImGuiBackend::~ImGuiBackend()
   {
      ImGui_ImplSDLRenderer3_Shutdown();
      ImGui_ImplSDL3_Shutdown();
   }

   bool Visualiser::tick(std::array<std::uint8_t, 0x10000> const& memory)
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
      {
         ImGui::DockSpaceOverViewport();
         {
            ImGui::Begin("Memory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
            {
               ImGui::BeginChild("MemoryRegion", { 0, 300 }, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
               {
                  ImGuiListClipper clipper;
                  clipper.Begin(static_cast<int>(std::ceil(static_cast<double>(memory.size()) / bytes_per_row_)));
                  {
                     while (clipper.Step())
                        for (int row_index = clipper.DisplayStart; row_index < clipper.DisplayEnd; ++row_index)
                        {
                           int const base_column_index = row_index * bytes_per_row_;
                           ImGui::Text("%04X:", base_column_index);
                           ImGui::SameLine();

                           int const max_column_index = std::min((row_index + 1) * bytes_per_row_,
                              static_cast<int>(memory.size()));
                           for (int column_index = base_column_index; column_index < max_column_index; ++column_index)
                           {
                              std::uint8_t const byte = memory[column_index];
                              if (byte == 0)
                                 ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 0.5f, 0.5f, 1.0f });

                              ImGui::Text("%02X", byte);

                              if (byte == 0)
                                 ImGui::PopStyleColor();

                              if (column_index < max_column_index - 1)
                                 ImGui::SameLine();
                           }
                        }
                  }
                  clipper.End();
               }
               ImGui::EndChild();
               ImGui::InputInt("Bytes per row", &bytes_per_row_, 1, 1);
            }
            ImGui::End();
         }
      }
      ImGui::Render();
      SDL_RenderClear(renderer_.get());
      ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_.get());
      SDL_RenderPresent(renderer_.get());

      return true;
   }
}