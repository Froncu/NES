#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_opcode.hpp"
#include "hardware/CPU/CPU.hpp"
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
   nes::Locator::get<nes::Logger>()->error(exception.what());
}

int main(int, char**)
{
   auto& logger = nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   nes::Memory memory{};
   nes::CPU processor{ memory };

   memory[0xFFFD] = 0x06;
   memory[0xFFFC] = 0x00;
   memory[0x0600] = 0x01;
   memory[0x0601] = 0x55;
   // memory[0x0602] = 0x02;
   memory[0x0055] = 0x10;
   memory[0x0011] = 0xBB;
   memory[0x0012] = 0xAA;
   memory[0xAABB] = 0xFF;

   double cycle_accumulator = 0.0;
   auto previous_time = std::chrono::high_resolution_clock::now();
   while (visualiser.update(memory, processor))
   {
      auto const current_time = std::chrono::high_resolution_clock::now();
      double const frame_time = std::chrono::duration<double>(current_time - previous_time).count();
      previous_time = current_time;

      if (visualiser.run())
      {
         if ((cycle_accumulator += nes::CPU::FREQUENCY * frame_time) > nes::CPU::CYCLES_PER_FRAME)
         {
            auto const discarded_cycles = cycle_accumulator - nes::CPU::CYCLES_PER_FRAME;
            logger.warning(std::format("exceeded the cycles per frame limit by {:5.2f}%; discarding {:.0f} cycles",
               discarded_cycles / nes::CPU::CYCLES_PER_FRAME * 100, discarded_cycles));

            cycle_accumulator = nes::CPU::CYCLES_PER_FRAME;
         }

         while (cycle_accumulator-- >= 1)
            tick(processor);
      }
      else if (visualiser.tick())
         tick(processor);
   }

   nes::Locator::remove_providers();
   return 0;
}