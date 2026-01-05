#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_opcode.hpp"
#include "hardware/cpu/cpu.hpp"
#include "hardware/memory/memory.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

void tick(nes::CPU& processor) try
{
   processor.tick();
}
catch (nes::EmulationException const& exception)
{
   nes::Locator::get<nes::Logger>()->error(exception.what(), false, exception.location());
}

void tick_repeatedly(std::stop_token const& stop_token, nes::CPU& processor)
{
   while (not stop_token.stop_requested())
      tick(processor);
}

int main(int, char**)
{
   nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   nes::Memory memory{};
   nes::CPU processor{ memory };

   std::jthread emulation_thread{};
   while (visualiser.update(memory, processor))
   {
      if (visualiser.tick_once())
      {
         tick(processor);
         continue;
      }

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
   }

   nes::Locator::remove_providers();
   return 0;
}