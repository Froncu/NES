#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_opcode.hpp"
#include "hardware/memory/memory.hpp"
#include "hardware/processor/processor.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

void handle_exception(nes::UnsupportedOpcode const& exception,
   std::source_location source_location = std::source_location::current())
{
   nes::Locator::get<nes::Logger>()->error(exception.what(), false, std::move(source_location));
}

void try_tick(nes::Processor& processor) try
{
   processor.tick();
}
catch (nes::UnsupportedOpcode const& exception)
{
   handle_exception(exception);
}

void try_step(nes::Processor& processor) try
{
   while (not processor.tick());
}
catch (nes::UnsupportedOpcode const& exception)
{
   handle_exception(exception);
}

void try_tick_repeatedly(std::stop_token const& stop_token, nes::Processor& processor)
{
   while (not stop_token.stop_requested())
      try_tick(processor);
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
            emulation_thread = std::jthread{ try_tick_repeatedly, std::ref(processor) };
      }
      else if (emulation_thread.joinable())
      {
         emulation_thread.request_stop();
         emulation_thread.join();
      }
      else if (visualiser.tick_once())
         try_tick(processor);
      else if (visualiser.step())
         try_step(processor);
      else if (visualiser.reset())
         processor.reset();

      if (visualiser.load_program_requested())
         memory.load_program(visualiser.program_path(), visualiser.program_load_address());
   }

   nes::Locator::remove_providers();
   return 0;
}