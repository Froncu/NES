#include "visualiser.hpp"

namespace nes
{
   Visualiser::SDL_Context::SDL_Context() noexcept
   {
      bool const succeeded{ SDL_Init(initialisation_flags_) };
      runtime_assert(succeeded, std::format("failed to initialise SDL ({})", SDL_GetError()));
   }

   Visualiser::SDL_Context::~SDL_Context() noexcept
   {
      SDL_QuitSubSystem(initialisation_flags_);
   }

   Visualiser::ImGuiBackend::ImGuiBackend(SDL_Window& window, SDL_Renderer& renderer) noexcept
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

   Visualiser::ImGuiBackend::~ImGuiBackend() noexcept
   {
      ImGui_ImplSDLRenderer3_Shutdown();
      ImGui_ImplSDL3_Shutdown();
   }

   Visualiser::Visualiser() noexcept
   {
      NFD::Init();
   }

   Visualiser::~Visualiser() noexcept
   {
      NFD::Quit();
   }

   bool Visualiser::update(Memory const& memory, Processor& processor) noexcept
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
                        jump_address_ = std::clamp(jump_address_, {}, static_cast<Word>(memory.size() - 1));
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
                              Byte const byte{ memory.read(static_cast<Word>(column_index)) };
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

               if (ImGui::Button("Select program"))
               {
                  NFD::UniquePath program_path;
                  std::array constexpr filters{ nfdu8filteritem_t{ "Binaries", "bin" } };
                  switch (NFD::OpenDialog(program_path, filters.data(), static_cast<nfdfiltersize_t>(filters.size())))
                  {
                     case NFD_OKAY:
                        program_path_ = program_path.get();
                        break;

                     case NFD_CANCEL:
                        Locator::get<Logger>()->warning("file selection cancelled");
                        break;

                     default:
                        Locator::get<Logger>()->error(std::format("file selection error: {}", NFD::GetError()));
                        break;
                  }
               }
               if (exists(program_path_))
               {
                  ImGui::SameLine();
                  ImGui::Text(program_path_.filename().string().c_str());
               }

               ImGui::InputScalar("Load address", ImGuiDataType_U16, &program_load_address_, nullptr, nullptr,
                  "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

               if (exists(program_path_))
                  load_program_requested_ = ImGui::Button("Load");
            }
            ImGui::End();

            ImGui::Begin("CPU", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
            {
               ImGui::Text("Cycle: %llu", processor.cycle());
               ImGui::Text("Program counter:");
               ImGui::SameLine();
               ImGui::SetNextItemWidth(50.0f);
               ImGui::InputScalar("##hidden", ImGuiDataType_U16, &processor.program_counter,
                  nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
               ImGui::Text("A: %02X", processor.accumulator());
               ImGui::Text("X: %02X", processor.x());
               ImGui::Text("Y: %02X", processor.y());
               ImGui::Text("S: %02X", processor.stack_pointer());

               auto const cast{
                  [](Processor::ProcessorStatusFlag const flag)
                  {
                     return static_cast<std::underlying_type_t<Processor::ProcessorStatusFlag>>(flag);
                  }
               };

               ProcessorStatus const processor_status{ processor.processor_status() };
               ImGui::Text(std::format("P: {}{}{}{}{}{}{}{}",
                  processor_status & cast(Processor::ProcessorStatusFlag::N) ? 'N' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::V) ? 'V' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::_) ? '_' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::B) ? 'B' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::D) ? 'D' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::I) ? 'I' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::Z) ? 'Z' : '-',
                  processor_status & cast(Processor::ProcessorStatusFlag::C) ? 'C' : '-').c_str());

               if (ImGui::Checkbox("Tick repeatedly", &tick_repeatedly_); not tick_repeatedly_)
               {
                  ImGui::SameLine();
                  tick_once_ = ImGui::Button("Tick once");
                  step_ = ImGui::Button("Step");
               }
               else
                  tick_once_ = step_ = false;

               reset_ = ImGui::Button("Reset");
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

   bool Visualiser::tick_repeatedly() const noexcept
   {
      return tick_repeatedly_;
   }

   bool Visualiser::tick_once() const noexcept
   {
      return tick_once_;
   }

   bool Visualiser::step() const noexcept
   {
      return step_;
   }

   bool Visualiser::reset() const noexcept
   {
      return reset_;
   }

   std::filesystem::path const& Visualiser::program_path() const noexcept
   {
      return program_path_;
   }

   Word Visualiser::program_load_address() const noexcept
   {
      return program_load_address_;
   }

   bool Visualiser::load_program_requested() const noexcept
   {
      return load_program_requested_;
   }
}