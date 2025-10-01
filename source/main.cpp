#include <SDL3/SDL_main.h>

#include "exceptions/unsupported_instruction.hpp"
#include "hardware/CPU/CPU.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

int main(int, char**)
{
   nes::Locator::provide<nes::Logger>();
   auto& visualiser = nes::Locator::provide<nes::Visualiser>();

   std::array<std::uint8_t, 0x10000> memory{};
   memory[0xFFFD] = 0x06;
   memory[0xFFFC] = 0x00;
   memory[0x0600] = 0x01;
   memory[0x0601] = 0x55;
   memory[0x0055] = 0x10;
   memory[0x0011] = 0xBB;
   memory[0x0012] = 0xAA;
   memory[0xAABB] = 0xFF;

   nes::CPU processor{ memory };

   bool continue_execution = true;
   while (visualiser.update(memory, processor))
      if (continue_execution && visualiser.tick())
         try
         {
            continue_execution = processor.tick();
         }
         catch (nes::UnsupportedInstruction const& exception)
         {
            std::println("{}", exception.what());
         }

   std::println("completed {} cycles!", processor.cycle());

   nes::Locator::remove_providers();
   return 0;
}