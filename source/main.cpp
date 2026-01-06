#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_opcode.hpp"
#include "hardware/cpu/cpu.hpp"
#include "hardware/memory/memory.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

void tick_repeatedly(std::stop_token const& stop_token, nes::Processor& processor)
{
   while (not stop_token.stop_requested())
      processor.tick();
}

int main(int, char**)
{
   nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   nes::Memory memory{};
   nes::Processor processor{ memory };

   // memory.write(0x0000, static_cast<nes::Data>(nes::Processor::Opcode::BPL_RELATIVE));
   // memory.write(0x0001, 0xF0);

   std::jthread emulation_thread{};
   while (visualiser.update(memory, processor))
   {
      if (visualiser.tick_repeatedly())
      {
         if (not emulation_thread.joinable())
            emulation_thread = std::jthread{ tick_repeatedly, std::ref(processor) };
      }
      else if (emulation_thread.joinable())
      {
         emulation_thread.request_stop();
         emulation_thread.join();
      }
      else if (visualiser.tick_once())
         processor.tick();
      else if (visualiser.step())
         processor.step();
      else if (visualiser.reset())
         processor.reset();

      if (visualiser.load_program_requested())
         memory.load_program(visualiser.program_path(), visualiser.program_load_address());
   }

   nes::Locator::remove_providers();
   return 0;
}