#include "visualiser.hpp"

namespace nes
{
   Visualiser::SDL_Context::SDL_Context()
   {
      bool const succeeded{ SDL_Init(initialisation_flags_) };
      runtime_assert(succeeded, std::format("failed to initialise SDL ({})", SDL_GetError()));
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

      ImGuiIO& input_output{ ImGui::GetIO() };
      input_output.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
      input_output.ConfigDockingWithShift = true;
      input_output.ConfigDockingTransparentPayload = true;
      input_output.FontGlobalScale = SDL_GetWindowDisplayScale(&window);
   }

   Visualiser::ImGuiBackend::~ImGuiBackend()
   {
      ImGui_ImplSDLRenderer3_Shutdown();
      ImGui_ImplSDL3_Shutdown();
   }

   bool Visualiser::update(Memory const& memory, CPU const& processor)
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
               // TODO: the MemoryRegion's height is not correct
               float const item_height{ ImGui::GetTextLineHeightWithSpacing() };
               ImGui::BeginChild("MemoryRegion", { 0, item_height * visible_rows_ }, ImGuiChildFlags_Borders,
                  ImGuiWindowFlags_HorizontalScrollbar);
               {
                  ImGuiListClipper clipper{};
                  clipper.Begin(static_cast<int>(std::ceil(static_cast<double>(memory.size()) / bytes_per_row_)), item_height);
                  {
                     if (jump_requested_)
                     {
                        jump_address_ = std::clamp(jump_address_, {}, static_cast<ProgramCounter>(memory.size() - 1));
                        float const target_row{ jump_address_ / bytes_per_row_ - visible_rows_ / 2.0f + 0.5f };
                        ImGui::SetScrollY(target_row * item_height);
                     }

                     while (clipper.Step())
                        for (int row_index = clipper.DisplayStart; row_index < clipper.DisplayEnd; ++row_index)
                        {
                           int const base_column_index{ row_index * bytes_per_row_ };
                           ImGui::Text("%04X:", base_column_index);
                           ImGui::SameLine();

                           int const max_column_index{
                              std::min((row_index + 1) * bytes_per_row_, static_cast<int>(memory.size()))
                           };

                           for (int column_index{ base_column_index }; column_index < max_column_index; ++column_index)
                           {
                              Data const byte{ memory[column_index] };
                              if (not byte)
                                 ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 0.5f, 0.5f, 1.0f });

                              ImGui::Text("%02X", byte);

                              if (column_index == jump_address_)
                                 ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                    IM_COL32(255, 255, 255, 255));

                              if (not byte)
                                 ImGui::PopStyleColor();

                              if (column_index < max_column_index - 1)
                                 ImGui::SameLine();
                           }
                        }
                  }
                  clipper.End();
               }
               ImGui::EndChild();

               jump_requested_ = ImGui::InputScalar("Jump to address", ImGuiDataType_U16, &jump_address_, nullptr, nullptr,
                  "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

               ImGui::InputInt("Bytes per row", &bytes_per_row_, 1, 1);
               ImGui::InputInt("Visible rows", &visible_rows_, 1, 1);
            }
            ImGui::End();

            ImGui::Begin("CPU", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
            {
               ImGui::Text("Cycle: %llu", processor.cycle());
               ImGui::Text("PC: %04X", processor.program_counter());
               ImGui::Text("A: %02X", processor.accumulator());
               ImGui::Text("X: %02X", processor.x());
               ImGui::Text("Y: %02X", processor.y());
               ImGui::Text("S: %02X", processor.stack_pointer());

               auto const cast{
                  [](CPU::ProcessorStatusFlag const flag)
                  {
                     return static_cast<std::underlying_type_t<CPU::ProcessorStatusFlag>>(flag);
                  }
               };

               ProcessorStatus const processor_status{ processor.processor_status() };
               ImGui::Text(std::format("P: {}{}{}{}{}{}{}{}",
                  processor_status & cast(CPU::ProcessorStatusFlag::N) ? 'N' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::V) ? 'V' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::_) ? '_' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::B) ? 'B' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::D) ? 'D' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::I) ? 'I' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::Z) ? 'Z' : '-',
                  processor_status & cast(CPU::ProcessorStatusFlag::C) ? 'C' : '-').c_str());

               if (ImGui::Checkbox("Run", &run_); not run_)
               {
                  ImGui::SameLine();
                  tick_ = ImGui::Button("Tick");
               }
               else
                  tick_ = false;
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

   bool Visualiser::run() const
   {
      return run_;
   }

   bool Visualiser::tick() const
   {
      return tick_;
   }
}