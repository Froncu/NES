#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_opcode.hpp"
#include "hardware/cpu/cpu.hpp"
#include "hardware/memory/memory.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

void try_invoke(nes::Processor& processor, void(nes::Processor::* const function)()) try
{
   std::invoke(function, processor);
}
catch (nes::EmulationException const& exception)
{
   nes::Locator::get<nes::Logger>()->error(exception.what(), false, exception.location());
}

void tick_repeatedly(std::stop_token const& stop_token, nes::Processor& processor)
{
   while (not stop_token.stop_requested())
      try_invoke(processor, &nes::Processor::tick);
}

int main(int, char**)
{
   nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   nes::Memory memory{};
   nes::Processor processor{ memory };

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
         try_invoke(processor, &nes::Processor::tick);
      else if (visualiser.step())
         try_invoke(processor, &nes::Processor::step);
      else if (visualiser.reset())
         processor.reset();

      if (visualiser.load_program_requested())
         memory.load_program(visualiser.program_path(), visualiser.program_load_address());
   }

   nes::Locator::remove_providers();
   return 0;
}