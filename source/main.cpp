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

int main(int, char**)
{
   auto& logger = nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   nes::Memory memory{};
   nes::CPU processor{ memory };

   memory.write(0xFFFD, 0x06);
   memory.write(0xFFFC, 0x00);
   memory.write(0x0600, static_cast<std::underlying_type_t<nes::CPU::Opcode>>(nes::CPU::Opcode::ORA_INDIRECT_Y));
   memory.write(0x0601, 0xFF);
   memory.write(0x0602, static_cast<std::underlying_type_t<nes::CPU::Opcode>>(nes::CPU::Opcode::ORA_X_INDIRECT));
   memory.write(0x0603, 0x01);
   memory.write(0x0604, static_cast<std::underlying_type_t<nes::CPU::Opcode>>(nes::CPU::Opcode::ORA_ZERO_PAGE));
   memory.write(0x0605, 0xFF);

   double cycle_accumulator{};
   auto previous_time{ std::chrono::high_resolution_clock::now() };
   while (visualiser.update(memory, processor))
   {
      auto const current_time{ std::chrono::high_resolution_clock::now() };
      double const frame_time{ std::chrono::duration<double>(current_time - previous_time).count() };
      previous_time = current_time;

      if (visualiser.run())
      {
         if (auto constexpr MAX_CYCLES_PER_FRAME{ 1'000'000 };
            (cycle_accumulator += nes::CPU::FREQUENCY * frame_time) > MAX_CYCLES_PER_FRAME)
         {
            auto const discarded_cycles{ cycle_accumulator - MAX_CYCLES_PER_FRAME };
            logger.warning(std::format("exceeded max cycles per frame by {:5.2f}%; discarding {:.0f} cycles",
               discarded_cycles / MAX_CYCLES_PER_FRAME * 100, discarded_cycles));

            cycle_accumulator = MAX_CYCLES_PER_FRAME;
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